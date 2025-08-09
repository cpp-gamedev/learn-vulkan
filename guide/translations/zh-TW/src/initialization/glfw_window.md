# GLFW Window

本教學使用 GLFW（3.4）來處理視窗與相關事件。 這個函式庫和其他外部相依套件一樣，會在 `ext/CMakeLists.txt` 中設定並加入建置樹（build tree）。 對使用者而言，我們會定義 `GLFW_INCLUDE_VULKAN`，以啟用 GLFW 的 Vulkan 相關功能，這些功能被稱為「視窗系統整合（Window System Integration, WSI）」

GLFW 3.4 在 Linux 上支援 Wayland，並且預設會同時建置 X11 與 Wayland 兩種後端。 因此，你必須同時安裝[兩個平台的開發套件](https://www.glfw.org/docs/latest/compile_guide.html#compile_deps_wayland)（以及一些其他 Wayland/CMake 相依套件），才能順利完成設定與建置。 如果需要，你也可以在執行階段透過 `GLFW_PLATFORM` 來指定要使用的特定後端

雖然 Vulkan-GLFW 的應用程式可以同時擁有多個視窗，但這不在本書探討範圍內。 對我們而言，GLFW（函式庫）與單一視窗被視為一個整體單元，它們會一起初始化並一起銷毀。 由於 GLFW 會回傳一個不透明指標（opaque pointer） `GLFWwindow*`，因此你可以將它封裝在帶有自訂刪除器的 `std::unique_ptr` 中來管理

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

GLFW 可以建立全螢幕或無邊框的視窗，本教學會使用帶有邊框與標題列的標準視窗。 由於如果無法建立視窗，就無法再進行任何有意義的操作，因此其他所有分支都會丟出致命例外（會在 main 中被捕捉）

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

接著 `App` 會儲存一個 `glfw::Window`，並在 `run()` 中持續輪詢它，直到使用者關閉視窗為止。 雖然暫時還無法在視窗上繪製任何內容，但這是邁向該目標的第一步

首先將它宣告為私有成員：

```cpp
private:
  glfw::Window m_window{};
```

接著加上一些私有的成員函式來將相關的操作封裝起來：

```cpp
void create_window();

void main_loop();
```

最後補上函式定義，並在 `run()` 中呼叫它們：

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

> 如果是在 Wayland 上，此時還看不到視窗。 Wayland 需要應用程式將 framebuffer 給它後，視窗才會顯示出來
