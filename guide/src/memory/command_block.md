# Command Block

Long-lived vertex buffers perform better when backed by Device memory, especially for 3D meshes. Data is transferred to device buffers in two steps: 

1. Allocate a host buffer and copy the data to its mapped memory
1. Allocate a device buffer, record a Buffer Copy operation and submit it

The second step requires a command buffer and queue submission (_and_ waiting for the submitted work to complete). Encapsulate this behavior into a class, it will also be used for creating images:

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

The constructor takes an existing command pool created for such ad-hoc allocations, and the queue for submission later. This way it can be passed around after creation and used by other code.

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

`submit_and_wait()` resets the unique command buffer at the end, to free it from its command pool:

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

## Multithreading considerations

Instead of blocking the main thread on every Command Block's `submit_and_wait()`, you might be wondering if command block usage could be multithreaded. The answer is yes! But with some extra work: each thread will require its own command pool - just using one owned (unique) pool per Command Block (with no need to free the buffer) is a good starting point. All queue operations need to be synchronized, ie a critical section protected by a mutex. This includes Swapchain acquire/present calls, and Queue submissions. A `class Queue` value type that stores a copy of the `vk::Queue` and a pointer/reference to its `std::mutex` - and wraps the submit call - can be passed to command blocks. Just this much will enable asynchronous asset loading etc, as each loading thread will use its own command pool, and queue submissions all around will be critical sections. `VmaAllocator` is internally synchronized (can be disabled at build time), so performing allocations through the same allocator on multiple threads is safe.

For multi-threaded rendering, use a Secondary command buffer per thread to record rendering commands, accumulate and execute them in the main (Primary) command buffer currently in `RenderSync`. This is not particularly helpful unless you have thousands of expensive draw calls and dozens of render passes, as recording even a hundred draws will likely be faster on a single thread.
