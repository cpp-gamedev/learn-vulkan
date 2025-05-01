# Vulkan 물리 디바이스

[물리 디바이스](https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#devsandqueues-physical-device-enumeration)는 Vulkan의 완전한 구현체를 나타내며, 일반적으로 하나의 GPU를 의미합니다(단, Mesa/lavapipe와 같은 소프트웨어 렌더러일 수도 있습니다). 여러 GPU가 장착된 랩탑과 같은 일부 기기에서는 여러 개의 물리 디바이스가 존재할 수 있습니다. 이 중에서 다음 조건을 만족하는 것을 하나 선택해야 합니다.

1. Vulkan 1.3을 지원해야 합니다.
2. 스왑체인을 지원해야 합니다.
3. Graphics와 Transfer작업을 지원하는 Vulkan Queue가 존재해야 합니다.
4. 이전에 생성한 Vulkan Surface로 출력(present)할 수 있어야 합니다.
5. (선택 사항) 외장 GPU를 우선적으로 고려합니다.

실제 물리 디바이스와 몇 가지 유용한 객체들을 `struct Gpu`로 묶겠습니다. 많은 유틸리티 함수가 함께 정의될 것이므로 이를 별도의 hpp, cpp파일에 구현하고 기존의 `vk_version_v` 상수를 이 새로운 헤더로 옮기겠습니다.

```cpp
constexpr auto vk_version_v = VK_MAKE_VERSION(1, 3, 0);

struct Gpu {
  vk::PhysicalDevice device{};
  vk::PhysicalDeviceProperties properties{};
  vk::PhysicalDeviceFeatures features{};
  std::uint32_t queue_family{};
};

[[nodiscard]] auto get_suitable_gpu(vk::Instance instance,
                                    vk::SurfaceKHR surface) -> Gpu;
```

아래는 구현부입니다.

```cpp
auto lvk::get_suitable_gpu(vk::Instance const instance,
               vk::SurfaceKHR const surface) -> Gpu {
  auto const supports_swapchain = [](Gpu const& gpu) {
    static constexpr std::string_view name_v =
      VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    static constexpr auto is_swapchain =
      [](vk::ExtensionProperties const& properties) {
        return properties.extensionName.data() == name_v;
      };
    auto const properties = gpu.device.enumerateDeviceExtensionProperties();
    auto const it = std::ranges::find_if(properties, is_swapchain);
    return it != properties.end();
  };

  auto const set_queue_family = [](Gpu& out_gpu) {
    static constexpr auto queue_flags_v =
      vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer;
    for (auto const [index, family] :
       std::views::enumerate(out_gpu.device.getQueueFamilyProperties())) {
      if ((family.queueFlags & queue_flags_v) == queue_flags_v) {
        out_gpu.queue_family = static_cast<std::uint32_t>(index);
        return true;
      }
    }
    return false;
  };

  auto const can_present = [surface](Gpu const& gpu) {
    return gpu.device.getSurfaceSupportKHR(gpu.queue_family, surface) ==
         vk::True;
  };

  auto fallback = Gpu{};
  for (auto const& device : instance.enumeratePhysicalDevices()) {
    auto gpu = Gpu{.device = device, .properties = device.getProperties()};
    if (gpu.properties.apiVersion < vk_version_v) { continue; }
    if (!supports_swapchain(gpu)) { continue; }
    if (!set_queue_family(gpu)) { continue; }
    if (!can_present(gpu)) { continue; }
    gpu.features = gpu.device.getFeatures();
    if (gpu.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      return gpu;
    }
    // keep iterating in case we find a Discrete Gpu later.
    fallback = gpu;
  }
  if (fallback.device) { return fallback; }

  throw std::runtime_error{"No suitable Vulkan Physical Devices"};
}
```

마지막으로 `Gpu` 멤버를 `App`에 추가하고 `create_surface()`이후에 초기화합니다.

```cpp
create_surface();
select_gpu();

// ...
void App::select_gpu() {
  m_gpu = get_suitable_gpu(*m_instance, *m_surface);
  std::println("Using GPU: {}",
               std::string_view{m_gpu.properties.deviceName});
}
```
