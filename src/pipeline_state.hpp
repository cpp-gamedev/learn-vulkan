#pragma once
#include <vulkan/vulkan.hpp>

namespace lvk {
struct PipelineFlag {
	enum : std::uint8_t {
		None = 0,
		AlphaBlend = 1 << 0,
		DepthTest = 1 << 1,
	};
};

struct PipelineState {
	using Flag = PipelineFlag;

	[[nodiscard]] static constexpr auto default_flags() -> std::uint8_t {
		return Flag::AlphaBlend | Flag::DepthTest;
	}

	vk::ShaderModule vertex_shader;
	vk::ShaderModule fragment_shader;

	vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
	vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};
	vk::CullModeFlags cull_mode{vk::CullModeFlagBits::eNone};
	vk::CompareOp depth_compare{vk::CompareOp::eLess};
	std::uint8_t flags{default_flags()};
};
} // namespace lvk
