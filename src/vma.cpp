#include <vma.hpp>
#include <numeric>
#include <print>
#include <stdexcept>

namespace lvk {
namespace vma {
void Deleter::operator()(VmaAllocator allocator) const noexcept {
	vmaDestroyAllocator(allocator);
}

void BufferDeleter::operator()(RawBuffer const& raw_buffer) const noexcept {
	vmaDestroyBuffer(raw_buffer.allocator, raw_buffer.buffer,
					 raw_buffer.allocation);
}

void ImageDeleter::operator()(RawImage const& raw_image) const noexcept {
	vmaDestroyImage(raw_image.allocator, raw_image.image, raw_image.allocation);
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

auto vma::create_buffer(BufferCreateInfo const& create_info,
						BufferMemoryType const memory_type,
						vk::DeviceSize const size) -> Buffer {
	if (size == 0) {
		std::println(stderr, "Buffer cannot be 0-sized");
		return {};
	}

	auto allocation_ci = VmaAllocationCreateInfo{};
	allocation_ci.flags =
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	auto usage = create_info.usage;
	if (memory_type == BufferMemoryType::Device) {
		allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		// device buffers need to support TransferDst.
		usage |= vk::BufferUsageFlagBits::eTransferDst;
	} else {
		allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		// host buffers can provide mapped memory.
		allocation_ci.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	auto buffer_ci = vk::BufferCreateInfo{};
	buffer_ci.setQueueFamilyIndices(create_info.queue_family)
		.setSize(size)
		.setUsage(usage);
	auto vma_buffer_ci = static_cast<VkBufferCreateInfo>(buffer_ci);

	VmaAllocation allocation{};
	VkBuffer buffer{};
	auto allocation_info = VmaAllocationInfo{};
	auto const result =
		vmaCreateBuffer(create_info.allocator, &vma_buffer_ci, &allocation_ci,
						&buffer, &allocation, &allocation_info);
	if (result != VK_SUCCESS) {
		std::println(stderr, "Failed to create VMA Buffer");
		return {};
	}

	return RawBuffer{
		.allocator = create_info.allocator,
		.allocation = allocation,
		.buffer = buffer,
		.size = size,
		.mapped = allocation_info.pMappedData,
	};
}

auto vma::create_device_buffer(BufferCreateInfo const& create_info,
							   CommandBlock command_block,
							   ByteSpans const& byte_spans) -> Buffer {
	auto const total_size = std::accumulate(
		byte_spans.begin(), byte_spans.end(), 0uz,
		[](std::size_t const n, std::span<std::byte const> bytes) {
			return n + bytes.size();
		});

	auto staging_ci = create_info;
	staging_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;

	// create staging Host Buffer with TransferSrc usage.
	auto staging_buffer =
		create_buffer(staging_ci, BufferMemoryType::Host, total_size);
	// create the Device Buffer.
	auto ret = create_buffer(create_info, BufferMemoryType::Device, total_size);
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

auto vma::create_image(ImageCreateInfo const& create_info,
					   vk::ImageUsageFlags const usage,
					   std::uint32_t const levels, vk::Format const format,
					   vk::Extent2D const extent) -> Image {
	if (extent.width == 0 || extent.height == 0) {
		std::println(stderr, "Images cannot have 0 width or height");
		return {};
	}
	auto image_ci = vk::ImageCreateInfo{};
	image_ci.setImageType(vk::ImageType::e2D)
		.setExtent({extent.width, extent.height, 1})
		.setFormat(format)
		.setUsage(usage)
		.setArrayLayers(1)
		.setMipLevels(levels)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setQueueFamilyIndices(create_info.queue_family);
	auto const vk_image_ci = static_cast<VkImageCreateInfo>(image_ci);

	auto allocation_ci = VmaAllocationCreateInfo{};
	allocation_ci.usage = VMA_MEMORY_USAGE_AUTO;
	VkImage image{};
	VmaAllocation allocation{};
	auto const result = vmaCreateImage(create_info.allocator, &vk_image_ci,
									   &allocation_ci, &image, &allocation, {});
	if (result != VK_SUCCESS) {
		std::println(stderr, "Failed to create VMA Image");
		return {};
	}

	return RawImage{
		.allocator = create_info.allocator,
		.allocation = allocation,
		.image = image,
		.extent = extent,
		.format = format,
		.levels = levels,
	};
}

auto vma::create_sampled_image(ImageCreateInfo const& create_info,
							   CommandBlock command_block, Bitmap const& bitmap)
	-> Image {
	// create image.
	// no mip-mapping right now: 1 level.
	auto const mip_levels = 1u;
	auto const usize = glm::uvec2{bitmap.size};
	auto const extent = vk::Extent2D{usize.x, usize.y};
	auto const usage =
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	auto ret = create_image(create_info, usage, mip_levels,
							vk::Format::eR8G8B8A8Srgb, extent);

	// create staging buffer.
	auto const buffer_ci = BufferCreateInfo{
		.allocator = create_info.allocator,
		.usage = vk::BufferUsageFlagBits::eTransferSrc,
		.queue_family = create_info.queue_family,
	};
	auto const staging_buffer = create_buffer(buffer_ci, BufferMemoryType::Host,
											  bitmap.bytes.size_bytes());

	// can't do anything if either creation failed.
	if (!ret.get().image || !staging_buffer.get().buffer) { return {}; }

	// copy bytes into staging buffer.
	std::memcpy(staging_buffer.get().mapped, bitmap.bytes.data(),
				bitmap.bytes.size_bytes());

	// transition image for transfer.
	auto dependency_info = vk::DependencyInfo{};
	auto subresource_range = vk::ImageSubresourceRange{};
	subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setLayerCount(1)
		.setLevelCount(mip_levels);
	auto barrier = vk::ImageMemoryBarrier2{};
	barrier.setImage(ret.get().image)
		.setSrcQueueFamilyIndex(create_info.queue_family)
		.setDstQueueFamilyIndex(create_info.queue_family)
		.setOldLayout(vk::ImageLayout::eUndefined)
		.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
		.setSubresourceRange(subresource_range)
		.setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
		.setSrcAccessMask(vk::AccessFlagBits2::eNone)
		.setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
		.setDstAccessMask(vk::AccessFlagBits2::eMemoryRead |
						  vk::AccessFlagBits2::eMemoryWrite);
	dependency_info.setImageMemoryBarriers(barrier);
	command_block.command_buffer().pipelineBarrier2(dependency_info);

	// record buffer image copy.
	auto buffer_image_copy = vk::BufferImageCopy2{};
	auto subresource_layers = vk::ImageSubresourceLayers{};
	subresource_layers.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setLayerCount(1)
		.setLayerCount(mip_levels);
	buffer_image_copy.setImageSubresource(subresource_layers)
		.setImageExtent(vk::Extent3D{extent.width, extent.height, 1});
	auto copy_info = vk::CopyBufferToImageInfo2{};
	copy_info.setDstImage(ret.get().image)
		.setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
		.setSrcBuffer(staging_buffer.get().buffer)
		.setRegions(buffer_image_copy);
	command_block.command_buffer().copyBufferToImage2(copy_info);

	// transition image for sampling.
	barrier.setOldLayout(barrier.newLayout)
		.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
		.setSrcStageMask(barrier.dstStageMask)
		.setSrcAccessMask(barrier.dstAccessMask)
		.setDstStageMask(vk::PipelineStageFlagBits2::eAllGraphics)
		.setDstAccessMask(vk::AccessFlagBits2::eMemoryRead |
						  vk::AccessFlagBits2::eMemoryWrite);
	dependency_info.setImageMemoryBarriers(barrier);
	command_block.command_buffer().pipelineBarrier2(dependency_info);

	command_block.submit_and_wait();

	return ret;
}
} // namespace lvk
