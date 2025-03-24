#pragma once
#include <pipeline_state.hpp>

namespace lvk {
struct PipelineBuilderCreateInfo {
	vk::Device device{};
	vk::SampleCountFlagBits samples{};
	vk::Format color_format{};
	vk::Format depth_format{};
};

class PipelineBuilder {
  public:
	using CreateInfo = PipelineBuilderCreateInfo;

	explicit PipelineBuilder(CreateInfo const& create_info);

	[[nodiscard]] auto build(vk::PipelineLayout layout,
							 PipelineState const& state) const
		-> vk::UniquePipeline;

  private:
	CreateInfo m_info{};
};
} // namespace lvk
