#pragma once
#include <dear_imgui.hpp>
#include <gpu.hpp>
#include <resource_buffering.hpp>
#include <scoped_waiter.hpp>
#include <shader_program.hpp>
#include <swapchain.hpp>
#include <window.hpp>
#include <filesystem>

namespace lvk {
namespace fs = std::filesystem;

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
	void create_imgui();
	void create_shader();

	[[nodiscard]] auto asset_path(std::string_view uri) const -> fs::path;

	void main_loop();

	auto acquire_render_target() -> bool;
	auto begin_frame() -> vk::CommandBuffer;
	void transition_for_render(vk::CommandBuffer command_buffer) const;
	void render(vk::CommandBuffer command_buffer);
	void transition_for_present(vk::CommandBuffer command_buffer) const;
	void submit_and_present();

	// ImGui code goes here.
	void inspect();
	// Issue draw calls here.
	void draw(vk::CommandBuffer command_buffer) const;

	fs::path m_assets_dir{};

	// the order of these RAII members is crucially important.
	glfw::Window m_window{};
	vk::UniqueInstance m_instance{};
	vk::UniqueSurfaceKHR m_surface{};
	Gpu m_gpu{}; // not an RAII member.
	vk::UniqueDevice m_device{};
	vk::Queue m_queue{}; // not an RAII member.

	std::optional<Swapchain> m_swapchain{};
	// command pool for all render Command Buffers.
	vk::UniqueCommandPool m_render_cmd_pool{};
	// Sync and Command Buffer for virtual frames.
	Buffered<RenderSync> m_render_sync{};
	// Current virtual frame index.
	std::size_t m_frame_index{};

	std::optional<DearImGui> m_imgui{};

	std::optional<ShaderProgram> m_shader{};

	glm::ivec2 m_framebuffer_size{};
	std::optional<RenderTarget> m_render_target{};
	bool m_wireframe{};

	// waiter must be the last member to ensure it blocks until device is idle
	// before other members get destroyed.
	ScopedWaiter m_waiter{};
};
} // namespace lvk
