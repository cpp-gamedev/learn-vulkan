#pragma once
#include <gpu.hpp>
#include <resource_buffering.hpp>
#include <scoped_waiter.hpp>
#include <swapchain.hpp>
#include <vulkan/vulkan.hpp>
#include <window.hpp>

namespace lvk {
class App {
  public:
	void run();

  private:
	struct RenderSync {
		// signalled when Swapchain image has been acquired.
		vk::UniqueSemaphore draw{};
		// signalled when image is ready to be presented.
		vk::UniqueSemaphore present{};
		// signalled with present Semaphore, waited on before next render.
		vk::UniqueFence drawn{};
		// used to record rendering commands.
		vk::CommandBuffer command_buffer{};
	};

	void create_window();
	void create_instance();
	void create_surface();
	void select_gpu();
	void create_device();
	void create_swapchain();
	void create_render_sync();

	void main_loop();

	glfw::Window m_window{};
	vk::UniqueInstance m_instance{};
	vk::UniqueSurfaceKHR m_surface{};
	Gpu m_gpu{};
	vk::UniqueDevice m_device{};
	vk::Queue m_queue{};

	std::optional<Swapchain> m_swapchain{};
	// command pool for all render Command Buffers.
	vk::UniqueCommandPool m_render_cmd_pool{};
	// Sync and Command Buffer for virtual frames.
	Buffered<RenderSync> m_render_sync{};
	// Current virtual frame index.
	std::size_t m_frame_index{};

	ScopedWaiter m_waiter{};
};
} // namespace lvk
