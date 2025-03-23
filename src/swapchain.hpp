#pragma once
#include <glm/vec2.hpp>
#include <gpu.hpp>
#include <render_target.hpp>
#include <optional>
#include <vector>

namespace lvk {
class Swapchain {
  public:
	explicit Swapchain(vk::Device device, Gpu const& gpu,
					   vk::SurfaceKHR surface, glm::ivec2 size);

	auto recreate(glm::ivec2 size) -> bool;

	[[nodiscard]] auto get_size() const -> glm::ivec2 {
		return {m_ci.imageExtent.width, m_ci.imageExtent.height};
	}

	[[nodiscard]] auto get_format() const -> vk::Format {
		return m_ci.imageFormat;
	}

	[[nodiscard]] auto acquire_next_image(vk::Semaphore to_signal)
		-> std::optional<RenderTarget>;

	[[nodiscard]] auto base_barrier() const -> vk::ImageMemoryBarrier2;

	[[nodiscard]] auto present(vk::Queue queue, vk::Semaphore to_wait) -> bool;

  private:
	void populate_images();
	void create_image_views();

	vk::Device m_device{};
	Gpu m_gpu{};

	vk::SwapchainCreateInfoKHR m_ci{};
	vk::UniqueSwapchainKHR m_swapchain{};
	std::vector<vk::Image> m_images{};
	std::vector<vk::UniqueImageView> m_image_views{};
	std::optional<std::size_t> m_image_index{};
};
} // namespace lvk
