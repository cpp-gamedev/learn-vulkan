#include <app.hpp>
#include <cassert>
#include <chrono>
#include <print>
#include <ranges>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace lvk {
using namespace std::chrono_literals;

void App::run() {
	create_window();
	create_instance();
	create_surface();
	select_gpu();
	create_device();
	create_swapchain();
	create_render_sync();

	main_loop();
}

void App::create_window() {
	m_window = glfw::create_window({1280, 720}, "Learn Vulkan");
}

void App::create_instance() {
	// initialize the dispatcher without any arguments.
	VULKAN_HPP_DEFAULT_DISPATCHER.init();
	auto const loader_version = vk::enumerateInstanceVersion();
	if (loader_version < vk_version_v) {
		throw std::runtime_error{"Loader does not support Vulkan 1.3"};
	}

	auto app_info = vk::ApplicationInfo{};
	app_info.setPApplicationName("Learn Vulkan").setApiVersion(vk_version_v);

	auto instance_ci = vk::InstanceCreateInfo{};
	// need WSI instance extensions here (platform-specific Swapchains).
	auto const extensions = glfw::instance_extensions();
	instance_ci.setPApplicationInfo(&app_info).setPEnabledExtensionNames(
		extensions);

	m_instance = vk::createInstanceUnique(instance_ci);
	// initialize the dispatcher against the created Instance.
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);
}

void App::create_surface() {
	m_surface = glfw::create_surface(m_window.get(), *m_instance);
}

void App::select_gpu() {
	m_gpu = get_suitable_gpu(*m_instance, *m_surface);
	std::println("[lvk] Using GPU: {}",
				 std::string_view{m_gpu.properties.deviceName});
}

void App::create_device() {
	auto queue_ci = vk::DeviceQueueCreateInfo{};
	// since we use only one queue, it has the entire priority range, ie, 1.0
	static constexpr auto queue_priorities_v = std::array{1.0f};
	queue_ci.setQueueFamilyIndex(m_gpu.queue_family)
		.setQueueCount(1)
		.setQueuePriorities(queue_priorities_v);

	// nice-to-have optional core features, enable if GPU supports them.
	auto enabled_features = vk::PhysicalDeviceFeatures{};
	enabled_features.fillModeNonSolid = m_gpu.features.fillModeNonSolid;
	enabled_features.wideLines = m_gpu.features.wideLines;
	enabled_features.samplerAnisotropy = m_gpu.features.samplerAnisotropy;
	enabled_features.sampleRateShading = m_gpu.features.sampleRateShading;

	// extra features that need to be explicitly enabled.
	auto sync_feature = vk::PhysicalDeviceSynchronization2Features{vk::True};
	auto dynamic_rendering_feature =
		vk::PhysicalDeviceDynamicRenderingFeatures{vk::True};
	// sync_feature.pNext => dynamic_rendering_feature,
	// and later device_ci.pNext => sync_feature.
	// this is 'pNext chaining'.
	sync_feature.setPNext(&dynamic_rendering_feature);

	auto device_ci = vk::DeviceCreateInfo{};
	// we only need one device extension: Swapchain.
	static constexpr auto extensions_v =
		std::array{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	device_ci.setPEnabledExtensionNames(extensions_v)
		.setQueueCreateInfos(queue_ci)
		.setPEnabledFeatures(&enabled_features)
		.setPNext(&sync_feature);

	m_device = m_gpu.device.createDeviceUnique(device_ci);
	// initialize the dispatcher against the created Device.
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);
	static constexpr std::uint32_t queue_index_v{0};
	m_queue = m_device->getQueue(m_gpu.queue_family, queue_index_v);

	m_waiter = *m_device;
}

void App::create_swapchain() {
	auto const size = glfw::framebuffer_size(m_window.get());
	m_swapchain.emplace(*m_device, m_gpu, *m_surface, size);
}

void App::create_render_sync() {
	// Command Buffers are 'allocated' from a Command Pool (which is 'created'
	// like all other Vulkan objects so far). We can allocate all the buffers
	// from a single pool here.
	auto command_pool_ci = vk::CommandPoolCreateInfo{};
	// this flag enables resetting the command buffer for re-recording (unlike a
	// single-time submit scenario).
	command_pool_ci.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		.setQueueFamilyIndex(m_gpu.queue_family);
	m_render_cmd_pool = m_device->createCommandPoolUnique(command_pool_ci);

	auto command_buffer_ai = vk::CommandBufferAllocateInfo{};
	command_buffer_ai.setCommandPool(*m_render_cmd_pool)
		.setCommandBufferCount(static_cast<std::uint32_t>(resource_buffering_v))
		.setLevel(vk::CommandBufferLevel::ePrimary);
	auto const command_buffers =
		m_device->allocateCommandBuffers(command_buffer_ai);
	assert(command_buffers.size() == m_render_sync.size());

	// we create Render Fences as pre-signaled so that on the first render for
	// each virtual frame we don't wait on their fences (since there's nothing
	// to wait for yet).
	static constexpr auto fence_create_info_v =
		vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled};
	for (auto [sync, command_buffer] :
		 std::views::zip(m_render_sync, command_buffers)) {
		sync.command_buffer = command_buffer;
		sync.draw = m_device->createSemaphoreUnique({});
		sync.present = m_device->createSemaphoreUnique({});
		sync.drawn = m_device->createFenceUnique(fence_create_info_v);
	}
}

