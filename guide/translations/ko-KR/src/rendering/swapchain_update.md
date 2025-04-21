# 스왑체인 업데이트

Swapchain acquire/present operations can have various results. We constrain ourselves to the following:
스왑체인에서 이미지를 받아오고 표시하는 작업은 다양한 결과를 반환할 수 있습니다. 우리는 다음과 같은 경우에 한정하여 처리합니다.

- `eSuccess` : 문제가 없습니다.
- `eSuboptimalKHR` : 역시 문제가 없습니다(에러는 아니며, 데스크탑 환경에서는 드물게 발생합니다).
- `eErrorOutOfDateKHR` : 스왑체인을 재생성해야 합니다.
- 그 외의 모든 `vk::Result` : 치명적이거나 예기치 않은 오류입니다.

`swapchain.cpp`에 함수를 생성합시다.

```cpp
auto needs_recreation(vk::Result const result) -> bool {
  switch (result) {
  case vk::Result::eSuccess:
  case vk::Result::eSuboptimalKHR: return false;
  case vk::Result::eErrorOutOfDateKHR: return true;
  default: break;
  }
  throw std::runtime_error{"Swapchain Error"};
}
```

스왑체인으로부터 이미지를 성공적으로 받아오면 이미지와 이미지 뷰, 그리고 크기를 반환해야 합니다. 이를 `struct`로 감싸겠습니다.

```cpp
struct RenderTarget {
  vk::Image image{};
  vk::ImageView image_view{};
  vk::Extent2D extent{};
};
```

VulkanHPP의 기본 API는 `vk::Result`가 오류에 해당되면 (사양에 따라) 예외를 던집니다. `eErrorOutOfDateKHR`은 기술적으로는 오류이지만, 프레임버퍼와 스왑체인의 크기가 일치하지 않을 때 일어날 수 있습니다. 이러한 예외 처리를 피하기 위해, 우리는 포인터 인자 또는 출력 인자를 사용하는 오버로드 버전 API를 사용하여 `vk::Result`를 직접 반환받는 방식으로 대체하겠습니다.

이미지를 가져오는 함수를 작성하겠습니다.

```cpp
auto Swapchain::acquire_next_image(vk::Semaphore const to_signal)
  -> std::optional<RenderTarget> {
  assert(!m_image_index);
  static constexpr auto timeout_v = std::numeric_limits<std::uint64_t>::max();
  // avoid VulkanHPP ErrorOutOfDateKHR exceptions by using alternate API that
  // returns a Result.
  auto image_index = std::uint32_t{};
  auto const result = m_device.acquireNextImageKHR(
    *m_swapchain, timeout_v, to_signal, {}, &image_index);
  if (needs_recreation(result)) { return {}; }

  m_image_index = static_cast<std::size_t>(image_index);
  return RenderTarget{
    .image = m_images.at(*m_image_index),
    .image_view = *m_image_views.at(*m_image_index),
    .extent = m_ci.imageExtent,
  };
}
```

표시하는 함수도 마찬가지입니다.

```cpp
auto Swapchain::present(vk::Queue const queue, vk::Semaphore const to_wait)
  -> bool {
  auto const image_index = static_cast<std::uint32_t>(m_image_index.value());
  auto present_info = vk::PresentInfoKHR{};
  present_info.setSwapchains(*m_swapchain)
    .setImageIndices(image_index)
    .setWaitSemaphores(to_wait);
  // avoid VulkanHPP ErrorOutOfDateKHR exceptions by using alternate API.
  auto const result = queue.presentKHR(&present_info);
  m_image_index.reset();
  return !needs_recreation(result);
}
```

각 작업에서 `std::nullopt` 혹은 `false`가 반환될 경우, 스왑체인을 재생성하는 것은 사용자(`class App`)의 책임입니다. 사용자는 또한 받아오는 것과 표시하는 작업 사이에 이미지의 레이아웃을 전환해야 합니다. 이 과정을 돕는 함수를 추가하고 공통 상수로 사용할 수 있도록 ImageSubresourceRange 분리해 정의합니다.

```cpp
constexpr auto subresource_range_v = [] {
  auto ret = vk::ImageSubresourceRange{};
  // this is a color image with 1 layer and 1 mip-level (the default).
  ret.setAspectMask(vk::ImageAspectFlagBits::eColor)
    .setLayerCount(1)
    .setLevelCount(1);
  return ret;
}();

// ...
auto Swapchain::base_barrier() const -> vk::ImageMemoryBarrier2 {
  // fill up the parts common to all barriers.
  auto ret = vk::ImageMemoryBarrier2{};
  ret.setImage(m_images.at(m_image_index.value()))
    .setSubresourceRange(subresource_range_v)
    .setSrcQueueFamilyIndex(m_gpu.queue_family)
    .setDstQueueFamilyIndex(m_gpu.queue_family);
  return ret;
}
```
