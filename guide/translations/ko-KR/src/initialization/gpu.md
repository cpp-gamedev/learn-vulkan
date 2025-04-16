# Vulkan Physical Device

A [Physical Device](https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#devsandqueues-physical-device-enumeration) represents a single complete implementation of Vulkan, for our intents and purposes a single GPU. (It could also be eg a software renderer like Mesa/lavapipe.) Some machines may have multiple Physical Devices available, like laptops with dual-GPUs. We need to select the one we want to use, given our constraints:

1. Vulkan 1.3 must be supported
1. Vulkan Swapchains must be supported
1. A Vulkan Queue that supports Graphics and Transfer operations must be available
1. It must be able to present to the previously created Vulkan Surface
1. (Optional) Prefer discrete GPUs

We wrap the actual Physical Device and a few other useful objects into `struct Gpu`. Since it will be accompanied by a hefty utility function, we put it in its own hpp/cpp files, and move the `vk_version_v` constant to this new header:

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

The implementation:

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

Finally, add a `Gpu` member in `App` and initialize it after `create_surface()`:

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
