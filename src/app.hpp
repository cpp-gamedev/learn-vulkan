#pragma once
#include <command_block.hpp>
#include <dear_imgui.hpp>
#include <gpu.hpp>
#include <resource_buffering.hpp>
#include <scoped_waiter.hpp>
#include <shader_buffer.hpp>
#include <shader_program.hpp>
#include <swapchain.hpp>
#include <texture.hpp>
#include <transform.hpp>
#include <vma.hpp>
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
	void create_allocator();
	void create_descriptor_pool();
	void create_pipeline_layout();
	void create_shader();
	void create_cmd_block_pool();
	void create_shader_resources();
	void create_descriptor_sets();

	[[nodiscard]] auto asset_path(std::string_view uri) const -> fs::path;
	[[nodiscard]] auto create_command_block() const -> CommandBlock;
	[[nodiscard]] auto allocate_sets() const -> std::vector<vk::DescriptorSet>;

	void main_loop();

	auto acquire_render_target() -> bool;
	auto begin_frame() -> vk::CommandBuffer;
	void transition_for_render(vk::CommandBuffer command_buffer) const;
	void render(vk::CommandBuffer command_buffer);
	void transition_for_present(vk::CommandBuffer command_buffer) const;
	void submit_and_present();

	// ImGui code goes here.
	void inspect();
	void update_view();
	// Issue draw calls here.
	void draw(vk::CommandBuffer command_buffer) const;

	void bind_descriptor_sets(vk::CommandBuffer command_buffer) const;

	fs::path m_assets_dir{};

	// the order of these RAII members is crucially important.
	glfw::Window m_window{};
	vk::UniqueInstance m_instance{};
	vk::UniqueSurfaceKHR m_surface{};
	Gpu m_gpu{}; // not an RAII member.
	vk::UniqueDevice m_device{};
	vk::Queue m_queue{};		  // not an RAII member.
	vma::Allocator m_allocator{}; // anywhere between m_device and m_shader.

	std::optional<Swapchain> m_swapchain{};
	// command pool for all render Command Buffers.
	vk::UniqueCommandPool m_render_cmd_pool{};
	// command pool for all Command Blocks.
	vk::UniqueCommandPool m_cmd_block_pool{};
	// Sync and Command Buffer for virtual frames.
	Buffered<RenderSync> m_render_sync{};
	// Current virtual frame index.
	std::size_t m_frame_index{};

	std::optional<DearImGui> m_imgui{};

	vk::UniqueDescriptorPool m_descriptor_pool{};
	std::vector<vk::UniqueDescriptorSetLayout> m_set_layouts{};
	std::vector<vk::DescriptorSetLayout> m_set_layout_views{};
	vk::UniquePipelineLayout m_pipeline_layout{};

	std::optional<ShaderProgram> m_shader{};

	vma::Buffer m_vbo{};
	std::optional<ShaderBuffer> m_view_ubo{};
	std::optional<Texture> m_texture{};
	Buffered<std::vector<vk::DescriptorSet>> m_descriptor_sets{};

	glm::ivec2 m_framebuffer_size{};
	std::optional<RenderTarget> m_render_target{};
	bool m_wireframe{};

	Transform m_view_transform{};

	// waiter must be the last member to ensure it blocks until device is idle
	// before other members get destroyed.
	ScopedWaiter m_waiter{};
};
} // namespace lvk
