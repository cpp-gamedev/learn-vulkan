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

	void main_loop();

	glfw::Window m_window{};
	vk::UniqueInstance m_instance{};
};
} // namespace lvk
