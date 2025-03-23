#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <dear_imgui.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/mat4x4.hpp>
#include <resource_buffering.hpp>
#include <stdexcept>

namespace lvk {
DearImGui::DearImGui(CreateInfo const& create_info) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	static auto const load_vk_func = +[](char const* name, void* user_data) {
		return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(
			*static_cast<vk::Instance*>(user_data), name);
	};
	auto instance = create_info.instance;
	ImGui_ImplVulkan_LoadFunctions(create_info.api_version, load_vk_func,
								   &instance);

	if (!ImGui_ImplGlfw_InitForVulkan(create_info.window, true)) {
		throw std::runtime_error{"Failed to initialize Dear ImGui"};
	}

	auto init_info = ImGui_ImplVulkan_InitInfo{};
	init_info.ApiVersion = create_info.api_version;
	init_info.Instance = create_info.instance;
	init_info.PhysicalDevice = create_info.physical_device;
	init_info.Device = create_info.device;
	init_info.QueueFamily = create_info.queue_family;
	init_info.Queue = create_info.queue;
	init_info.MinImageCount = 2;
	init_info.ImageCount = static_cast<std::uint32_t>(resource_buffering_v);
	init_info.MSAASamples =
		static_cast<VkSampleCountFlagBits>(create_info.samples);
	init_info.DescriptorPoolSize = 2;
	auto pipline_rendering_ci = vk::PipelineRenderingCreateInfo{};
	pipline_rendering_ci.setColorAttachmentCount(1).setColorAttachmentFormats(
		create_info.color_format);
	init_info.PipelineRenderingCreateInfo = pipline_rendering_ci;
	init_info.UseDynamicRendering = true;
	if (!ImGui_ImplVulkan_Init(&init_info)) {
		throw std::runtime_error{"Failed to initialize Dear ImGui"};
	}
	ImGui_ImplVulkan_CreateFontsTexture();

	ImGui::StyleColorsDark();
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
	for (auto& colour : ImGui::GetStyle().Colors) {
		auto const linear = glm::convertSRGBToLinear(
			glm::vec4{colour.x, colour.y, colour.z, colour.w});
		colour = ImVec4{linear.x, linear.y, linear.z, linear.w};
	}
	ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.99f; // more opaque

	m_device = Scoped<vk::Device, Deleter>{create_info.device};
}

void DearImGui::new_frame() {
	if (m_state == State::Begun) { end_frame(); }
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();
	m_state = State::Begun;
}

void DearImGui::end_frame() {
	if (m_state == State::Ended) { return; }
	ImGui::Render();
	m_state = State::Ended;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void DearImGui::render(vk::CommandBuffer const command_buffer) const {
	auto* data = ImGui::GetDrawData();
	if (data == nullptr) { return; }
	ImGui_ImplVulkan_RenderDrawData(data, command_buffer);
}

void DearImGui::Deleter::operator()(vk::Device const device) const {
	device.waitIdle();
	ImGui_ImplVulkan_DestroyFontsTexture();
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}
} // namespace lvk
