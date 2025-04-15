#pragma once
#include <glm/vec2.hpp>
#include <cstddef>
#include <span>

namespace lvk {
struct Bitmap {
	std::span<std::byte const> bytes{};
	glm::ivec2 size{};
};
} // namespace lvk
