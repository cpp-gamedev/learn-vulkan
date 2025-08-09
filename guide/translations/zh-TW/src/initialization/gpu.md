# Vulkan Physical Device

[Physical Device](https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#devsandqueues-physical-device-enumeration) 代表一個完整的 Vulkan 實作； 就我們的用途而言，可以把它視為一張 GPU（它也可能是像 Mesa/lavapipe 這樣的軟體算繪器）。 有些機器上可能同時存在多個 Physical Device，例如配備雙顯卡的筆電。 我們需要在下列限制條件下，挑選要使用的是哪一個：

1. 必須支援 Vulkan 1.3
2. 必須支援 Vulkan Swapchain
3. 必須有支援 Graphics 與 Transfer 作業的 Vulkan Queue
4. 必須能呈現（present）先前建立的 Vulkan Surface
5. （Optional）優先選擇獨立顯示卡（discrete GPU）

我們會把實際的 Physical Device 以及其他幾個實用物件包裝成 `struct Gpu`。 由於它會搭配一個相當龐大的工具函式，我們會把它放到獨立的 hpp/cpp 檔案中，並把 `vk_version_v` 這個常數移到新的標頭檔裡：

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

函式定義：

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

最後，在 `App` 中加入 `Gpu` 成員，並在 `create_surface()` 之後初始化它：

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
