# Vulkan Surface

Vulkan은 플랫폼과 독립적으로 작동하기 위해 [`VK_KHR_surface` 확장](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_surface.html)을 통해 WSI와 상호작용합니다. [Surface](https://docs.vulkan.org/guide/latest/wsi.html#_surface)는 프레젠테이션 엔진을 통해 창에 이미지를 표시할 수 있게 해줍니다.

`window.hpp/cpp`에 또 다른 함수를 추가합시다.

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

`App`에 `vk::UniqueSurfaceKHR`이라는 멤버를 `m_instance` 이후에 추가하고 Surface를 생성합니다.

```cpp
void App::create_surface() {
  m_surface = glfw::create_surface(m_window.get(), *m_instance);
}
```
