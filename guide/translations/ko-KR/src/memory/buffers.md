# 버퍼

먼저 VMA 버퍼를 위한 RAII 래퍼 컴포넌트를 추가합니다.

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

버퍼는 호스트(RAM)와 디바이스(VRAM) 메모리를 기반으로 할당될 수 있습니다. 호스트 메모리는 매핑이 가능하므로 매 프레임마다 바뀌는 정보를 담기에 적합하며, 디바이스 메모리는 GPU에서 접근하기에 빠르지만 데이터를 복사하는 데 더 복잡한 절차가 필요합니다. 이를 고려하여 관련된 타입과 생성 함수를 추가하겠습니다.

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
