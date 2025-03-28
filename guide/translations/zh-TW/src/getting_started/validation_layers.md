# (中文 WIP) Validation Layers

The area of Vulkan that apps interact with: the loader, is very powerful and flexible. Read more about it [here](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderInterfaceArchitecture.md). Its design enables it to chain API calls through configurable **layers**, eg for overlays, and most importantly for us: [Validation Layers](https://github.com/KhronosGroup/Vulkan-ValidationLayers/blob/main/docs/README.md).

![Vulkan Loader](high_level_loader.png)

As [suggested](https://github.com/KhronosGroup/Vulkan-ValidationLayers/blob/main/docs/khronos_validation_layer.md#vkconfig) by the Khronos Group, we recommend using [Vulkan Configurator (GUI)](https://github.com/LunarG/VulkanTools/tree/main/vkconfig_gui) for validation layers. This application is shipped with the Vulkan SDK, just keep it running while developing Vulkan applications, and ensure it is setup to inject validation layers into all detected applications, with Synchronization Validation enabled. This approach provides a lot of flexibility at runtime, including the ability to have VkConfig break the debugger on encountering an error, and also eliminates the need for validation layer specific code in the applications.

> Note: modify your development (or desktop) environment's `PATH` (or use `LD_LIBRARY_PATH` on supported systems) to make sure the SDK's binaries are visible first.