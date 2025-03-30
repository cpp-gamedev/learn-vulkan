#pragma once
#include <vk_mem_alloc.h>
#include <command_block.hpp>
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

struct RawBuffer {
	[[nodiscard]] auto mapped_span() const -> std::span<std::byte> {
		return std::span{static_cast<std::byte*>(mapped), size};
	}

	auto operator==(RawBuffer const& rhs) const -> bool = default;

	VmaAllocator allocator{};
	VmaAllocation allocation{};
	vk::Buffer buffer{};
	vk::DeviceSize size{};
	void* mapped{};
};

struct BufferDeleter {
	void operator()(RawBuffer const& raw_buffer) const noexcept;
};

using Buffer = Scoped<RawBuffer, BufferDeleter>;

struct BufferCreateInfo {
	VmaAllocator allocator;
	vk::BufferUsageFlags usage;
	std::uint32_t queue_family;
};

enum class BufferMemoryType : std::int8_t { Host, Device };

[[nodiscard]] auto create_buffer(BufferCreateInfo const& create_info,
								 BufferMemoryType memory_type,
								 vk::DeviceSize size) -> Buffer;

// disparate byte spans.
using ByteSpans = std::span<std::span<std::byte const> const>;

// returns a Device Buffer with each byte span sequentially written.
[[nodiscard]] auto create_device_buffer(BufferCreateInfo const& create_info,
										CommandBlock command_block,
										ByteSpans const& byte_spans) -> Buffer;

struct RawImage {
	auto operator==(RawImage const& rhs) const -> bool = default;

	VmaAllocator allocator{};
	VmaAllocation allocation{};
	vk::Image image{};
	vk::Extent2D extent{};
	vk::Format format{};
	std::uint32_t levels{};
};

struct ImageDeleter {
	void operator()(RawImage const& raw_image) const noexcept;
};

using Image = Scoped<RawImage, ImageDeleter>;

struct ImageCreateInfo {
	std::uint32_t queue_family;
	vk::ImageUsageFlags usage;
	vk::Format format;
	vk::Extent2D extent;
	std::uint32_t levels;
};

[[nodiscard]] auto create_image(VmaAllocator allocator,
								ImageCreateInfo const& create_info) -> Image;
} // namespace lvk::vma
