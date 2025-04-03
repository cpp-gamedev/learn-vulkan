# Vulkan Surface

Being platform agnostic, Vulkan interfaces with the WSI via the [`VK_KHR_surface` extension](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_surface.html). A [Surface](https://docs.vulkan.org/guide/latest/wsi.html#_surface) enables displaying images on the window through the presentation engine.

Add another helper function in `window.hpp/cpp`:

```cpp
auto glfw::create_surface(GLFWwindow* window, vk::Instance const instance)
  -> vk::UniqueSurfaceKHR {
  VkSurfaceKHR ret{};
  auto const result =
    glfwCreateWindowSurface(instance, window, nullptr, &ret);
  if (result != VK_SUCCESS || ret == VkSurfaceKHR{}) {
    throw std::runtime_error{"Failed to create Vulkan Surface"};
  }
  return vk::UniqueSurfaceKHR{ret, instance};
}
```

Add a `vk::UniqueSurfaceKHR` member to `App` after `m_instance`, and create the surface:

```cpp
void App::create_surface() {
  m_surface = glfw::create_surface(m_window.get(), *m_instance);
}
```
