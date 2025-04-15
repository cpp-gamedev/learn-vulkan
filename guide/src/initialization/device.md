# Vulkan Device

A [Vulkan Device](https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#devsandqueues-devices) is a logical instance of a Physical Device, and will the primary interface for everything Vulkan now onwards. [Vulkan Queues](https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#devsandqueues-queues) are owned by the Device, we will need one from the queue family stored in the `Gpu` to submit recorded command buffers. We also need to explicitly declare all features we want to use, eg [Dynamic Rendering](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_dynamic_rendering.html) and [Synchronization2](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_synchronization2.html).

Setup a `vk::QueueCreateInfo` object:

```cpp
auto queue_ci = vk::DeviceQueueCreateInfo{};
// since we use only one queue, it has the entire priority range, ie, 1.0
static constexpr auto queue_priorities_v = std::array{1.0f};
queue_ci.setQueueFamilyIndex(m_gpu.queue_family)
  .setQueueCount(1)
  .setQueuePriorities(queue_priorities_v);
```

Setup the core device features:

```cpp
// nice-to-have optional core features, enable if GPU supports them.
auto enabled_features = vk::PhysicalDeviceFeatures{};
enabled_features.fillModeNonSolid = m_gpu.features.fillModeNonSolid;
enabled_features.wideLines = m_gpu.features.wideLines;
enabled_features.samplerAnisotropy = m_gpu.features.samplerAnisotropy;
enabled_features.sampleRateShading = m_gpu.features.sampleRateShading;
```

Setup the additional features, using `setPNext()` to chain them:

```cpp
// extra features that need to be explicitly enabled.
auto sync_feature = vk::PhysicalDeviceSynchronization2Features{vk::True};
auto dynamic_rendering_feature =
  vk::PhysicalDeviceDynamicRenderingFeatures{vk::True};
// sync_feature.pNext => dynamic_rendering_feature,
// and later device_ci.pNext => sync_feature.
// this is 'pNext chaining'.
sync_feature.setPNext(&dynamic_rendering_feature);
```

Setup a `vk::DeviceCreateInfo` object:

```cpp
auto device_ci = vk::DeviceCreateInfo{};
// we only need one device extension: Swapchain.
static constexpr auto extensions_v =
  std::array{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
device_ci.setPEnabledExtensionNames(extensions_v)
  .setQueueCreateInfos(queue_ci)
  .setPEnabledFeatures(&enabled_features)
  .setPNext(&sync_feature);
```

Declare a `vk::UniqueDevice` member after `m_gpu`, create it, and initialize the dispatcher against it:

```cpp
m_device = m_gpu.device.createDeviceUnique(device_ci);
// initialize the dispatcher against the created Device.
VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);
```

Declare a `vk::Queue` member (order doesn't matter since it's just a handle, the actual Queue is owned by the Device) and initialize it:

```cpp
static constexpr std::uint32_t queue_index_v{0};
m_queue = m_device->getQueue(m_gpu.queue_family, queue_index_v);
```
