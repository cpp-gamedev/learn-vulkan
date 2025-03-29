#include <vma.hpp>
#include <print>
#include <stdexcept>

namespace lvk {
namespace vma {
namespace {
[[nodiscard]] auto create_buffer(VmaAllocator allocator,
								 VmaAllocationCreateInfo const& allocation_ci,
								 vk::BufferUsageFlags const usage,
								 vk::DeviceSize const size) -> Buffer {
	if (size == 0) {
		std::println(stderr, "Buffer cannot be 0-sized");
		return {};
	}

	auto buffer_ci = vk::BufferCreateInfo{};
	buffer_ci.setSize(size).setUsage(usage);
	auto vma_buffer_ci = static_cast<VkBufferCreateInfo>(buffer_ci);

	VmaAllocation allocation{};
	VkBuffer buffer{};
	auto allocation_info = VmaAllocationInfo{};
	auto const result =
		vmaCreateBuffer(allocator, &vma_buffer_ci, &allocation_ci, &buffer,
						&allocation, &allocation_info);
	if (result != VK_SUCCESS) {
		std::println(stderr, "Failed to create VMA Buffer");
		return {};
	}

	return RawBuffer{
		.allocator = allocator,
		.allocation = allocation,
		.buffer = buffer,
		.size = size,
		.mapped = allocation_info.pMappedData,
	};
}
} // namespace

void Deleter::operator()(VmaAllocator allocator) const noexcept {
	vmaDestroyAllocator(allocator);
}

void BufferDeleter::operator()(RawBuffer const& raw_buffer) const noexcept {
	vmaDestroyBuffer(raw_buffer.allocator, raw_buffer.buffer,
					 raw_buffer.allocation);
}
} // namespace vma

auto vma::create_allocator(vk::Instance const instance,
						   vk::PhysicalDevice const physical_device,
						   vk::Device const device) -> Allocator {
	auto const& dispatcher = VULKAN_HPP_DEFAULT_DISPATCHER;
	// need to zero initialize C structs, unlike VulkanHPP.
	auto vma_vk_funcs = VmaVulkanFunctions{};
	vma_vk_funcs.vkGetInstanceProcAddr = dispatcher.vkGetInstanceProcAddr;
	vma_vk_funcs.vkGetDeviceProcAddr = dispatcher.vkGetDeviceProcAddr;

	auto allocator_ci = VmaAllocatorCreateInfo{};
	allocator_ci.physicalDevice = physical_device;
	allocator_ci.device = device;
	allocator_ci.pVulkanFunctions = &vma_vk_funcs;
	allocator_ci.instance = instance;
	VmaAllocator ret{};
	auto const result = vmaCreateAllocator(&allocator_ci, &ret);
	if (result == VK_SUCCESS) { return ret; }

	throw std::runtime_error{"Failed to create Vulkan Memory Allocator"};
}

auto vma::create_host_buffer(VmaAllocator allocator,
							 vk::BufferUsageFlags const usage,
							 vk::DeviceSize const size) -> Buffer {
	auto allocation_ci = VmaAllocationCreateInfo{};
	allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	allocation_ci.flags =
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;
	return create_buffer(allocator, allocation_ci, usage, size);
}
} // namespace lvk
