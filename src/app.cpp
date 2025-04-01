#include <app.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vertex.hpp>
#include <bit>
#include <cassert>
#include <chrono>
#include <fstream>
#include <print>
#include <ranges>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace lvk {
using namespace std::chrono_literals;

namespace {
template <typename T>
[[nodiscard]] constexpr auto to_byte_array(T const& t) {
	return std::bit_cast<std::array<std::byte, sizeof(T)>>(t);
}

constexpr auto layout_binding(std::uint32_t binding,
							  vk::DescriptorType const type) {
	return vk::DescriptorSetLayoutBinding{
		binding, type, 1, vk::ShaderStageFlagBits::eAllGraphics};
}

[[nodiscard]] auto locate_assets_dir() -> fs::path {
	// look for '<path>/assets/', starting from the working
	// directory and walking up the parent directory tree.
	static constexpr std::string_view dir_name_v{"assets"};
	for (auto path = fs::current_path();
		 !path.empty() && path.has_parent_path(); path = path.parent_path()) {
		auto ret = path / dir_name_v;
		if (fs::is_directory(ret)) { return ret; }
	}
	std::println("[lvk] Warning: could not locate '{}' directory", dir_name_v);
	return fs::current_path();
}

[[nodiscard]] auto get_layers(std::span<char const* const> desired)
	-> std::vector<char const*> {
	auto ret = std::vector<char const*>{};
	ret.reserve(desired.size());
	auto const available = vk::enumerateInstanceLayerProperties();
	for (char const* layer : desired) {
		auto const pred = [layer = std::string_view{layer}](
							  vk::LayerProperties const& properties) {
			return properties.layerName == layer;
		};
		if (std::ranges::find_if(available, pred) == available.end()) {
			std::println("[lvk] [WARNING] Vulkan Layer '{}' not found", layer);
			continue;
		}
		ret.push_back(layer);
	}
	return ret;
}

[[nodiscard]] auto to_spir_v(fs::path const& path)
	-> std::vector<std::uint32_t> {
	// open the file at the end, to get the total size.
	auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
	if (!file.is_open()) {
		throw std::runtime_error{
			std::format("Failed to open file: '{}'", path.generic_string())};
	}

	auto const size = file.tellg();
	auto const usize = static_cast<std::uint64_t>(size);
	// file data must be uint32 aligned.
	if (usize % sizeof(std::uint32_t) != 0) {
		throw std::runtime_error{std::format("Invalid SPIR-V size: {}", usize)};
	}

	// seek to the beginning before reading.
	file.seekg({}, std::ios::beg);
	auto ret = std::vector<std::uint32_t>{};
	ret.resize(usize / sizeof(std::uint32_t));
	void* data = ret.data();
	file.read(static_cast<char*>(data), size);
	return ret;
}
} // namespace

void App::run() {
	m_assets_dir = locate_assets_dir();

	create_window();
	create_instance();
	create_surface();
	select_gpu();
	create_device();
	create_allocator();
	create_swapchain();
	create_render_sync();
	create_imgui();
	create_descriptor_pool();
	create_pipeline_layout();
	create_shader();
	create_cmd_block_pool();

	create_shader_resources();
	create_descriptor_sets();

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

	// add the Shader Object emulation layer.
	static constexpr auto layers_v = std::array{
		"VK_LAYER_KHRONOS_shader_object",
	};
	auto const layers = get_layers(layers_v);
	instance_ci.setPEnabledLayerNames(layers);

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
	auto shader_object_feature =
		vk::PhysicalDeviceShaderObjectFeaturesEXT{vk::True};
	dynamic_rendering_feature.setPNext(&shader_object_feature);

	auto device_ci = vk::DeviceCreateInfo{};
	// we need two device extensions: Swapchain and Shader Object.
	static constexpr auto extensions_v = std::array{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		"VK_EXT_shader_object",
	};
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

void App::create_imgui() {
	auto const imgui_ci = DearImGui::CreateInfo{
		.window = m_window.get(),
		.api_version = vk_version_v,
		.instance = *m_instance,
		.physical_device = m_gpu.device,
		.queue_family = m_gpu.queue_family,
		.device = *m_device,
		.queue = m_queue,
		.color_format = m_swapchain->get_format(),
		.samples = vk::SampleCountFlagBits::e1,
	};
	m_imgui.emplace(imgui_ci);
}

void App::create_allocator() {
	m_allocator = vma::create_allocator(*m_instance, m_gpu.device, *m_device);
}

void App::create_descriptor_pool() {
	static constexpr auto pool_sizes_v = std::array{
		// 2 uniform buffers, can be more if desired.
		vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2},
		vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 2},
		vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 2},
	};
	auto pool_ci = vk::DescriptorPoolCreateInfo{};
	// allow 16 sets to be allocated from this pool.
	pool_ci.setPoolSizes(pool_sizes_v).setMaxSets(16);
	m_descriptor_pool = m_device->createDescriptorPoolUnique(pool_ci);
}

