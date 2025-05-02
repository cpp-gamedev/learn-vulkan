# GLFW Window

창 생성과 이벤트 처리를 위해 GLFW 3.4를 사용할 것입니다. 모든 외부 의존 라이브러리들은 `ext/CMakeLists.txt`에 구성되어 빌드 트리에 추가됩니다. `GLFW_INCLUDE_VULKAN`은 GLFW의 Vulkan 관련 기능(WSI, Window System Integration)을 활성화하기 위해 GLFW를 사용하는 측에서 정의되어야 합니다. GLFW 3.4는 Linux에서 Wayland를 지원하며, 기본적으로 X11과 Wayland 모두를 위한 백엔드를 빌드합니다. 따라서 빌드를 성공적으로 진행하려면 두 플랫폼 모두에 필요한 패키지와 일부 Wayland 및 CMake 의존성이 필요합니다. 특정 백엔드를 사용하고자 할 경우, 런타임에서 `GLFW_PLATFORM`을 통해 지정할 수 있습니다.

Vulkan-GLFW 애플리케이션에서 다중 창을 사용할 수는 있지만, 이 가이드에서는 다루지 않습니다. 여기서는 GLFW 라이브러리와 단일 창을 하나의 단위로 보고 함께 초기화하고 해제하는 방식으로 구성합니다. GLFW는 구조를 알 수 없는(Opaque) 포인터 `GLFWwindow*`를 반환하므로, 이를 `std::unique_ptr`과 커스텀 파괴자를 사용해 캡슐화하는 것이 적절합니다.

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

GLFW는 전체 화면이나 테두리 없는 창을 만들 수 있지만, 이 가이드에서는 일반 창을 사용합니다. 창을 생성하지 못하면 이후 작업이 불가능하기 때문에, 실패한 경우에는 모두 치명적인 예외를 발생시키도록 되어 있습니다.

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

`App`은 이제 `glfw::Window`를 멤버로 저장해 사용자가 창을 닫을 때 까지 `run()`에서 이를 사용할 수 있습니다. 아직은 창에 아무것도 그릴 수 없지만, 이는 앞으로의 과정을 시작하는 첫 단계입니다.

이를 private 멤버로 선언합니다.

```cpp
private:
  glfw::Window m_window{};
```

각 작업을 캡슐화하는 몇 가지 private 멤버 함수를 추가합니다.

```cpp
void create_window();

void main_loop();
```

이를 구현하고 `run()`에서 호출합니다.

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

> Wayland에서는 아직 창이 보이지 않을 수 있습니다. 창은 애플리케이션이 프레임버퍼를 렌더링한 후에야 화면에 표시됩니다.
