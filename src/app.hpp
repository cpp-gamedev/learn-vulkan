#pragma once
#include <gpu.hpp>
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

	void main_loop();

	glfw::Window m_window{};
	vk::UniqueInstance m_instance{};
	vk::UniqueSurfaceKHR m_surface{};
	Gpu m_gpu{};
	vk::UniqueDevice m_device{};
	vk::Queue m_queue{};
};
} // namespace lvk
