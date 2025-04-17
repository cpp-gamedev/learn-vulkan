# Vulkan Instance

Vulkan을 SDK를 통해 빌드 시점에 링킹하는 대신, 런타임에 동적으로 로드할 것입니다. 이를 위해 몇 가지 조정이 필요합니다.

1. CMake의 ext target에서 `VK_NO_PROTOTYPES`를 정의해, Vulkan API 함수 선언이 함수 포인터로 변환되도록 합니다.
2. `app.cpp`에서 전역에 `VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE`를 추가합니다.
3. 초기화 이전과 초기화 과정 중에 `VULKAN_HPP_DEFAULT_DISPATCHER.init()`을 호출합니다.

Vulkan에서 가장 먼저 해야할 것은 [Instance](https://docs.vulkan.org/spec/latest/chapters/initialization.html#initialization-instances)를 생성하는 것입니다. 이는 물리 디바이스(GPU)의 목록을 가져오거나, 논리 디바이스를 생성할 수 있습니다.
Instance

Vulkan 1.3 버전을 필요로 하므로, 이를 상수로 정의해 쉽게 참조할 수 있도록 합니다.

```cpp
namespace {
constexpr auto vk_version_v = VK_MAKE_VERSION(1, 3, 0);
} // namespace
```

`App`클래스에 새로운 멤버 함수 `create_instance()`를 추가하고, `run()`함수 내에서 `create_window()`호출 직후에 이를 호출하세요. 디스패쳐를 초기화한 후에는, 로더가 요구하는 Vulkan 버전을 충족하는지 확인합니다.

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

WSI 관련 인스턴스 확장이 필요하며, 이는 GLFW가 제공해줍니다. 관련 확장을 받아오는 함수를 `window.hpp/cpp`에 추가합니다.

```cpp
auto glfw::instance_extensions() -> std::span<char const* const> {
  auto count = std::uint32_t{};
  auto const* extensions = glfwGetRequiredInstanceExtensions(&count);
  return {extensions, static_cast<std::size_t>(count)};
}
```

인스턴스 생성에 이어서, `vk::ApplicationInfo`객체를 생성하고 필요한 정보를 채워 넣습니다.

```cpp
auto app_info = vk::ApplicationInfo{};
app_info.setPApplicationName("Learn Vulkan").setApiVersion(vk_version_v);
```

`vk::InstanceCreateInfo` 구조체를 생성하고 초기화합니다.

```cpp
auto instance_ci = vk::InstanceCreateInfo{};
// need WSI instance extensions here (platform-specific Swapchains).
auto const extensions = glfw::instance_extensions();
instance_ci.setPApplicationInfo(&app_info).setPEnabledExtensionNames(
  extensions);
```

`m_window`멤버 다음에 `vk::UniqueInstance` 멤버를 추가하세요. 이는 GLFW 종료 전에 반드시 파괴되어야 하므로 순서를 지키는 것이 중요합니다. 인스턴스를 생성한 뒤, 해당 인스턴스를 기반으로 디스패쳐를 다시 초기화합니다.

```cpp
glfw::Window m_window{};
vk::UniqueInstance m_instance{};

// ...
// initialize the dispatcher against the created Instance.
m_instance = vk::createInstanceUnique(instance_ci);
VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);
```

VkConfig가 검증 레이어가 활성화된 상태인지 확인한 후, 애플리케이션을 디버그 혹은 실행하세요. 만약 로더 메시지의 "Information" 레벨 로그가 활성화되어 있다면, 이 시점에서 로드된 레이어 정보, 물리 디바이스 및 해당 ICD의 열거 등 다양한 콘솔 출력이 보일 것입니다.

해당 메시지 또는 유사한 로그가 보이지 않는다면, Vulkan Configurator 설정과 환경변수 `PATH`를 다시 확인해보세요.

```
INFO | LAYER:      Insert instance layer "VK_LAYER_KHRONOS_validation"
```

예를 들어, `libVkLayer_khronos_validation.so` / `VkLayer_khronos_validation.dll`이 애플리케이션 / 로더에 보이지 않는다면 다음과 같은 메시지가 출력될 수 있습니다

```
INFO | LAYER:   Requested layer "VK_LAYER_KHRONOS_validation" failed to load.
```

축하합니다! 성공적으로 Vulkan Instance를 초기화하였습니다.

> Wayland 사용자의 경우 아직 창이 보이지 않을 것이기 때문에 현재로서는 VkConfig/Validation 유일한 확인 수단입니다.