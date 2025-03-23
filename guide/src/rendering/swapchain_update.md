# Swapchain Update

Swapchain acquire/present operations can have various results. We constrain ourselves to the following:

- `eSuccess`: all good
- `eSuboptimalKHR`: also all good (not an error, and this is unlikely to occur on a desktop)
- `eErrorOutOfDateKHR`: Swapchain needs to be recreated
- Any other `vk::Result`: fatal/unexpected error

Expressing as a helper function in `swapchain.cpp`:

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

We also want to return the Image, Image View, and size upon successful acquisition of the underlying Swapchain Image. Wrapping those in a `struct`:

```cpp
struct RenderTarget {
	vk::Image image{};
	vk::ImageView image_view{};
	vk::Extent2D extent{};
};
```

VulkanHPP's primary API throws if the `vk::Result` corresponds to an error (based on the spec). `eErrorOutOfDateKHR` is technically an error, but it's quite possible to get it when the framebuffer size doesn't match the Swapchain size. To avoid having to deal with exceptions here, we use the alternate API for the acquire and present calls (overloads distinguished by pointer arguments and/or out parameters, and returning a `vk::Result`).

Implementing the acquire operation:

```cpp
auto Swapchain::acquire_next_image(vk::Semaphore const to_signal)
	-> std::optional<RenderTarget> {
	assert(!m_image_index);
	static constexpr auto timeout_v = std::numeric_limits<std::uint64_t>::max();
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

Similarly, present:

```cpp
auto Swapchain::present(vk::Queue const queue, vk::Semaphore const to_wait)
	-> bool {
	auto const image_index = static_cast<std::uint32_t>(m_image_index.value());
	auto present_info = vk::PresentInfoKHR{};
	present_info.setSwapchains(*m_swapchain)
		.setImageIndices(image_index)
		.setWaitSemaphores(to_wait);
	auto const result = queue.presentKHR(&present_info);
	m_image_index.reset();
	return !needs_recreation(result);
}
```

It is the responsibility of the user (`class App`) to recreate the Swapchain on receiving `std::nullopt` / `false` return values for either operation. Users will also need to transition the layouts of the returned images between acquire and present operations. Add a helper to assist in that process, and extract the Image Subresource Range out as a common constant:

```cpp
constexpr auto subresource_range_v = [] {
	auto ret = vk::ImageSubresourceRange{};
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
