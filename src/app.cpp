#include <app.hpp>
#include <print>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace lvk {
void App::run() {
	create_window();
	create_instance();
	create_surface();
	select_gpu();
	create_device();

	main_loop();
}

void App::create_window() {
	m_window = glfw::create_window({1280, 720}, "Learn Vulkan");
}

void App::create_instance() {
	VULKAN_HPP_DEFAULT_DISPATCHER.init();
	auto const loader_version = vk::enumerateInstanceVersion();
	if (loader_version < vk_version_v) {
		throw std::runtime_error{"Loader does not support Vulkan 1.3"};
	}

	auto app_info = vk::ApplicationInfo{};
	app_info.setPApplicationName("Learn Vulkan").setApiVersion(vk_version_v);

	auto instance_ci = vk::InstanceCreateInfo{};
	auto const extensions = glfw::instance_extensions();
	instance_ci.setPApplicationInfo(&app_info).setPEnabledExtensionNames(
		extensions);

	m_instance = vk::createInstanceUnique(instance_ci);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);
}

void App::create_surface() {
	m_surface = glfw::create_surface(m_window.get(), *m_instance);
}

void App::select_gpu() {
	m_gpu = get_suitable_gpu(*m_instance, *m_surface);
	std::println("[lvk] Using GPU: {}",
				 std::string_view{m_gpu.properties.deviceName});
}

void App::create_device() {
	auto queue_ci = vk::DeviceQueueCreateInfo{};
	// since we use only one queue, it has the entire priority range, ie, 1.0
	static constexpr auto queue_priorities_v = std::array{1.0f};
	queue_ci.setQueueFamilyIndex(m_gpu.queue_family)
		.setQueueCount(1)
		.setQueuePriorities(queue_priorities_v);

	auto enabled_features = vk::PhysicalDeviceFeatures{};
	enabled_features.fillModeNonSolid = m_gpu.features.fillModeNonSolid;
	enabled_features.wideLines = m_gpu.features.wideLines;
	enabled_features.samplerAnisotropy = m_gpu.features.samplerAnisotropy;
	enabled_features.sampleRateShading = m_gpu.features.sampleRateShading;

	auto sync_feature = vk::PhysicalDeviceSynchronization2Features{vk::True};
	auto dynamic_rendering_feature =
		vk::PhysicalDeviceDynamicRenderingFeatures{vk::True};
	sync_feature.setPNext(&dynamic_rendering_feature);

	auto device_ci = vk::DeviceCreateInfo{};
	static constexpr auto extensions_v =
		std::array{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	device_ci.setPEnabledExtensionNames(extensions_v)
		.setQueueCreateInfos(queue_ci)
		.setPEnabledFeatures(&enabled_features)
		.setPNext(&sync_feature);

	m_device = m_gpu.device.createDeviceUnique(device_ci);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);
	static constexpr std::uint32_t queue_index_v{0};
	m_queue = m_device->getQueue(m_gpu.queue_family, queue_index_v);

	// m_device_block.get() = *m_device;
}

void App::main_loop() {
	while (glfwWindowShouldClose(m_window.get()) == GLFW_FALSE) {
		glfwPollEvents();
	}
}
} // namespace lvk
