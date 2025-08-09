# Vulkan 디바이스

[디바이스](https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#devsandqueues-devices)는 Physical Device의 논리적 인스턴스이며, 이후의 모든 Vulkan 작업에서 주요 인터페이스 역할을 하게 됩니다. [큐](https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#devsandqueues-queues)는 디바이스가 소유하는 것으로, `Gpu` 구조체에 저장된 큐 패밀리에서 하나를 가져와 기록된 커맨드 버퍼를 제출하는 데 사용할 것입니다. 또한 사용하기를 원하는 [Dynamic Rendering](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_dynamic_rendering.html) 과 [Synchronization2](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_synchronization2.html)같은 기능들을 명시적으로 선언해야 합니다.

`vk::DeviceQueueCreateInfo`객체를 설정합시다.

```cpp
auto queue_ci = vk::DeviceQueueCreateInfo{};
// since we use only one queue, it has the entire priority range, ie, 1.0
static constexpr auto queue_priorities_v = std::array{1.0f};
queue_ci.setQueueFamilyIndex(m_gpu.queue_family)
  .setQueueCount(1)
  .setQueuePriorities(queue_priorities_v);
```

핵심 디바이스 기능을 설정합니다.

```cpp
// nice-to-have optional core features, enable if GPU supports them.
auto enabled_features = vk::PhysicalDeviceFeatures{};
enabled_features.fillModeNonSolid = m_gpu.features.fillModeNonSolid;
enabled_features.wideLines = m_gpu.features.wideLines;
enabled_features.samplerAnisotropy = m_gpu.features.samplerAnisotropy;
enabled_features.sampleRateShading = m_gpu.features.sampleRateShading;
```

추가 기능을 설정하기 위해 `setPNext()`를 사용해 묶습니다.

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

`vk::DeviceCreateInfo` 구조체를 설정합니다.

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

`vk::UniqueDevice` 멤버를 `m_gpu` 이후에 선언하고, 이를 생성한 다음 디스패쳐를 해당 디바이스로 다시 초기화합니다.

```cpp
m_device = m_gpu.device.createDeviceUnique(device_ci);
// initialize the dispatcher against the created Device.
VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);
```

`vk::Queue` 멤버도 선언하고 초기화합니다(순서는 중요하지 않습니다. 이는 단순한 핸들이며 실제 큐는 디바이스가 관리하기 때문입니다).

```cpp
static constexpr std::uint32_t queue_index_v{0};
m_queue = m_device->getQueue(m_gpu.queue_family, queue_index_v);
```
