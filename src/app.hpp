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

	void main_loop();

	glfw::Window m_window{};
	vk::UniqueInstance m_instance{};
	vk::UniqueSurfaceKHR m_surface{};
	Gpu m_gpu{};
};
} // namespace lvk
