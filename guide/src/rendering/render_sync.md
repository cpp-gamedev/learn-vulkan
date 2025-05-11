# Render Sync

Create a new header `resource_buffering.hpp`:

```cpp
// Number of virtual frames.
inline constexpr std::size_t buffering_v{2};

// Alias for N-buffered resources.
template <typename Type>
using Buffered = std::array<Type, buffering_v>;
```

Add a private `struct RenderSync` to `App`:

```cpp
struct RenderSync {
  // signaled when Swapchain image has been acquired.
  vk::UniqueSemaphore draw{};
  // signaled with present Semaphore, waited on before next render.
  vk::UniqueFence drawn{};
  // used to record rendering commands.
  vk::CommandBuffer command_buffer{};
};
```

Add the new members associated with the Swapchain loop:

```cpp
// command pool for all render Command Buffers.
vk::UniqueCommandPool m_render_cmd_pool{};
// Sync and Command Buffer for virtual frames.
Buffered<RenderSync> m_render_sync{};
// Current virtual frame index.
std::size_t m_frame_index{};
```

Add, implement, and call the create function:

```cpp
void App::create_render_sync() {
  // Command Buffers are 'allocated' from a Command Pool (which is 'created'
  // like all other Vulkan objects so far). We can allocate all the buffers
  // from a single pool here.
  auto command_pool_ci = vk::CommandPoolCreateInfo{};
  // this flag enables resetting the command buffer for re-recording (unlike a
  // single-time submit scenario).
  command_pool_ci.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
    .setQueueFamilyIndex(m_gpu.queue_family);
  m_render_cmd_pool = m_device->createCommandPoolUnique(command_pool_ci);

  auto command_buffer_ai = vk::CommandBufferAllocateInfo{};
  command_buffer_ai.setCommandPool(*m_render_cmd_pool)
    .setCommandBufferCount(static_cast<std::uint32_t>(resource_buffering_v))
    .setLevel(vk::CommandBufferLevel::ePrimary);
  auto const command_buffers =
    m_device->allocateCommandBuffers(command_buffer_ai);
  assert(command_buffers.size() == m_render_sync.size());

  // we create Render Fences as pre-signaled so that on the first render for
  // each virtual frame we don't wait on their fences (since there's nothing
  // to wait for yet).
  static constexpr auto fence_create_info_v =
    vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled};
  for (auto [sync, command_buffer] :
     std::views::zip(m_render_sync, command_buffers)) {
    sync.command_buffer = command_buffer;
    sync.draw = m_device->createSemaphoreUnique({});
    sync.drawn = m_device->createFenceUnique(fence_create_info_v);
  }
}
```