void App::create_pipeline_layout() {
	static constexpr auto set_0_bindings_v = std::array{
		layout_binding(0, vk::DescriptorType::eUniformBuffer),
	};
	static constexpr auto set_1_bindings_v = std::array{
		layout_binding(0, vk::DescriptorType::eCombinedImageSampler),
		layout_binding(1, vk::DescriptorType::eStorageBuffer),
	};
	auto set_layout_cis = std::array<vk::DescriptorSetLayoutCreateInfo, 2>{};
	set_layout_cis[0].setBindings(set_0_bindings_v);
	set_layout_cis[1].setBindings(set_1_bindings_v);

	for (auto const& set_layout_ci : set_layout_cis) {
		m_set_layouts.push_back(
			m_device->createDescriptorSetLayoutUnique(set_layout_ci));
		m_set_layout_views.push_back(*m_set_layouts.back());
	}

	auto pipeline_layout_ci = vk::PipelineLayoutCreateInfo{};
	pipeline_layout_ci.setSetLayouts(m_set_layout_views);
	m_pipeline_layout =
		m_device->createPipelineLayoutUnique(pipeline_layout_ci);
}

void App::create_shader() {
	auto const vertex_spirv = to_spir_v(asset_path("shader.vert"));
	auto const fragment_spirv = to_spir_v(asset_path("shader.frag"));

	static constexpr auto vertex_input_v = ShaderVertexInput{
		.attributes = vertex_attributes_v,
		.bindings = vertex_bindings_v,
	};
	auto const shader_ci = ShaderProgram::CreateInfo{
		.device = *m_device,
		.vertex_spirv = vertex_spirv,
		.fragment_spirv = fragment_spirv,
		.vertex_input = vertex_input_v,
		.set_layouts = m_set_layout_views,
	};
	m_shader.emplace(shader_ci);
}

void App::create_cmd_block_pool() {
	auto command_pool_ci = vk::CommandPoolCreateInfo{};
	command_pool_ci
		.setQueueFamilyIndex(m_gpu.queue_family)
		// this flag indicates that the allocated Command Buffers will be
		// short-lived.
		.setFlags(vk::CommandPoolCreateFlagBits::eTransient);
	m_cmd_block_pool = m_device->createCommandPoolUnique(command_pool_ci);
}

