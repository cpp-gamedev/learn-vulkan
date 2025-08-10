# Vulkan Surface

Vulkan 與平台無關，它會透過 [`VK_KHR_surface` 擴充功能](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_surface.html)與 WSI 介接。 [Surface](https://docs.vulkan.org/guide/latest/wsi.html#_surface) 讓我們能透過呈現引擎（presentation engine）把影像顯示在視窗上

在 `window.hpp/cpp` 中再加入一個輔助函式：

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

在 `m_instance` 之後替 `App` 新增一個 `vk::UniqueSurfaceKHR` 的成員，並建立該 Surface：

```cpp
void App::create_surface() {
  m_surface = glfw::create_surface(m_window.get(), *m_instance);
}
```
