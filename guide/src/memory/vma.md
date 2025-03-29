# Vulkan Memory Allocator

VMA has full CMake support, but it is also a single-header library that requires users to "instantiate" it in a single translation unit. Isolating that into a wrapper library to minimize warning pollution etc, we create our own `vma::vma` target that compiles this source file:

```cpp
// vk_mem_alloc.cpp
#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>
```

Unlike VulkanHPP, VMA's interface is C only, thus we shall use our `Scoped` class template to wrap objects in RAII types. The first thing we need is a `VmaAllocator`, which is similar to a `vk::Device` or `GLFWwindow*`:

```cpp
// vma.hpp
namespace lvk::vma {
struct Deleter {
  void operator()(VmaAllocator allocator) const noexcept;
};

using Allocator = Scoped<VmaAllocator, Deleter>;

[[nodiscard]] auto create_allocator(vk::Instance instance,
                                    vk::PhysicalDevice physical_device,
                                    vk::Device device) -> Allocator;
} // namespace lvk::vma

// vma.cpp
void vma::Deleter::operator()(VmaAllocator allocator) const noexcept {
  vmaDestroyAllocator(allocator);
}

auto vma::create_allocator(vk::Instance const instance,
                           vk::PhysicalDevice const physical_device,
                           vk::Device const device) -> Allocator {
  auto const& dispatcher = VULKAN_HPP_DEFAULT_DISPATCHER;
  // need to zero initialize C structs, unlike VulkanHPP.
  auto vma_vk_funcs = VmaVulkanFunctions{};
  vma_vk_funcs.vkGetInstanceProcAddr = dispatcher.vkGetInstanceProcAddr;
  vma_vk_funcs.vkGetDeviceProcAddr = dispatcher.vkGetDeviceProcAddr;

  auto allocator_ci = VmaAllocatorCreateInfo{};
  allocator_ci.physicalDevice = physical_device;
  allocator_ci.device = device;
  allocator_ci.pVulkanFunctions = &vma_vk_funcs;
  allocator_ci.instance = instance;
  VmaAllocator ret{};
  auto const result = vmaCreateAllocator(&allocator_ci, &ret);
  if (result == VK_SUCCESS) { return ret; }

  throw std::runtime_error{"Failed to create Vulkan Memory Allocator"};
}
```

`App` stores and creates a `vma::Allocator` object:

```cpp
// ...
vma::Allocator m_allocator{}; // anywhere between m_device and m_shader.

// ...
void App::create_allocator() {
  m_allocator = vma::create_allocator(*m_instance, m_gpu.device, *m_device);
}
```
