#include <app.hpp>
#include <print>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace lvk {
namespace {
constexpr auto vk_version_v = VK_MAKE_VERSION(1, 3, 0);
} // namespace

void App::run() {
	create_window();
	create_instance();
	create_surface();

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

void App::main_loop() {
	while (glfwWindowShouldClose(m_window.get()) == GLFW_FALSE) {
		glfwPollEvents();
	}
}
} // namespace lvk
