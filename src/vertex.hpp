#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

namespace lvk {
struct Vertex {
	glm::vec2 position{};
	glm::vec3 color{1.0f};
	glm::vec2 uv{};
};

// two vertex attributes: position at 0, color at 1.
constexpr auto vertex_attributes_v = std::array{
	// the format matches the type and layout of data: vec2 => 2x 32-bit floats.
	vk::VertexInputAttributeDescription2EXT{0, 0, vk::Format::eR32G32Sfloat,
											offsetof(Vertex, position)},
	// vec3 => 3x 32-bit floats
	vk::VertexInputAttributeDescription2EXT{1, 0, vk::Format::eR32G32B32Sfloat,
											offsetof(Vertex, color)},
	// vec2 => 2x 32-bit floats
	vk::VertexInputAttributeDescription2EXT{2, 0, vk::Format::eR32G32Sfloat,
											offsetof(Vertex, uv)},
};

// one vertex binding at location 0.
constexpr auto vertex_bindings_v = std::array{
	// we are using interleaved data with a stride of sizeof(Vertex).
	vk::VertexInputBindingDescription2EXT{0, sizeof(Vertex),
										  vk::VertexInputRate::eVertex, 1},
};
} // namespace lvk
