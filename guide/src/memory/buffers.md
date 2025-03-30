# Buffers

First add the RAII wrapper components for VMA buffers:

```cpp
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

// ...
void BufferDeleter::operator()(RawBuffer const& raw_buffer) const noexcept {
  vmaDestroyBuffer(raw_buffer.allocator, raw_buffer.buffer,
                   raw_buffer.allocation);
}
```

Buffers can be backed by host (RAM) or device (VRAM) memory: the former is mappable and thus useful for data that changes every frame, latter is faster to access for the GPU but needs more complex methods to copy data to. Add the related types and a create function:

```cpp
struct BufferCreateInfo {
  VmaAllocator allocator;
  vk::BufferUsageFlags usage;
  std::uint32_t queue_family;
};

enum class BufferMemoryType : std::int8_t { Host, Device };

[[nodiscard]] auto create_buffer(BufferCreateInfo const& create_info,
                                 BufferMemoryType memory_type,
                                 vk::DeviceSize size) -> Buffer;

// ...
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
```