void App::create_shader_resources() {
	// vertices of a quad.
	static constexpr auto vertices_v = std::array{
		Vertex{.position = {-200.0f, -200.0f}, .uv = {0.0f, 1.0f}},
		Vertex{.position = {200.0f, -200.0f}, .uv = {1.0f, 1.0f}},
		Vertex{.position = {200.0f, 200.0f}, .uv = {1.0f, 0.0f}},
		Vertex{.position = {-200.0f, 200.0f}, .uv = {0.0f, 0.0f}},
	};
	static constexpr auto indices_v = std::array{
		0u, 1u, 2u, 2u, 3u, 0u,
	};
	static constexpr auto vertices_bytes_v = to_byte_array(vertices_v);
	static constexpr auto indices_bytes_v = to_byte_array(indices_v);
	static constexpr auto total_bytes_v =
		std::array<std::span<std::byte const>, 2>{
			vertices_bytes_v,
			indices_bytes_v,
		};
	// we want to write total_bytes_v to a Device VertexBuffer | IndexBuffer.
	auto const buffer_ci = vma::BufferCreateInfo{
		.allocator = m_allocator.get(),
		.usage = vk::BufferUsageFlagBits::eVertexBuffer |
				 vk::BufferUsageFlagBits::eIndexBuffer,
		.queue_family = m_gpu.queue_family,
	};
	m_vbo = vma::create_device_buffer(buffer_ci, create_command_block(),
									  total_bytes_v);

	m_view_ubo.emplace(m_allocator.get(), m_gpu.queue_family,
					   vk::BufferUsageFlagBits::eUniformBuffer);

	m_instance_ssbo.emplace(m_allocator.get(), m_gpu.queue_family,
							vk::BufferUsageFlagBits::eStorageBuffer);

	using Pixel = std::array<std::byte, 4>;
	static constexpr auto rgby_pixels_v = std::array{
		Pixel{std::byte{0xff}, {}, {}, std::byte{0xff}},
		Pixel{std::byte{}, std::byte{0xff}, {}, std::byte{0xff}},
		Pixel{std::byte{}, {}, std::byte{0xff}, std::byte{0xff}},
		Pixel{std::byte{0xff}, std::byte{0xff}, {}, std::byte{0xff}},
	};
	static constexpr auto rgby_bytes_v =
		std::bit_cast<std::array<std::byte, sizeof(rgby_pixels_v)>>(
			rgby_pixels_v);
	static constexpr auto rgby_bitmap_v = Bitmap{
		.bytes = rgby_bytes_v,
		.size = {2, 2},
	};
	auto texture_ci = Texture::CreateInfo{
		.device = *m_device,
		.allocator = m_allocator.get(),
		.queue_family = m_gpu.queue_family,
		.command_block = create_command_block(),
		.bitmap = rgby_bitmap_v,
	};
	// use Nearest filtering instead of Linear (interpolation).
	texture_ci.sampler.setMagFilter(vk::Filter::eNearest);
	m_texture.emplace(std::move(texture_ci));
}

void App::create_descriptor_sets() {
	for (auto& descriptor_sets : m_descriptor_sets) {
		descriptor_sets = allocate_sets();
	}
}

auto App::asset_path(std::string_view const uri) const -> fs::path {
	return m_assets_dir / uri;
}

auto App::create_command_block() const -> CommandBlock {
	return CommandBlock{*m_device, m_queue, *m_cmd_block_pool};
}

auto App::allocate_sets() const -> std::vector<vk::DescriptorSet> {
	auto allocate_info = vk::DescriptorSetAllocateInfo{};
	allocate_info.setDescriptorPool(*m_descriptor_pool)
		.setSetLayouts(m_set_layout_views);
	return m_device->allocateDescriptorSets(allocate_info);
}

void App::main_loop() {
	while (glfwWindowShouldClose(m_window.get()) == GLFW_FALSE) {
		glfwPollEvents();
		if (!acquire_render_target()) { continue; }
		auto const command_buffer = begin_frame();
		transition_for_render(command_buffer);
		render(command_buffer);
		transition_for_present(command_buffer);
		submit_and_present();
	}
}

auto App::acquire_render_target() -> bool {
	m_framebuffer_size = glfw::framebuffer_size(m_window.get());
	// minimized? skip loop.
	if (m_framebuffer_size.x <= 0 || m_framebuffer_size.y <= 0) {
		return false;
	}

	auto& render_sync = m_render_sync.at(m_frame_index);

	// wait for the fence to be signaled.
	static constexpr auto fence_timeout_v =
		static_cast<std::uint64_t>(std::chrono::nanoseconds{3s}.count());
	auto result =
		m_device->waitForFences(*render_sync.drawn, vk::True, fence_timeout_v);
	if (result != vk::Result::eSuccess) {
		throw std::runtime_error{"Failed to wait for Render Fence"};
	}

	m_render_target = m_swapchain->acquire_next_image(*render_sync.draw);
	if (!m_render_target) {
		// acquire failure => ErrorOutOfDate. Recreate Swapchain.
		m_swapchain->recreate(m_framebuffer_size);
		return false;
	}

	// reset fence _after_ acquisition of image: if it fails, the
	// fence remains signaled.
	m_device->resetFences(*render_sync.drawn);
	m_imgui->new_frame();

	return true;
}

auto App::begin_frame() -> vk::CommandBuffer {
	auto const& render_sync = m_render_sync.at(m_frame_index);

	auto command_buffer_bi = vk::CommandBufferBeginInfo{};
	// this flag means recorded commands will not be reused.
	command_buffer_bi.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	render_sync.command_buffer.begin(command_buffer_bi);
	return render_sync.command_buffer;
}

