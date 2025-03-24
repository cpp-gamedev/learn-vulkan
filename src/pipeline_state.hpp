#pragma once
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <span>

namespace lvk {
// bit flags for various binary Pipeline States.
struct PipelineFlag {
	enum : std::uint8_t {
		None = 0,
		AlphaBlend = 1 << 0, // turn on alpha blending.
		DepthTest = 1 << 1,	 // turn on depth write and test.
	};
};

// specification of a unique Graphics Pipeline.
struct PipelineState {
	using Flag = PipelineFlag;

	[[nodiscard]] static constexpr auto default_flags() -> std::uint8_t {
		return Flag::AlphaBlend | Flag::DepthTest;
	}

	vk::ShaderModule vertex_shader;	  // required.
	vk::ShaderModule fragment_shader; // required.

	std::span<vk::VertexInputAttributeDescription const> vertex_attributes{};
	std::span<vk::VertexInputBindingDescription const> vertex_bindings{};

	vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
	vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};
	vk::CullModeFlags cull_mode{vk::CullModeFlagBits::eNone};
	vk::CompareOp depth_compare{vk::CompareOp::eLess};
	std::uint8_t flags{default_flags()};
};
} // namespace lvk
