# Command Block

오래 유지되는 정점 버퍼의 경우 디바이스 메모리에 위치하는 편이 더 나은 성능을 보입니다. 특히 3D 메시와 같은 경우에 그렇습니다. 데이터를 디바이스 버퍼로 전송하려면 두 단계가 필요합니다.

1. 호스트 버퍼를 할당하고 데이터를 매핑된 메모리로 복사합니다.
2. 디바이스 버퍼를 할당하고 메모리 복사 명령을 기록한 뒤, 이를 제출합니다.

두 번째 단계는 커맨드 버퍼와 큐 제출이 필요하며, 제출된 작업이 완료될 때까지 기다려야 합니다. 이 동작을 클래스로 캡슐화하겠습니다. 이 구조는 이미지를 생성할때에도 쓰일 수 있습니다.

```cpp
class CommandBlock {
 public:
  explicit CommandBlock(vk::Device device, vk::Queue queue,
                        vk::CommandPool command_pool);

  [[nodiscard]] auto command_buffer() const -> vk::CommandBuffer {
    return *m_command_buffer;
  }

  void submit_and_wait();

 private:
  vk::Device m_device{};
  vk::Queue m_queue{};
  vk::UniqueCommandBuffer m_command_buffer{};
};
```

이 클래스의 생성자는 임시 할당용으로 미리 생성된 커맨드 풀과, 나중에 명령을 제출할 큐를 인자로 받습니다. 이렇게 하면 해당 객체를 생성 후 다른 코드로 전달해 재사용할 수 있습니다.

```cpp
CommandBlock::CommandBlock(vk::Device const device, vk::Queue const queue,
               vk::CommandPool const command_pool)
  : m_device(device), m_queue(queue) {
  // allocate a UniqueCommandBuffer which will free the underlying command
  // buffer from its owning pool on destruction.
  auto allocate_info = vk::CommandBufferAllocateInfo{};
  allocate_info.setCommandPool(command_pool)
    .setCommandBufferCount(1)
    .setLevel(vk::CommandBufferLevel::ePrimary);
  // all the current VulkanHPP functions for UniqueCommandBuffer allocation
  // return vectors.
  auto command_buffers = m_device.allocateCommandBuffersUnique(allocate_info);
  m_command_buffer = std::move(command_buffers.front());

  // start recording commands before returning.
  auto begin_info = vk::CommandBufferBeginInfo{};
  begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  m_command_buffer->begin(begin_info);
}
```

`submit_and_wait()`은 커맨드 버퍼의 작업이 끝난 뒤 리셋하여 커맨드 풀로 반환합니다.

```cpp
void CommandBlock::submit_and_wait() {
  if (!m_command_buffer) { return; }

  // end recording and submit.
  m_command_buffer->end();
  auto submit_info = vk::SubmitInfo2KHR{};
  auto const command_buffer_info =
    vk::CommandBufferSubmitInfo{*m_command_buffer};
  submit_info.setCommandBufferInfos(command_buffer_info);
  auto fence = m_device.createFenceUnique({});
  m_queue.submit2(submit_info, *fence);

  // wait for submit fence to be signaled.
  static constexpr auto timeout_v =
    static_cast<std::uint64_t>(std::chrono::nanoseconds(30s).count());
  auto const result = m_device.waitForFences(*fence, vk::True, timeout_v);
  if (result != vk::Result::eSuccess) {
    std::println(stderr, "Failed to submit Command Buffer");
  }
  // free the command buffer.
  m_command_buffer.reset();
}
```

## 멀티쓰레딩 시 고려사항

"`submit_and_wait()`를 호출할 때마다 메인 쓰레드를 블록하는 대신, 멀티쓰레드로 CommandBlock을 구현하는 편이 낫지 않을까?" 라고 생각하실 수 있습니다. 맞습니다! 하지만 멀티 쓰레딩을 위해서는 몇 가지 추가 작업이 필요합니다. 각 쓰레드가 고유한 커맨드 풀이 필요합니다. CommandBlock마다 하나의 고유한 커맨드 풀을 사용하며, 임계 영역을 뮤텍스로 보호하는 것, 스왑체인으로부터 이미지를 가져오고 표시하는 작업 등 큐에 대한 모든 작업은 동기화되어야 합니다. 이를 위해 `vk::Queue`객체와 해당 큐를 보호하는 `std::mutex` 포인터 혹은 참조를 보관하는 `class Queue`를 설계할 수 있으며, 큐 제출은 이 클래스를 통해 수행할 것입니다. 이렇게 하면 각 에셋을 불러오는 쓰레드가 고유한 커맨드 풀을 사용하면서도 큐 제출은 안전하게 이루어질 수 있습니다. `VmaAllocator`는 내부적으로 동기화되어 있으며, 빌드 시 동기화를 끌 수도 있지만 기본적으로는 멀티쓰레드 환경에서 안전하게 사용할 수 있습니다.

멀티쓰레드 렌더링을 구현하려면 각 쓰레드에서 Secondary 커맨드 버퍼에 렌더링 명령을 기록한 후, 이를 Primary 커맨드 버퍼로 모아 `RenderSync`에서 실행할 수 있습니다. 다만 수백개의 드로우콜 정도는 하나의 쓰레드에서 처리하는 편이 더 빠르기 때문에, 수천개의 비싼 드로우콜과 수십개의 렌더패스를 사용하지 않는 한 사용할 일이 없습니다.
