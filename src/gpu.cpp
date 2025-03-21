#include <gpu.hpp>
#include <algorithm>
#include <ranges>
#include <stdexcept>

auto lvk::get_suitable_gpu(vk::Instance const instance,
						   vk::SurfaceKHR const surface) -> Gpu {
	auto const supports_swapchain = [](Gpu const& gpu) {
		static constexpr std::string_view name_v =
			VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		static constexpr auto is_swapchain =
			[](vk::ExtensionProperties const& properties) {
				return properties.extensionName.data() == name_v;
			};
		auto const properties = gpu.device.enumerateDeviceExtensionProperties();
		auto const it = std::ranges::find_if(properties, is_swapchain);
		return it != properties.end();
	};

	auto const set_queue_family = [](Gpu& out_gpu) {
		static constexpr auto queue_flags_v =
			vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer;
		for (auto const [index, family] :
			 std::views::enumerate(out_gpu.device.getQueueFamilyProperties())) {
			if ((family.queueFlags & queue_flags_v) == queue_flags_v) {
				out_gpu.queue_family = static_cast<std::uint32_t>(index);
				return true;
			}
		}
		return false;
	};

	auto const can_present = [surface](Gpu const& gpu) {
		return gpu.device.getSurfaceSupportKHR(gpu.queue_family, surface) ==
			   vk::True;
	};

	auto fallback = Gpu{};
	for (auto const& device : instance.enumeratePhysicalDevices()) {
		auto gpu = Gpu{.device = device, .properties = device.getProperties()};
		if (gpu.properties.apiVersion < vk_version_v) { continue; }
		if (!supports_swapchain(gpu)) { continue; }
		if (!set_queue_family(gpu)) { continue; }
		if (!can_present(gpu)) { continue; }
		gpu.features = gpu.device.getFeatures();
		if (gpu.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			return gpu;
		}
		// keep iterating in case we find a Discrete Gpu later.
		fallback = gpu;
	}
	if (fallback.device) { return fallback; }

	throw std::runtime_error{"No suitable Vulkan Physical Devices"};
}
