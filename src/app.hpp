#pragma once
#include <gpu.hpp>
#include <scoped_waiter.hpp>
#include <swapchain.hpp>
#include <vulkan/vulkan.hpp>
#include <window.hpp>

namespace lvk {
class App {
  public:
	void run();

  private:
	void create_window();
	void create_instance();
	void create_surface();
	void select_gpu();
	void create_device();
	void create_swapchain();

	void main_loop();

	// the order of these RAII members is crucially important.
	glfw::Window m_window{};
	vk::UniqueInstance m_instance{};
	vk::UniqueSurfaceKHR m_surface{};
	Gpu m_gpu{}; // not an RAII member.
	vk::UniqueDevice m_device{};
	vk::Queue m_queue{}; // not an RAII member.

	std::optional<Swapchain> m_swapchain{};

	// waiter must be the last member to ensure it blocks until device is idle
	// before other members get destroyed.
	ScopedWaiter m_waiter{};
};
} // namespace lvk