void App::main_loop() {
	while (glfwWindowShouldClose(m_window.get()) == GLFW_FALSE) {
		glfwPollEvents();

		auto const framebuffer_size = glfw::framebuffer_size(m_window.get());
		// minimized? skip loop.
		if (framebuffer_size.x <= 0 || framebuffer_size.y <= 0) { continue; }
		// an eErrorOutOfDateKHR result is not guaranteed if the
		// framebuffer size does not match the Swapchain image size, check it
		// explicitly.
		auto fb_size_changed = framebuffer_size != m_swapchain->get_size();
		auto& render_sync = m_render_sync.at(m_frame_index);
		auto render_target = m_swapchain->acquire_next_image(*render_sync.draw);
		if (fb_size_changed || !render_target) {
			m_swapchain->recreate(framebuffer_size);
			continue;
		}

		static constexpr auto fence_timeout_v =
			static_cast<std::uint64_t>(std::chrono::nanoseconds{3s}.count());
		auto result = m_device->waitForFences(*render_sync.drawn, vk::True,
											  fence_timeout_v);
		if (result != vk::Result::eSuccess) {
			throw std::runtime_error{"Failed to wait for Render Fence"};
		}
		// reset fence _after_ acquisition of image: if it fails, the
		// fence remains signaled.
		m_device->resetFences(*render_sync.drawn);

		auto command_buffer_bi = vk::CommandBufferBeginInfo{};
		// this flag means recorded commands will not be reused.
		command_buffer_bi.setFlags(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		render_sync.command_buffer.begin(command_buffer_bi);

		auto dependency_info = vk::DependencyInfo{};
		auto barrier = m_swapchain->base_barrier();
		// Undefined => AttachmentOptimal
		// we don't need to block any operations before the barrier, since we
		// rely on the image acquired semaphore to block rendering.
		// any color attachment operations must happen after the barrier.
		barrier.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eAttachmentOptimal)
			.setSrcAccessMask(vk::AccessFlagBits2::eNone)
			.setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
			.setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
			.setDstStageMask(
				vk::PipelineStageFlagBits2::eColorAttachmentOutput);
		dependency_info.setImageMemoryBarriers(barrier);
		render_sync.command_buffer.pipelineBarrier2(dependency_info);

		auto attachment_info = vk::RenderingAttachmentInfo{};
		attachment_info.setImageView(render_target->image_view)
			.setImageLayout(vk::ImageLayout::eAttachmentOptimal)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setClearValue(vk::ClearColorValue{1.0f, 0.0f, 0.0f, 1.0f});
		auto rendering_info = vk::RenderingInfo{};
		auto const render_area =
			vk::Rect2D{vk::Offset2D{}, render_target->extent};
		rendering_info.setRenderArea(render_area)
			.setColorAttachments(attachment_info)
			.setLayerCount(1);

		render_sync.command_buffer.beginRendering(rendering_info);
		// draw stuff here.
		render_sync.command_buffer.endRendering();

		// AttachmentOptimal => PresentSrc
		// the barrier must wait for color attachment operations to complete.
		// we don't need any post-synchronization as the present Sempahore takes
		// care of that.
		barrier.setOldLayout(vk::ImageLayout::eAttachmentOptimal)
			.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
			.setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
			.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
			.setDstAccessMask(vk::AccessFlagBits2::eNone)
			.setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe);
		dependency_info.setImageMemoryBarriers(barrier);
		render_sync.command_buffer.pipelineBarrier2(dependency_info);

		render_sync.command_buffer.end();

		auto submit_info = vk::SubmitInfo2{};
		auto const command_buffer_info =
			vk::CommandBufferSubmitInfo{render_sync.command_buffer};
		auto wait_semaphore_info = vk::SemaphoreSubmitInfo{};
		wait_semaphore_info.setSemaphore(*render_sync.draw)
			.setStageMask(vk::PipelineStageFlagBits2::eTopOfPipe);
		auto signal_semaphore_info = vk::SemaphoreSubmitInfo{};
		signal_semaphore_info.setSemaphore(*render_sync.present)
			.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
		submit_info.setCommandBufferInfos(command_buffer_info)
			.setWaitSemaphoreInfos(wait_semaphore_info)
			.setSignalSemaphoreInfos(signal_semaphore_info);
		m_queue.submit2(submit_info, *render_sync.drawn);

		m_frame_index = (m_frame_index + 1) % m_render_sync.size();

		if (!m_swapchain->present(m_queue, *render_sync.present)) {
			m_swapchain->recreate(framebuffer_size);
			continue;
		}
	}
}
} // namespace lvk
