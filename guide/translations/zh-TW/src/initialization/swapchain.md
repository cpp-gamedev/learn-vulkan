# Swapchain

[Vulkan Swapchain](https://docs.vulkan.org/guide/latest/wsi.html#_swapchain) 是與某個 Surface 關聯的一組可呈現影像（presentable images），它充當應用程式與平台呈現引擎（compositor / display engine）之間的橋樑。 Swapchain 會在主迴圈中持續用來取得與呈現影像。 若 Swapchain 的建立失敗了將會是致命錯誤，因此它的建立屬於初始化流程的一部分。

我們會將 Vulkan Swapchain 封裝成自訂的 `class Swapchain`。 它還會儲存 Vulkan Swapchain 擁有的影像副本，並為每個影像建立（並擁有）對應的 Image View。 有時你會需要在主迴圈中重新建立 Vulkan Swapchain，例如當 framebuffer 大小改變，或 acquire/present 操作回傳 `vk::ErrorOutOfDateKHR` 時。 這會被封裝在一個 `recreate()` 函式中，在初始化時也可以直接呼叫該函式

```cpp
// swapchain.hpp
class Swapchain {
 public:
  explicit Swapchain(vk::Device device, Gpu const& gpu,
                     vk::SurfaceKHR surface, glm::ivec2 size);

  auto recreate(glm::ivec2 size) -> bool;

  [[nodiscard]] auto get_size() const -> glm::ivec2 {
    return {m_ci.imageExtent.width, m_ci.imageExtent.height};
  }

 private:
  void populate_images();
  void create_image_views();

  vk::Device m_device{};
  Gpu m_gpu{};

  vk::SwapchainCreateInfoKHR m_ci{};
  vk::UniqueSwapchainKHR m_swapchain{};
  std::vector<vk::Image> m_images{};
  std::vector<vk::UniqueImageView> m_image_views{};
};

// swapchain.cpp
Swapchain::Swapchain(vk::Device const device, Gpu const& gpu,
           vk::SurfaceKHR const surface, glm::ivec2 const size)
  : m_device(device), m_gpu(gpu) {}
```

## Static Swapchain Properties

有些 Swapchain 會根據 Surface 的特性（capabilities）來建立如影像範圍大小與張數之類的參數，而這些特性在執行期間可能會變化。 我們可以在建構子中設定其他固定參數，並用一個輔助函式來取得所需的 [Surface Format](https://registry.khronos.org/vulkan/specs/latest/man/html/VkSurfaceFormatKHR.html)：

```cpp
constexpr auto srgb_formats_v = std::array{
  vk::Format::eR8G8B8A8Srgb,
  vk::Format::eB8G8R8A8Srgb,
};

// returns a SurfaceFormat with SrgbNonLinear color space and an sRGB format.
[[nodiscard]] constexpr auto
get_surface_format(std::span<vk::SurfaceFormatKHR const> supported)
  -> vk::SurfaceFormatKHR {
  for (auto const desired : srgb_formats_v) {
    auto const is_match = [desired](vk::SurfaceFormatKHR const& in) {
      return in.format == desired &&
           in.colorSpace ==
             vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
    };
    auto const it = std::ranges::find_if(supported, is_match);
    if (it == supported.end()) { continue; }
    return *it;
  }
  return supported.front();
}
```

之所以偏好 sRGB 格式，是因為 sRGB 是顯示器的標準色彩空間。 這一點可由唯一的核心[色彩空間](https://registry.khronos.org/vulkan/specs/latest/man/html/VkColorSpaceKHR.html) `vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear` 得知（該設定表示支援 sRGB 色彩空間影像）

現在可以實作 `Swapchain` 的建構子了：

```cpp
auto const surface_format =
  get_surface_format(m_gpu.device.getSurfaceFormatsKHR(surface));
m_ci.setSurface(surface)
  .setImageFormat(surface_format.format)
  .setImageColorSpace(surface_format.colorSpace)
  .setImageArrayLayers(1)
  // Swapchain images will be used as color attachments (render targets).
  .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
  // eFifo is guaranteed to be supported.
  .setPresentMode(vk::PresentModeKHR::eFifo);
if (!recreate(size)) {
  throw std::runtime_error{"Failed to create Vulkan Swapchain"};
}
```

## Swapchain Recreation

[Surface Capabilities](https://registry.khronos.org/vulkan/specs/latest/man/html/VkSurfaceCapabilitiesKHR.html) 規範了建立 Swapchain 時的參數限制。 根據規範，我們會新增兩個輔助函式與一個常數：

```cpp
constexpr std::uint32_t min_images_v{3};

// returns currentExtent if specified, else clamped size.
[[nodiscard]] constexpr auto
get_image_extent(vk::SurfaceCapabilitiesKHR const& capabilities,
                 glm::uvec2 const size) -> vk::Extent2D {
  constexpr auto limitless_v = 0xffffffff;
  if (capabilities.currentExtent.width < limitless_v &&
    capabilities.currentExtent.height < limitless_v) {
    return capabilities.currentExtent;
  }
  auto const x = std::clamp(size.x, capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);
  auto const y = std::clamp(size.y, capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);
  return vk::Extent2D{x, y};
}

[[nodiscard]] constexpr auto
get_image_count(vk::SurfaceCapabilitiesKHR const& capabilities)
  -> std::uint32_t {
  if (capabilities.maxImageCount < capabilities.minImageCount) {
    return std::max(min_images_v, capabilities.minImageCount);
  }
  return std::clamp(min_images_v, capabilities.minImageCount,
            capabilities.maxImageCount);
}
```

我們希望至少有三張影像，這樣才能夠啟用三重緩衝（triple buffering）。 雖然理論上某些 Surface 可能會出現 `maxImageCount < 3` 的情況，但這種狀況相當罕見，大多數的情況都是 `minImageCount > 3`

Vulkan Image 的大小必須為正數，因此如果輸入的 framebuffer 大小不是正數，就會略過重建的嘗試。 這種情況在 Windows 上視窗被最小化時可能發生（在視窗恢復前，算繪基本上會暫停）

```cpp
auto Swapchain::recreate(glm::ivec2 size) -> bool {
  // Image sizes must be positive.
  if (size.x <= 0 || size.y <= 0) { return false; }

  auto const capabilities =
    m_gpu.device.getSurfaceCapabilitiesKHR(m_ci.surface);
  m_ci.setImageExtent(get_image_extent(capabilities, size))
    .setMinImageCount(get_image_count(capabilities))
    .setOldSwapchain(m_swapchain ? *m_swapchain : vk::SwapchainKHR{})
    .setQueueFamilyIndices(m_gpu.queue_family);
  assert(m_ci.imageExtent.width > 0 && m_ci.imageExtent.height > 0 &&
       m_ci.minImageCount >= min_images_v);

  // wait for the device to be idle before destroying the current swapchain.
  m_device.waitIdle();
  m_swapchain = m_device.createSwapchainKHRUnique(m_ci);

  return true;
}
```

在成功重建之後，我們要把影像與 View 的那些 vector 填好。 針對影像部分，我們使用較穩妥的作法，避免每次都要指派一個新的 vector 給它：

```cpp
void require_success(vk::Result const result, char const* error_msg) {
  if (result != vk::Result::eSuccess) { throw std::runtime_error{error_msg}; }
}

// ...
void Swapchain::populate_images() {
  // we use the more verbose two-call API to avoid assigning m_images to a new
  // vector on every call.
  auto image_count = std::uint32_t{};
  auto result =
    m_device.getSwapchainImagesKHR(*m_swapchain, &image_count, nullptr);
  require_success(result, "Failed to get Swapchain Images");

  m_images.resize(image_count);
  result = m_device.getSwapchainImagesKHR(*m_swapchain, &image_count,
                                          m_images.data());
  require_success(result, "Failed to get Swapchain Images");
}
```

建立 Image View 的流程相對直接：

```cpp
void Swapchain::create_image_views() {
  auto subresource_range = vk::ImageSubresourceRange{};
  // this is a color image with 1 layer and 1 mip-level (the default).
  subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
    .setLayerCount(1)
    .setLevelCount(1);
  auto image_view_ci = vk::ImageViewCreateInfo{};
  // set common parameters here (everything except the Image).
  image_view_ci.setViewType(vk::ImageViewType::e2D)
    .setFormat(m_ci.imageFormat)
    .setSubresourceRange(subresource_range);
  m_image_views.clear();
  m_image_views.reserve(m_images.size());
  for (auto const image : m_images) {
    image_view_ci.setImage(image);
    m_image_views.push_back(m_device.createImageViewUnique(image_view_ci));
  }
}
```

現在我們可以在 `recreate()` 中於 `return true` 之前呼叫這些函式，並加上一行日誌作為回饋：

```cpp
populate_images();
create_image_views();

size = get_size();
std::println("[lvk] Swapchain [{}x{}]", size.x, size.y);
return true;
```

> 在不斷調整視窗大小時（特別是在 Linux 上），這行日誌可能會過於頻繁地出現

為了取得 framebuffer 大小，在 `window.hpp/cpp` 中新增一個輔助函式：

```cpp
auto glfw::framebuffer_size(GLFWwindow* window) -> glm::ivec2 {
  auto ret = glm::ivec2{};
  glfwGetFramebufferSize(window, &ret.x, &ret.y);
  return ret;
}
```

最後，在 `App` 中的 `m_device` 後面新增一個 `std::optional<Swapchain>` 成員，新增建立函式，並在 `create_device()` 之後呼叫它：

```cpp
std::optional<Swapchain> m_swapchain{};

// ...
void App::create_swapchain() {
  auto const size = glfw::framebuffer_size(m_window.get());
  m_swapchain.emplace(*m_device, m_gpu, *m_surface, size);
}
```
