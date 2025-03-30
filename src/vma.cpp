#include <vma.hpp>
#include <numeric>
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

auto vma::create_device_buffer(VmaAllocator allocator,
							   vk::BufferUsageFlags usage,
							   CommandBlock command_block,
							   ByteSpans const& byte_spans) -> Buffer {
	auto const total_size = std::accumulate(
		byte_spans.begin(), byte_spans.end(), 0uz,
		[](std::size_t const n, std::span<std::byte const> bytes) {
			return n + bytes.size();
		});

	// create staging Host Buffer with TransferSrc usage.
	auto staging_buffer = create_host_buffer(
		allocator, vk::BufferUsageFlagBits::eTransferSrc, total_size);

	// create the Device Buffer, ensuring TransferDst usage.
	usage |= vk::BufferUsageFlagBits::eTransferDst;
	auto allocation_ci = VmaAllocationCreateInfo{};
	allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocation_ci.flags =
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	auto ret = create_buffer(allocator, allocation_ci, usage, total_size);

	// can't do anything if either buffer creation failed.
	if (!staging_buffer.get().buffer || !ret.get().buffer) { return {}; }

	// copy byte spans into staging buffer.
	auto dst = staging_buffer.get().mapped_span();
	for (auto const bytes : byte_spans) {
		std::memcpy(dst.data(), bytes.data(), bytes.size());
		dst = dst.subspan(bytes.size());
	}

	// record buffer copy operation.
	auto buffer_copy = vk::BufferCopy2{};
	buffer_copy.setSize(total_size);
	auto copy_buffer_info = vk::CopyBufferInfo2{};
	copy_buffer_info.setSrcBuffer(staging_buffer.get().buffer)
		.setDstBuffer(ret.get().buffer)
		.setRegions(buffer_copy);
	command_block.command_buffer().copyBuffer2(copy_buffer_info);

	// submit and wait.
	// waiting here is necessary to keep the staging buffer alive while the GPU
	// accesses it through the recorded commands.
	// this is also why the function takes ownership of the passed CommandBlock
	// instead of just referencing it / taking a vk::CommandBuffer.
	command_block.submit_and_wait();

	return ret;
}
} // namespace lvk
