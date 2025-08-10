# Vulkan Device

[Vulkan Device](https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#devsandqueues-devices) 是實體裝置（Physical Device）的邏輯實例，它是我們之後操作 Vulkan 的主要介面。 [Vulkan Queues](https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#devsandqueues-queues) 由 Device 所擁有，我們需要從儲存在 `Gpu` 中的佇列家族（queue family）中取得一個 Device，來提交已記錄的指令緩衝區（command buffer）。 我們還需要顯式地宣告所有想要使用的功能，例如 [Dynamic Rendering](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_dynamic_rendering.html) 與 [Synchronization2](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_synchronization2.html)

接下來讓我們來設定各項功能。 首先要設定 `vk::DeviceQueueCreateInfo` 物件：

```cpp
auto queue_ci = vk::DeviceQueueCreateInfo{};
// since we use only one queue, it has the entire priority range, ie, 1.0
static constexpr auto queue_priorities_v = std::array{1.0f};
queue_ci.setQueueFamilyIndex(m_gpu.queue_family)
  .setQueueCount(1)
  .setQueuePriorities(queue_priorities_v);
```

接著設定核心功能：

```cpp
// nice-to-have optional core features, enable if GPU supports them.
auto enabled_features = vk::PhysicalDeviceFeatures{};
enabled_features.fillModeNonSolid = m_gpu.features.fillModeNonSolid;
enabled_features.wideLines = m_gpu.features.wideLines;
enabled_features.samplerAnisotropy = m_gpu.features.samplerAnisotropy;
enabled_features.sampleRateShading = m_gpu.features.sampleRateShading;
```

設定額外功能，並用 `setPNext()` 把它們串接起來：

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

建立並設定一個 `vk::DeviceCreateInfo` 物件：

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

在 `m_gpu` 之後宣告一個 `vk::UniqueDevice` 成員，建立它，並針對它初始化 dispatcher：

```cpp
m_device = m_gpu.device.createDeviceUnique(device_ci);
// initialize the dispatcher against the created Device.
VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);
```

宣告一個 `vk::Queue` 成員（順序無所謂，因為它只是個 handle，實際的 Queue 由 Device 擁有），並完成初始化：

```cpp
static constexpr std::uint32_t queue_index_v{0};
m_queue = m_device->getQueue(m_gpu.queue_family, queue_index_v);
```
