#include <swapchain.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <print>
#include <stdexcept>

namespace lvk {
namespace {
constexpr std::uint32_t min_images_v{3};

constexpr auto srgb_formats_v = std::array{
	vk::Format::eR8G8B8A8Srgb,
	vk::Format::eB8G8R8A8Srgb,
};

constexpr auto subresource_range_v = [] {
	auto ret = vk::ImageSubresourceRange{};
	// this is a color image with 1 layer and 1 mip-level (the default).
	ret.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setLayerCount(1)
		.setLevelCount(1);
	return ret;
}();

[[nodiscard]] constexpr auto
get_surface_format(std::span<vk::SurfaceFormatKHR const> supported)
	-> vk::SurfaceFormatKHR {
	for (auto const desired : srgb_formats_v) {
		auto const is_match = [desired](vk::SurfaceFormatKHR const& in) {
			return in.format == desired &&
				   in.colorSpace ==
					   vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
		};
		auto const it = std::ranges::find_if(supported, is_match);
		if (it == supported.end()) { continue; }
		return *it;
	}
	return supported.front();
}

// returns currentExtent if specified, else clamped size.
[[nodiscard]] constexpr auto
get_image_extent(vk::SurfaceCapabilitiesKHR const& capabilities,
				 glm::uvec2 const size) -> vk::Extent2D {
	constexpr auto limitless_v = 0xffffffff;
	if (capabilities.currentExtent.width < limitless_v &&
		capabilities.currentExtent.height < limitless_v) {
		return capabilities.currentExtent;
	}
	auto const x = std::clamp(size.x, capabilities.minImageExtent.width,
							  capabilities.maxImageExtent.width);
	auto const y = std::clamp(size.y, capabilities.minImageExtent.height,
							  capabilities.maxImageExtent.height);
	return vk::Extent2D{x, y};
}

[[nodiscard]] constexpr auto
get_image_count(vk::SurfaceCapabilitiesKHR const& capabilities)
	-> std::uint32_t {
	if (capabilities.maxImageCount < capabilities.minImageCount) {
		return std::max(min_images_v, capabilities.minImageCount);
	}
	return std::clamp(min_images_v, capabilities.minImageCount,
					  capabilities.maxImageCount);
}

// throws if result is not eSuccess.
void require_success(vk::Result const result, char const* error_msg) {
	if (result != vk::Result::eSuccess) { throw std::runtime_error{error_msg}; }
}

auto needs_recreation(vk::Result const result) -> bool {
	switch (result) {
	case vk::Result::eSuccess:
	case vk::Result::eSuboptimalKHR: return false;
	case vk::Result::eErrorOutOfDateKHR: return true;
	default: break;
	}
	throw std::runtime_error{"Swapchain Error"};
}
} // namespace

Swapchain::Swapchain(vk::Device const device, Gpu const& gpu,
					 vk::SurfaceKHR const surface, glm::ivec2 const size)
	: m_device(device), m_gpu(gpu) {
	auto const surface_format =
		get_surface_format(m_gpu.device.getSurfaceFormatsKHR(surface));
	m_ci.setSurface(surface)
		.setImageFormat(surface_format.format)
		.setImageColorSpace(surface_format.colorSpace)
		.setImageArrayLayers(1)
		// Swapchain images will be used as color attachments (render targets).
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		// eFifo is guaranteed to be supported.
		.setPresentMode(vk::PresentModeKHR::eFifo);
	if (!recreate(size)) {
		throw std::runtime_error{"Failed to create Vulkan Swapchain"};
	}
}

auto Swapchain::recreate(glm::ivec2 size) -> bool {
	// Image sizes must be positive.
	if (size.x <= 0 || size.y <= 0) { return false; }

	auto const capabilities =
		m_gpu.device.getSurfaceCapabilitiesKHR(m_ci.surface);
	m_ci.setImageExtent(get_image_extent(capabilities, size))
		.setMinImageCount(get_image_count(capabilities))
		.setOldSwapchain(m_swapchain ? *m_swapchain : vk::SwapchainKHR{})
		.setQueueFamilyIndices(m_gpu.queue_family);
	assert(m_ci.imageExtent.width > 0 && m_ci.imageExtent.height > 0 &&
		   m_ci.minImageCount >= min_images_v);

	// wait for the device to be idle before destroying the current swapchain.
	m_device.waitIdle();
	m_swapchain = m_device.createSwapchainKHRUnique(m_ci);
	m_image_index.reset();

	populate_images();
	create_image_views();

	size = get_size();
	std::println("[lvk] Swapchain [{}x{}]", size.x, size.y);
	return true;
}

auto Swapchain::acquire_next_image(vk::Semaphore const to_signal)
	-> std::optional<RenderTarget> {
	assert(!m_image_index);
	static constexpr auto timeout_v = std::numeric_limits<std::uint64_t>::max();
	auto image_index = std::uint32_t{};
	auto const result = m_device.acquireNextImageKHR(
		*m_swapchain, timeout_v, to_signal, {}, &image_index);
	if (needs_recreation(result)) { return {}; }

	m_image_index = static_cast<std::size_t>(image_index);
	return RenderTarget{
		.image = m_images.at(*m_image_index),
		.image_view = *m_image_views.at(*m_image_index),
		.extent = m_ci.imageExtent,
	};
}

auto Swapchain::base_barrier() const -> vk::ImageMemoryBarrier2 {
	// fill up the parts common to all barriers.
	auto ret = vk::ImageMemoryBarrier2{};
	ret.setImage(m_images.at(m_image_index.value()))
		.setSubresourceRange(subresource_range_v)
		.setSrcQueueFamilyIndex(m_gpu.queue_family)
		.setDstQueueFamilyIndex(m_gpu.queue_family);
	return ret;
}

auto Swapchain::present(vk::Queue const queue, vk::Semaphore const to_wait)
	-> bool {
	auto const image_index = static_cast<std::uint32_t>(m_image_index.value());
	auto present_info = vk::PresentInfoKHR{};
	present_info.setSwapchains(*m_swapchain)
		.setImageIndices(image_index)
		.setWaitSemaphores(to_wait);
	auto const result = queue.presentKHR(&present_info);
	m_image_index.reset();
	return !needs_recreation(result);
}

void Swapchain::populate_images() {
	// we use the more verbose two-call API to avoid assigning m_images to a new
	// vector on every call.
	auto image_count = std::uint32_t{};
	auto result =
		m_device.getSwapchainImagesKHR(*m_swapchain, &image_count, nullptr);
	require_success(result, "Failed to get Swapchain Images");

	m_images.resize(image_count);
	result = m_device.getSwapchainImagesKHR(*m_swapchain, &image_count,
											m_images.data());
	require_success(result, "Failed to get Swapchain Images");
}

void Swapchain::create_image_views() {
	auto image_view_ci = vk::ImageViewCreateInfo{};
	// set common parameters here (everything except the Image).
	image_view_ci.setViewType(vk::ImageViewType::e2D)
		.setFormat(m_ci.imageFormat)
		.setSubresourceRange(subresource_range_v);
	m_image_views.clear();
	m_image_views.reserve(m_images.size());
	for (auto const image : m_images) {
		image_view_ci.setImage(image);
		m_image_views.push_back(m_device.createImageViewUnique(image_view_ci));
	}
}
} // namespace lvk
