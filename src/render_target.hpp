#pragma once
#include <vulkan/vulkan.hpp>

namespace lvk {
struct RenderTarget {
	vk::Image image{};
	vk::ImageView image_view{};
	vk::Extent2D extent{};
};
} // namespace lvk
