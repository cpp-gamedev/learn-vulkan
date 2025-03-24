#include <pipeline_builder.hpp>

namespace lvk {
PipelineBuilder::PipelineBuilder(CreateInfo const& create_info)
	: m_info(create_info) {}

auto PipelineBuilder::build(vk::PipelineLayout const layout,
							PipelineState const& state) const
	-> vk::UniquePipeline {
	auto shader_stages = std::array<vk::PipelineShaderStageCreateInfo, 2>{};
	shader_stages[0]
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setPName("main")
		.setModule(state.vertex_shader);
	shader_stages[1]
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setPName("main")
		.setModule(state.fragment_shader);

	auto prsci = vk::PipelineRasterizationStateCreateInfo{};
	prsci.setPolygonMode(state.polygon_mode).setCullMode(state.cull_mode);

	auto pdssci = vk::PipelineDepthStencilStateCreateInfo{};
	auto const depth_test =
		(state.flags & PipelineFlag::DepthTest) == PipelineFlag::DepthTest;
	pdssci.setDepthTestEnable(depth_test ? vk::True : vk::False)
		.setDepthCompareOp(state.depth_compare);

	auto const piasci =
		vk::PipelineInputAssemblyStateCreateInfo{{}, state.topology};

	auto pcbas = vk::PipelineColorBlendAttachmentState{};
	auto const alpha_blend =
		(state.flags & PipelineFlag::AlphaBlend) == PipelineFlag::AlphaBlend;
	using CCF = vk::ColorComponentFlagBits;
	pcbas.setColorWriteMask(CCF::eR | CCF::eG | CCF::eB | CCF::eA)
		.setBlendEnable(alpha_blend ? vk::True : vk::False)
		.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		.setAlphaBlendOp(vk::BlendOp::eAdd);
	auto pcbsci = vk::PipelineColorBlendStateCreateInfo{};
	pcbsci.setAttachments(pcbas);

	auto const pdscis = std::array{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
		vk::DynamicState::eLineWidth,
	};
	auto pdsci = vk::PipelineDynamicStateCreateInfo{};
	pdsci.setDynamicStates(pdscis);

	auto const pvsci = vk::PipelineViewportStateCreateInfo({}, 1, {}, 1);

	auto pmsci = vk::PipelineMultisampleStateCreateInfo{};
	pmsci.setRasterizationSamples(m_info.samples)
		.setSampleShadingEnable(vk::False);

	auto prci = vk::PipelineRenderingCreateInfo{};
	if (m_info.color_format != vk::Format::eUndefined) {
		prci.setColorAttachmentFormats(m_info.color_format);
	}
	prci.setDepthAttachmentFormat(m_info.depth_format);

	auto gpci = vk::GraphicsPipelineCreateInfo{};
	gpci.setStages(shader_stages)
		.setPRasterizationState(&prsci)
		.setPDepthStencilState(&pdssci)
		.setPInputAssemblyState(&piasci)
		.setPColorBlendState(&pcbsci)
		.setPDynamicState(&pdsci)
		.setPViewportState(&pvsci)
		.setPMultisampleState(&pmsci)
		.setLayout(layout)
		.setPNext(&prci);

	auto ret = vk::Pipeline{};
	if (m_info.device.createGraphicsPipelines({}, 1, &gpci, {}, &ret) !=
		vk::Result::eSuccess) {
		return {};
	}

	return vk::UniquePipeline{ret, m_info.device};
}
} // namespace lvk
