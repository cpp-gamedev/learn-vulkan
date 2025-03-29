#pragma once
#include <vk_mem_alloc.h>
#include <scoped.hpp>
#include <vulkan/vulkan.hpp>

namespace lvk::vma {
struct Deleter {
	void operator()(VmaAllocator allocator) const noexcept;
};

using Allocator = Scoped<VmaAllocator, Deleter>;

[[nodiscard]] auto create_allocator(vk::Instance instance,
									vk::PhysicalDevice physical_device,
									vk::Device device) -> Allocator;
} // namespace lvk::vma