void App::transition_for_render(vk::CommandBuffer const command_buffer) const {
	auto dependency_info = vk::DependencyInfo{};
	auto barrier = m_swapchain->base_barrier();
	// Undefined => AttachmentOptimal
	// the barrier must wait for prior color attachment operations to complete,
	// and block subsequent ones.
	barrier.setOldLayout(vk::ImageLayout::eUndefined)
		.setNewLayout(vk::ImageLayout::eAttachmentOptimal)
		.setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead |
						  vk::AccessFlagBits2::eColorAttachmentWrite)
		.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
		.setDstAccessMask(barrier.srcAccessMask)
		.setDstStageMask(barrier.srcStageMask);
	dependency_info.setImageMemoryBarriers(barrier);
	command_buffer.pipelineBarrier2(dependency_info);
}

void App::render(vk::CommandBuffer const command_buffer) {
	auto color_attachment = vk::RenderingAttachmentInfo{};
	color_attachment.setImageView(m_render_target->image_view)
		.setImageLayout(vk::ImageLayout::eAttachmentOptimal)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		// temporarily red.
		.setClearValue(vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f});
	auto rendering_info = vk::RenderingInfo{};
	auto const render_area =
		vk::Rect2D{vk::Offset2D{}, m_render_target->extent};
	rendering_info.setRenderArea(render_area)
		.setColorAttachments(color_attachment)
		.setLayerCount(1);

	command_buffer.beginRendering(rendering_info);
	inspect();
	update_view();
	update_instances();
	draw(command_buffer);
	command_buffer.endRendering();

	m_imgui->end_frame();
	// we don't want to clear the image again, instead load it intact after the
	// previous pass.
	color_attachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
	rendering_info.setColorAttachments(color_attachment)
		.setPDepthAttachment(nullptr);
	command_buffer.beginRendering(rendering_info);
	m_imgui->render(command_buffer);
	command_buffer.endRendering();
}

void App::transition_for_present(vk::CommandBuffer const command_buffer) const {
	auto dependency_info = vk::DependencyInfo{};
	auto barrier = m_swapchain->base_barrier();
	// AttachmentOptimal => PresentSrc
	// the barrier must wait for prior color attachment operations to complete,
	// and block subsequent ones.
	barrier.setOldLayout(vk::ImageLayout::eAttachmentOptimal)
		.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
		.setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead |
						  vk::AccessFlagBits2::eColorAttachmentWrite)
		.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
		.setDstAccessMask(barrier.srcAccessMask)
		.setDstStageMask(barrier.srcStageMask);
	dependency_info.setImageMemoryBarriers(barrier);
	command_buffer.pipelineBarrier2(dependency_info);
}

void App::submit_and_present() {
	auto const& render_sync = m_render_sync.at(m_frame_index);
	render_sync.command_buffer.end();

	auto submit_info = vk::SubmitInfo2{};
	auto const command_buffer_info =
		vk::CommandBufferSubmitInfo{render_sync.command_buffer};
	auto wait_semaphore_info = vk::SemaphoreSubmitInfo{};
	wait_semaphore_info.setSemaphore(*render_sync.draw)
		.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	auto signal_semaphore_info = vk::SemaphoreSubmitInfo{};
	signal_semaphore_info.setSemaphore(*render_sync.present)
		.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	submit_info.setCommandBufferInfos(command_buffer_info)
		.setWaitSemaphoreInfos(wait_semaphore_info)
		.setSignalSemaphoreInfos(signal_semaphore_info);
	m_queue.submit2(submit_info, *render_sync.drawn);

	m_frame_index = (m_frame_index + 1) % m_render_sync.size();
	m_render_target.reset();

	// an eErrorOutOfDateKHR result is not guaranteed if the
	// framebuffer size does not match the Swapchain image size, check it
	// explicitly.
	auto const fb_size_changed = m_framebuffer_size != m_swapchain->get_size();
	auto const out_of_date =
		!m_swapchain->present(m_queue, *render_sync.present);
	if (fb_size_changed || out_of_date) {
		m_swapchain->recreate(m_framebuffer_size);
	}
}

