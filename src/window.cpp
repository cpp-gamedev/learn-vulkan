#include <window.hpp>
#include <cstdint>
#include <print>
#include <stdexcept>

namespace lvk {
namespace glfw {
void Deleter::operator()(GLFWwindow* window) const noexcept {
	glfwDestroyWindow(window);
	glfwTerminate();
}
} // namespace glfw

auto glfw::create_window(glm::ivec2 const size, char const* title) -> Window {
	static auto const on_error = [](int const code, char const* description) {
		std::println(stderr, "[GLFW] Error {}: {}", code, description);
	};
	glfwSetErrorCallback(on_error);
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error{"Failed to initialize GLFW"};
	}
	// check for Vulkan support.
	if (glfwVulkanSupported() != GLFW_TRUE) {
		throw std::runtime_error{"Vulkan not supported"};
	}
	auto ret = Window{};
	// tell GLFW that we don't want an OpenGL context.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	ret.reset(glfwCreateWindow(size.x, size.y, title, nullptr, nullptr));
	if (!ret) { throw std::runtime_error{"Failed to create GLFW Window"}; }
	return ret;
}

auto glfw::instance_extensions() -> std::span<char const* const> {
	auto count = std::uint32_t{};
	auto const* extensions = glfwGetRequiredInstanceExtensions(&count);
	return {extensions, static_cast<std::size_t>(count)};
}

auto glfw::create_surface(GLFWwindow* window, vk::Instance const instance)
	-> vk::UniqueSurfaceKHR {
	VkSurfaceKHR ret{};
	auto const result =
		glfwCreateWindowSurface(instance, window, nullptr, &ret);
	if (result != VK_SUCCESS || ret == VkSurfaceKHR{}) {
		throw std::runtime_error{"Failed to create Vulkan Surface"};
	}
	return vk::UniqueSurfaceKHR{ret, instance};
}

auto glfw::framebuffer_size(GLFWwindow* window) -> glm::ivec2 {
	auto ret = glm::ivec2{};
	glfwGetFramebufferSize(window, &ret.x, &ret.y);
	return ret;
}
} // namespace lvk
