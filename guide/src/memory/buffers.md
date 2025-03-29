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

Buffers can be backed by host (RAM) or device (VRAM) memory: the former is mappable and thus useful for data that changes every frame, latter is faster to access for the GPU but needs more complex methods to copy data from the CPU to it. Leaving device buffers for later, add the `Buffer` alias and a create function:

```cpp
using Buffer = Scoped<RawBuffer, BufferDeleter>;

[[nodiscard]] auto create_host_buffer(VmaAllocator allocator,
                                      vk::BufferUsageFlags usage,
                                      vk::DeviceSize size) -> Buffer;
```

Add a helper function that can be reused for device buffers too later:

```cpp
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
```

Implement `create_host_buffer()`:

```cpp
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
```