void App::inspect() {
	ImGui::ShowDemoWindow();

	ImGui::SetNextWindowSize({200.0f, 100.0f}, ImGuiCond_Once);
	if (ImGui::Begin("Inspect")) {
		if (ImGui::Checkbox("wireframe", &m_wireframe)) {
			m_shader->polygon_mode =
				m_wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
		}
		if (m_wireframe) {
			auto const& line_width_range =
				m_gpu.properties.limits.lineWidthRange;
			ImGui::SetNextItemWidth(100.0f);
			ImGui::DragFloat("line width", &m_shader->line_width, 0.25f,
							 line_width_range[0], line_width_range[1]);
		}

		static auto const inspect_transform = [](Transform& out) {
			ImGui::DragFloat2("position", &out.position.x);
			ImGui::DragFloat("rotation", &out.rotation);
			ImGui::DragFloat2("scale", &out.scale.x, 0.1f);
		};

		ImGui::Separator();
		if (ImGui::TreeNode("View")) {
			inspect_transform(m_view_transform);
			ImGui::TreePop();
		}

		ImGui::Separator();
		if (ImGui::TreeNode("Instances")) {
			for (std::size_t i = 0; i < m_instances.size(); ++i) {
				auto const label = std::to_string(i);
				if (ImGui::TreeNode(label.c_str())) {
					inspect_transform(m_instances.at(i));
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();
}

void App::update_view() {
	auto const half_size = 0.5f * glm::vec2{m_framebuffer_size};
	auto const mat_projection =
		glm::ortho(-half_size.x, half_size.x, -half_size.y, half_size.y);
	auto const mat_view = m_view_transform.view_matrix();
	auto const mat_vp = mat_projection * mat_view;
	auto const bytes =
		std::bit_cast<std::array<std::byte, sizeof(mat_vp)>>(mat_vp);
	m_view_ubo->write_at(m_frame_index, bytes);
}

void App::update_instances() {
	m_instance_data.clear();
	m_instance_data.reserve(m_instances.size());
	for (auto const& transform : m_instances) {
		m_instance_data.push_back(transform.model_matrix());
	}
	// can't use bit_cast anymore, reinterpret data as a byte array instead.
	auto const span = std::span{m_instance_data};
	void* data = span.data();
	auto const bytes =
		std::span{static_cast<std::byte const*>(data), span.size_bytes()};
	m_instance_ssbo->write_at(m_frame_index, bytes);
}

void App::draw(vk::CommandBuffer const command_buffer) const {
	m_shader->bind(command_buffer, m_framebuffer_size);
	bind_descriptor_sets(command_buffer);
	// single VBO at binding 0 at no offset.
	command_buffer.bindVertexBuffers(0, m_vbo.get().buffer, vk::DeviceSize{});
	// u32 indices after offset of 4 vertices.
	command_buffer.bindIndexBuffer(m_vbo.get().buffer, 4 * sizeof(Vertex),
								   vk::IndexType::eUint32);
	auto const instances = static_cast<std::uint32_t>(m_instances.size());
	// m_vbo has 6 indices.
	command_buffer.drawIndexed(6, instances, 0, 0, 0);
}

void App::bind_descriptor_sets(vk::CommandBuffer const command_buffer) const {
	auto writes = std::array<vk::WriteDescriptorSet, 3>{};
	auto const& descriptor_sets = m_descriptor_sets.at(m_frame_index);
	auto const set0 = descriptor_sets[0];
	auto write = vk::WriteDescriptorSet{};
	auto const view_ubo_info = m_view_ubo->descriptor_info_at(m_frame_index);
	write.setBufferInfo(view_ubo_info)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1)
		.setDstSet(set0)
		.setDstBinding(0);
	writes[0] = write;

	auto const set1 = descriptor_sets[1];
	auto const image_info = m_texture->descriptor_info();
	write.setImageInfo(image_info)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1)
		.setDstSet(set1)
		.setDstBinding(0);
	writes[1] = write;
	auto const instance_ssbo_info =
		m_instance_ssbo->descriptor_info_at(m_frame_index);
	write.setBufferInfo(instance_ssbo_info)
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setDescriptorCount(1)
		.setDstSet(set1)
		.setDstBinding(1);
	writes[2] = write;

	m_device->updateDescriptorSets(writes, {});

	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
									  *m_pipeline_layout, 0, descriptor_sets,
									  {});
}
} // namespace lvk
