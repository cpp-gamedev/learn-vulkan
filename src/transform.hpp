#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace lvk {
struct Transform {
	glm::vec2 position{};
	float rotation{};
	glm::vec2 scale{1.0f};

	[[nodiscard]] auto model_matrix() const -> glm::mat4;
	[[nodiscard]] auto view_matrix() const -> glm::mat4;
};
} // namespace lvk
