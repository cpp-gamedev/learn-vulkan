# Swapchain

A [Vulkan Swapchain](https://registry.khronos.org/vulkan/specs/latest/man/html/VkSwapchainKHR.html) is an array of presentable images associated with a Surface, which acts as a bridge between the application and the platform's presentation engine (compositor / display engine). The Swapchain will be continually used in the main loop to acquire and present images. Since failing to create a Swapchain is a fatal error, its creation is part of the initialiation section.

We shall wrap the Vulkan Swapchain into our own `class Swapchain`. It will also store the a copy of the Images owned by the Vulkan Swapchain, and create (and own) Image Views for each Image. The Vulkan Swapchain may need to be recreated in the main loop, eg when the framebuffer size changes, or an acquire/present operation returns `vk::ErrorOutOfDateKHR`. This will be encapsulated in a `recreate()` function which can simply be called during initialization as well.

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

Some Swapchain creation parameters like the image extent (size) and count depend on the surface capabilities, which can change during runtime. We can setup the rest in the constructor, for which we need a helper function to obtain a desired [Surface Format](https://registry.khronos.org/vulkan/specs/latest/man/html/VkSurfaceFormatKHR.html):

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

An sRGB format is preferred because that is what the screen's color space is in. This is indicated by the fact that the only core [Color Format](https://registry.khronos.org/vulkan/specs/latest/man/html/VkColorSpaceKHR.html) is `vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear`, which specifies support for the images in sRGB color space.

The constructor can now be implemented:

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

The constraints on Swapchain creation parameters are specified by [Surface Capabilities](https://registry.khronos.org/vulkan/specs/latest/man/html/VkSurfaceCapabilitiesKHR.html). Based on the spec we add two helper functions and a constant:

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

We want at least three images in order to be have the option to set up triple buffering. While it's possible for a Surface to have `maxImageCount < 3`, it is quite unlikely. It is in fact much more likely for `minImageCount > 3`.

The dimensions of Vulkan Images must be positive, so if the incoming framebuffer size is not, we skip the attempt to recreate. This can happen eg on Windows when the window is minimized. (Until it is restored, rendering will basically be paused.)

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

After successful recreation we want to fill up those vectors of images and views. For the images we use a more verbose approach to avoid having to assign the member vector to a newly returned one every time:

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

Creation of the views is fairly straightforward:

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

We can now call these functions in `recreate()`, before `return true`, and add a log for some feedback:

```cpp
populate_images();
create_image_views();

size = get_size();
std::println("[lvk] Swapchain [{}x{}]", size.x, size.y);
return true;
```

> The log can get a bit noisy on incessant resizing (especially on Linux).

To get the framebuffer size, add a helper function in `window.hpp/cpp`:

```cpp
auto glfw::framebuffer_size(GLFWwindow* window) -> glm::ivec2 {
  auto ret = glm::ivec2{};
  glfwGetFramebufferSize(window, &ret.x, &ret.y);
  return ret;
}
```

Finally, add a `std::optional<Swapchain>` member to `App` after `m_device`, add the create function, and call it after `create_device()`:

```cpp
std::optional<Swapchain> m_swapchain{};

// ...
void App::create_swapchain() {
  auto const size = glfw::framebuffer_size(m_window.get());
  m_swapchain.emplace(*m_device, m_gpu, *m_surface, size);
}
```
