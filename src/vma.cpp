#include <vma.hpp>
#include <print>
#include <stdexcept>

namespace lvk {
namespace vma {
namespace {
[[nodiscard]] constexpr auto positive_size(vk::DeviceSize const in) {
	return in > 0 ? in : 1;
}
} // namespace

void Deleter::operator()(VmaAllocator allocator) const noexcept {
	vmaDestroyAllocator(allocator);
}

void BufferDeleter::operator()(RawBuffer const& raw_buffer) const noexcept {
	vmaDestroyBuffer(raw_buffer.allocator, raw_buffer.buffer,
					 raw_buffer.allocation);
}

Buffer::Buffer(VmaAllocator allocator, CreateInfo const& create_info)
	: m_usage(create_info.usage) {
	if (create_info.type == Type::Device) {
		// device buffers require a transfer operation to copy data.
		m_usage |= vk::BufferUsageFlagBits::eTransferDst;
	}
	create(allocator, create_info.type, create_info.size);
}

auto Buffer::resize(vk::DeviceSize const size) -> bool {
	if (size <= m_buffer.get().capacity) {
		m_buffer.get().size = size;
		return true;
	}
	return create(m_buffer.get().allocator, get_type(), size);
}

auto Buffer::create(VmaAllocator allocator, Type const type,
					vk::DeviceSize size) -> bool {
	// buffers cannot be zero sized.
	size = positive_size(size);
	auto allocation_ci = VmaAllocationCreateInfo{};
	allocation_ci.flags =
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	if (type == BufferType::Device) {
		allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	} else {
		allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		allocation_ci.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	auto buffer_ci = vk::BufferCreateInfo{};
	buffer_ci.setSize(size).setUsage(m_usage);
	auto vma_buffer_ci = static_cast<VkBufferCreateInfo>(buffer_ci);

	VmaAllocation allocation{};
	VkBuffer buffer{};
	auto allocation_info = VmaAllocationInfo{};
	auto const result =
		vmaCreateBuffer(allocator, &vma_buffer_ci, &allocation_ci, &buffer,
						&allocation, &allocation_info);
	if (result != VK_SUCCESS) {
		std::println(stderr, "Failed to create VMA Buffer");
		return false;
	}

	m_buffer = RawBuffer{
		.allocator = allocator,
		.allocation = allocation,
		.buffer = buffer,
		.capacity = size,
		.size = size,
		.mapped = allocation_info.pMappedData,
	};
	return true;
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
} // namespace lvk
