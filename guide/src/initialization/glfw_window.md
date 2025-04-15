# GLFW Window

We will use GLFW (3.4) for windowing and related events. The library - like all external dependencies - is configured and added to the build tree in `ext/CMakeLists.txt`. `GLFW_INCLUDE_VULKAN` is defined for all consumers, to enable GLFW's Vulkan related functions (known as **Window System Integration (WSI)**). GLFW 3.4 supports Wayland on Linux, and by default it builds backends for both X11 and Wayland. For this reason it will need the development packages for [both platforms](https://www.glfw.org/docs/latest/compile_guide.html#compile_deps_wayland) (and some other Wayland/CMake dependencies) to configure/build successfully. A particular backend can be requested at runtime if desired via `GLFW_PLATFORM`.

Although it is quite feasible to have multiple windows in a Vulkan-GLFW application, that is out of scope for this guide. For our purposes GLFW (the library) and a single window are a monolithic unit - initialized and destroyed together. This can be encapsulated in a `std::unique_ptr` with a custom deleter, especially since GLFW returns an opaque pointer (`GLFWwindow*`).

```cpp
// window.hpp
namespace lvk::glfw {
struct Deleter {
  void operator()(GLFWwindow* window) const noexcept;
};

using Window = std::unique_ptr<GLFWwindow, Deleter>;

// Returns a valid Window if successful, else throws.
[[nodiscard]] auto create_window(glm::ivec2 size, char const* title) -> Window;
} // namespace lvk::glfw

// window.cpp
void Deleter::operator()(GLFWwindow* window) const noexcept {
  glfwDestroyWindow(window);
  glfwTerminate();
}
```

GLFW can create fullscreen and borderless windows, but we will stick to a standard window with decorations. Since we cannot do anything useful if we are unable to create a window, all other branches throw a fatal exception (caught in main).

```cpp
auto glfw::create_window(glm::ivec2 const size, char const* title) -> Window {
  static auto const on_error = [](int const code, char const* description) {
    std::println(stderr, "[GLFW] Error {}: {}", code, description);
  };
  glfwSetErrorCallback(on_error);
  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error{"Failed to initialize GLFW"};
  }
  // check for Vulkan support.
  if (glfwVulkanSupported() != GLFW_TRUE) {
    throw std::runtime_error{"Vulkan not supported"};
  }
  auto ret = Window{};
  // tell GLFW that we don't want an OpenGL context.
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  ret.reset(glfwCreateWindow(size.x, size.y, title, nullptr, nullptr));
  if (!ret) { throw std::runtime_error{"Failed to create GLFW Window"}; }
  return ret;
}
```

`App` can now store a `glfw::Window` and keep polling it in `run()` until it gets closed by the user. We will not be able to draw anything to the window for a while, but this is the first step in that journey.

Declare it as a private member:

```cpp
private:
  glfw::Window m_window{};
```

Add some private member functions to encapsulate each operation:

```cpp
void create_window();

void main_loop();
```

Implement them and call them in `run()`:

```cpp
void App::run() {
  create_window();

  main_loop();
}

void App::create_window() {
  m_window = glfw::create_window({1280, 720}, "Learn Vulkan");
}

void App::main_loop() {
  while (glfwWindowShouldClose(m_window.get()) == GLFW_FALSE) {
    glfwPollEvents();
  }
}
```

> On Wayland you will not even see a window yet: it is only shown _after_ the application presents a framebuffer to it.

