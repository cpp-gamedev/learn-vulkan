#pragma once
#include <vk_mem_alloc.h>
#include <scoped.hpp>
#include <vulkan/vulkan.hpp>
#include <cstdint>

namespace lvk::vma {
struct Deleter {
	void operator()(VmaAllocator allocator) const noexcept;
};

using Allocator = Scoped<VmaAllocator, Deleter>;

[[nodiscard]] auto create_allocator(vk::Instance instance,
									vk::PhysicalDevice physical_device,
									vk::Device device) -> Allocator;

struct RawBuffer {
	VmaAllocator allocator{};
	VmaAllocation allocation{};
	vk::Buffer buffer{};
	vk::DeviceSize capacity{};
	vk::DeviceSize size{};
	void* mapped{};

	auto operator==(RawBuffer const& rhs) const -> bool = default;
};

struct BufferDeleter {
	void operator()(RawBuffer const& raw_buffer) const noexcept;
};

enum class BufferType : std::int8_t { Host, Device };

struct BufferCreateInfo {
	vk::BufferUsageFlags usage{};
	vk::DeviceSize size{};
	BufferType type{BufferType::Host};
};

class Buffer {
  public:
	using CreateInfo = BufferCreateInfo;
	using Type = BufferType;

	explicit Buffer(VmaAllocator allocator, CreateInfo const& create_info);

	[[nodiscard]] auto get_type() const -> Type {
		return m_buffer.get().mapped == nullptr ? Type::Device : Type::Host;
	}

	[[nodiscard]] auto get_usage() const -> vk::BufferUsageFlags {
		return m_usage;
	}

	[[nodiscard]] auto get_raw() const -> RawBuffer const& {
		return m_buffer.get();
	}

	auto resize(vk::DeviceSize size) -> bool;

  private:
	auto create(VmaAllocator allocator, Type type, vk::DeviceSize size) -> bool;

	Scoped<RawBuffer, BufferDeleter> m_buffer{};
	vk::BufferUsageFlags m_usage{};
};
} // namespace lvk::vma
