# Vulkan Instance

本教學不會在建置階段透過 SDK 連結 Vulkan，而是在執行期載入 Vulkan，這需要做一些調整：

1. 在 CMake ext target 中定義 `VK_NO_PROTOTYPES`，讓 API 的函式宣告變成函示指標
2. 在 `app.cpp` 的全域範圍新增這行：`VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE`
3. 在初始化前與初始化過程中呼叫 `VULKAN_HPP_DEFAULT_DISPATCHER.init()`

在 Vulkan 中，第一步是建立一個 [Instance](https://docs.vulkan.org/spec/latest/chapters/initialization.html#initialization-instances)，這讓我們能列舉可用的實體裝置（GPU），並建立邏輯裝置

由於我們需要 Vulkan 1.3，因此會將這個需求存為常數，以方便後續引用：

```cpp
namespace {
constexpr auto vk_version_v = VK_MAKE_VERSION(1, 3, 0);
} // namespace
```

現在讓我們於 `App` 中建立一個新的成員函式 `create_instance()`，並在 `run()` 中於 `create_window()` 之後呼叫它。 這會在初始化 dispatcher 後，檢查載入器是否符合版本需求：

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

我們之後會需要 WSI 的 instance 擴充功能，對此 GLFW 為我們提供了方便的介面。 讓我們在 `window.hpp/cpp` 中加入一個輔助函式：

```cpp
auto glfw::instance_extensions() -> std::span<char const* const> {
  auto count = std::uint32_t{};
  auto const* extensions = glfwGetRequiredInstanceExtensions(&count);
  return {extensions, static_cast<std::size_t>(count)};
}
```

接著繼續建立 Instance，先建立一個 `vk::ApplicationInfo` 物件並填入其內容：

```cpp
auto app_info = vk::ApplicationInfo{};
app_info.setPApplicationName("Learn Vulkan").setApiVersion(vk_version_v);
```

並建立一個 `vk::InstanceCreateInfo` 並填入其內容：

```cpp
auto instance_ci = vk::InstanceCreateInfo{};
// need WSI instance extensions here (platform-specific Swapchains).
auto const extensions = glfw::instance_extensions();
instance_ci.setPApplicationInfo(&app_info).setPEnabledExtensionNames(
  extensions);
```

在 `m_window`「之後」新增一個 `vk::UniqueInstance` 成員，因為它必須在 GLFW 結束前銷毀。 建立它並針對它初始化 dispatcher：

```cpp
glfw::Window m_window{};
vk::UniqueInstance m_instance{};

// ...
// initialize the dispatcher against the created Instance.
m_instance = vk::createInstanceUnique(instance_ci);
VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);
```

請確保 VkConfig 是在啟用了 validation layer 的情況下執行的，接著再去偵錯/執行你的應用程式。 若啟用了「Information」等級的載入器訊息，此時你應該會在主控台上看到不少的輸出，像是已載入的 validation layer 的資訊、實體裝置（physical device）及其 ICD 的列舉結果等

如果在日誌中看不到以下這行或類似的內容，請重新檢查你 Vulkan Configurator 的設定與 `PATH`：

```
INFO | LAYER:      Insert instance layer "VK_LAYER_KHRONOS_validation"
```

另外例如，如果應用程式或載入器無法找到 `libVkLayer_khronos_validation.so` / `VkLayer_khronos_validation.dll`，你會看到類似以下的訊息：

```
INFO | LAYER:   Requested layer "VK_LAYER_KHRONOS_validation" failed to load.
```

至此，你已經成功初始化了 Vulkan Instance，恭喜你！

> Wayland 使用者注意：要看到視窗還有很長一段路要走，目前 VkConfig 與驗證層的日誌就是你唯一的回饋來源
