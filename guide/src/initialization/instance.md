# Vulkan Instance

Instead of linking to Vulkan (via the SDK) at build-time, we will load Vulkan at runtime. This requires a few adjustments:

1. In the CMake ext target `VK_NO_PROTOTYPES` is defined, which turns API function declarations into function pointers
1. In `app.cpp` this line is added to the global scope: `VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE`
1. Before and during initialization `VULKAN_HPP_DEFAULT_DISPATCHER.init()` is called

The first thing to do in Vulkan is to create an [Instance](https://docs.vulkan.org/spec/latest/chapters/initialization.html#initialization-instances), which will enable enumeration of physical devices (GPUs) and creation of a logical device.

Since we require Vulkan 1.3, store that in a constant to be easily referenced:

```cpp
namespace {
constexpr auto vk_version_v = VK_MAKE_VERSION(1, 3, 0);
} // namespace
```

In `App`, create  a new member function `create_instance()` and call it after `create_window()` in `run()`. After initializing the dispatcher, check that the loader meets the version requirement:

```cpp
void App::create_instance() {
  // initialize the dispatcher without any arguments.
  VULKAN_HPP_DEFAULT_DISPATCHER.init();
  auto const loader_version = vk::enumerateInstanceVersion();
  if (loader_version < vk_version_v) {
    throw std::runtime_error{"Loader does not support Vulkan 1.3"};
  }
}
```

We will need the WSI instance extensions, which GLFW conveniently provides for us. Add a helper function in `window.hpp/cpp`:

```cpp
auto glfw::instance_extensions() -> std::span<char const* const> {
  auto count = std::uint32_t{};
  auto const* extensions = glfwGetRequiredInstanceExtensions(&count);
  return {extensions, static_cast<std::size_t>(count)};
}
```

Continuing with instance creation, create a `vk::ApplicationInfo` object and fill it up:

```cpp
auto app_info = vk::ApplicationInfo{};
app_info.setPApplicationName("Learn Vulkan").setApiVersion(vk_version_v);
```

Create a `vk::InstanceCreateInfo` object and fill it up:

```cpp
auto instance_ci = vk::InstanceCreateInfo{};
// need WSI instance extensions here (platform-specific Swapchains).
auto const extensions = glfw::instance_extensions();
instance_ci.setPApplicationInfo(&app_info).setPEnabledExtensionNames(
  extensions);
```

Add a `vk::UniqueInstance` member _after_ `m_window`: this must be destroyed before terminating GLFW. Create it, and initialize the dispatcher against it:

```cpp
glfw::Window m_window{};
vk::UniqueInstance m_instance{};

// ...
// initialize the dispatcher against the created Instance.
m_instance = vk::createInstanceUnique(instance_ci);
VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);
```

Make sure VkConfig is running with validation layers enabled, and debug/run the app. If "Information" level loader messages are enabled, you should see quite a bit of console output at this point: information about layers being loaded, physical devices and their ICDs being enumerated, etc.

If this line or equivalent is not visible in the logs, re-check your Vulkan Configurator setup and `PATH`:

```
INFO | LAYER:      Insert instance layer "VK_LAYER_KHRONOS_validation"
```

For instance, if `libVkLayer_khronos_validation.so` / `VkLayer_khronos_validation.dll` is not visible to the app / loader, you'll see a line similar to:

```
INFO | LAYER:   Requested layer "VK_LAYER_KHRONOS_validation" failed to load.
```

Congratulations, you have successfully initialized a Vulkan Instance!

> Wayland users: seeing the window is still a long way off, these VkConfig/validation logs are your only feedback for now.
