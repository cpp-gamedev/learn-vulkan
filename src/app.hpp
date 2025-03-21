#pragma once
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

	void main_loop();

	glfw::Window m_window{};
	vk::UniqueInstance m_instance{};
	vk::UniqueSurfaceKHR m_surface{};
};
} // namespace lvk
