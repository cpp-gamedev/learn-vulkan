# Vulkan Device

A [Vulkan Device](https://registry.khronos.org/vulkan/specs/latest/man/html/VkDevice.html) is a logical instance of a Physical Device, and will the primary interface for everything Vulkan now onwards. [Vulkan Queues](https://registry.khronos.org/vulkan/specs/latest/man/html/VkQueue.html) are owned by the Device, we will need one from the queue family stored in the `Gpu`, to submit recorded command buffers. We also need to explicitly declare all features we want to use, eg [Dynamic Rendering](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_dynamic_rendering.html) and [Synchronization2](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_synchronization2.html).

Setup a `vk::QueueCreateInfo` object:

```cpp
	auto queue_ci = vk::DeviceQueueCreateInfo{};
	// since we use only one queue, it has the entire priority range, ie, 1.0
	static constexpr auto queue_priorities_v = std::array{1.0f};
	queue_ci.setQueueFamilyIndex(m_gpu.queue_family)
		.setQueueCount(1)
		.setQueuePriorities(queue_priorities_v);
```

Setup the core device features:

```cpp
	auto enabled_features = vk::PhysicalDeviceFeatures{};
	enabled_features.fillModeNonSolid = m_gpu.features.fillModeNonSolid;
	enabled_features.wideLines = m_gpu.features.wideLines;
	enabled_features.samplerAnisotropy = m_gpu.features.samplerAnisotropy;
	enabled_features.sampleRateShading = m_gpu.features.sampleRateShading;
```

Setup the additional features, using `setPNext()` to chain them:

```cpp
	auto sync_feature = vk::PhysicalDeviceSynchronization2Features{vk::True};
	auto dynamic_rendering_feature =
		vk::PhysicalDeviceDynamicRenderingFeatures{vk::True};
	sync_feature.setPNext(&dynamic_rendering_feature);
```

Setup a `vk::DeviceCreateInfo` object:

```cpp
	auto device_ci = vk::DeviceCreateInfo{};
	static constexpr auto extensions_v =
		std::array{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	device_ci.setPEnabledExtensionNames(extensions_v)
		.setQueueCreateInfos(queue_ci)
		.setPEnabledFeatures(&enabled_features)
		.setPNext(&sync_feature);
```

Declare a `vk::UniqueDevice` member after `m_gpu`, create it, and initialize the dispatcher against it:

```cpp
	m_device = m_gpu.device.createDeviceUnique(device_ci);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);
```

Declare a `vk::Queue` member (order doesn't matter since it's just a handle, the actual Queue is owned by the Device) and initialize it:

```cpp
	static constexpr std::uint32_t queue_index_v{0};
	m_queue = m_device->getQueue(m_gpu.queue_family, queue_index_v);
```

## ScopedWaiter

A useful abstraction to have is an object that in its destructor waits/blocks until the Device is idle. Being able to do arbitary things on scope exit is useful in general too, but it requires some custom class template like `UniqueResource<Type, Deleter>`. We shall "abuse" `std::unique_ptr<Type, Deleter>` instead: it will not manage the pointer (`Type*`) at all, but instead `Deleter` will call a member function on it (if it isn't null).

Adding this to a new header `scoped_waiter.hpp`:

```cpp
class ScopedWaiter {
  public:
	ScopedWaiter() = default;

	explicit ScopedWaiter(vk::Device const* device) : m_device(device) {}

  private:
	struct Deleter {
		void operator()(vk::Device const* device) const noexcept {
			if (device == nullptr) { return; }
			device->waitIdle();
		}
	};
	std::unique_ptr<vk::Device const, Deleter> m_device{};
};
```

This requires the passed `vk::Device*` to outlive itself, so to be defensive we make `App` be non-moveable and non-copiable, and create a member factory function for waiters:

```cpp
	auto operator=(App&&) = delete;
// ...

	[[nodiscard]] auto create_waiter() const -> ScopedWaiter {
		return ScopedWaiter{&*m_device};
	}
```
