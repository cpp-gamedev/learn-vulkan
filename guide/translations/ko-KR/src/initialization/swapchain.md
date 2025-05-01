# 스왑체인

[스왑체인](https://docs.vulkan.org/guide/latest/wsi.html#_swapchain)은 Surface와 연결된, 화면에 표시 가능한 이미지들의 배열입니다. 이는 애플리케이션과 플랫폼의 프레젠테이션 엔진 사이를 이어주는 다리 역할을 합니다. 스왑체인은 메인 루프에서 이미지를 받아오고 화면에 표시하기 위해 지속적으로 사용됩니다. 스왑체인 생성에 실패하는 것은 치명적인 오류이므로, 그 생성 과정은 초기화 단계에 포함됩니다.

스왑체인을 우리가 정의한 `class Swapchain`으로 감쌀 것입니다. 이 클래스는 스왑체인이 소유한 이미지의 복사본을 저장하고, 각 이미지에 맞는 이미지 뷰를 생성하고 소유합니다. 스왑체인은 프레임 버퍼 크기가 변경되거나, acquire/present 작업이 `vk::ErrorOutOfDataKHR`를 반환하는 경우처럼, 메인 루프 중에 재생성이 필요할 수 있습니다. 이를 `recreate()` 함수로 캡슐화하여 초기화 시점에도 간단히 호출할 수 있도록 하겠습니다.

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

## 정적 스왑체인 속성

이미지 크기나 개수와 같은 몇몇 스왑체인 생성 파라미터는 surface capabilities에 따라 결정되며, 이는 런타임 중 변경될 수 있습니다. 나머지 설정은 생성자에서 처리할 수 있으며, 이때 필요한 [Surface Format](https://registry.khronos.org/vulkan/specs/latest/man/html/VkSurfaceFormatKHR.html)을 얻기 위한 함수가 필요합니다.

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

sRGB 포맷이 선호되는 이유는 화면의 색상 공간이 sRGB이기 때문입니다. 이는 Vulkan의 기본 [색상 영역](https://registry.khronos.org/vulkan/specs/latest/man/html/VkColorSpaceKHR.html)이 `vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear` 하나 뿐이라는 사실에서 알 수 있습니다. 이 값은 sRGB 색상 공간의 이미지를 지원함을 나타냅니다.

이제 생성자를 구현할 수 있습니다.

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

## 스왑체인 재생성

스왑체인 생성 파라미터의 제약은 [Surface Capabilities](https://registry.khronos.org/vulkan/specs/latest/man/html/VkSurfaceCapabilitiesKHR.html)에 지정됩니다. 사양에 따라 함수 두 개와 상수 하나를 추가하겠습니다.

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

트리플 버퍼링을 설정할 수 있도록 최소한 세 개의 이미지가 필요합니다. Surface의 `maxImageCount < 3`일 가능성도 있지만, 그럴 일은 거의 없습니다. 오히려 `minImageCount > 3`인 경우가 더 흔합니다.

Vulkan 이미지의 차원은 양수여야만 하므로, 전달받은 프레임 버퍼 크기가 유효하지 않다면 재생성을 생략합니다. 예를 들어 윈도우가 최소화된 경우 이런 상황이 발생할 수 있습니다(이때는 창이 복원될 때까지 렌더링이 일시 중지됩니다).

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

재생성에 성공한 후에는 이미지와 이미지 뷰 벡터를 채워야 합니다. 이미지의 경우, 매번 새로 반환된 벡터를 멤버 변수에 할당하지 않기 위해 약간 더 복잡한 방식으로 접근합니다.

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

이미지 뷰 생성은 비교적 간단합니다.

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

이제 이 함수를 `recreate()`에서 `return true` 직전에 호출하고 로그를 찍어봅시다.

```cpp
populate_images();
create_image_views();

size = get_size();
std::println("[lvk] Swapchain [{}x{}]", size.x, size.y);
return true;
```

> 창 크기를 계속해서 바꿀 경우 로그가 많이 찍힐 수 있습니다(특히 Linux에서).

프레임 버퍼 크기를 얻기 위해 `window.hpp/cpp`에 함수를 추가합니다.

```cpp
auto glfw::framebuffer_size(GLFWwindow* window) -> glm::ivec2 {
  auto ret = glm::ivec2{};
  glfwGetFramebufferSize(window, &ret.x, &ret.y);
  return ret;
}
```

마지막으로, `std::optional<Swapchain>` 멤버를 `App`의 `m_device` 이후에 추가하고, 생성 함수를 추가하여 `create_device()` 이후에 이를 호출합니다.

```cpp
std::optional<Swapchain> m_swapchain{};

// ...
void App::create_swapchain() {
  auto const size = glfw::framebuffer_size(m_window.get());
  m_swapchain.emplace(*m_device, m_gpu, *m_surface, size);
}
```
