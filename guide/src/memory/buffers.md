# Buffers

First add the RAII wrapper components for VMA buffers:

```cpp
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

// ...
void BufferDeleter::operator()(RawBuffer const& raw_buffer) const noexcept {
  vmaDestroyBuffer(raw_buffer.allocator, raw_buffer.buffer,
                   raw_buffer.allocation);
}
```

Buffers can be backed by host (RAM) or device (VRAM) memory: the former is mappable and thus useful for data that changes every frame, latter is faster to access for the GPU but needs more complex methods to copy data from the CPU to it. Add the relevant subset of parameters and the RAII wrapper:

```cpp
enum class BufferType : std::int8_t { Host, Device };

struct BufferCreateInfo {
  vk::BufferUsageFlags usage{};
  vk::DeviceSize size{};
  BufferType type{BufferType::Host};
};

class Buffer {
 public:
  using CreateInfo = BufferCreateInfo;

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
  auto create(VmaAllocator allocator, vk::DeviceSize size) -> bool;

  Scoped<RawBuffer, BufferDeleter> m_buffer{};
  vk::BufferUsageFlags m_usage{};
};
```

`resize()` and `create()` are separate because the former uses the existing `m_buffer`'s allocator. The implementation:

```cpp
[[nodiscard]] constexpr auto positive_size(vk::DeviceSize const in) {
  return in > 0 ? in : 1;
}

// ...
Buffer::Buffer(VmaAllocator allocator, CreateInfo const& create_info)
  : m_usage(create_info.usage) {
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
```
