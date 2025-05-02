# class DearImGui

Dear ImGui는 자체적인 초기화 과정과 렌더링 루프를 가지고 있으며, 이를 `class DearImGui`로 캡슐화하겠습니다.

```cpp
struct DearImGuiCreateInfo {
  GLFWwindow* window{};
  std::uint32_t api_version{};
  vk::Instance instance{};
  vk::PhysicalDevice physical_device{};
  std::uint32_t queue_family{};
  vk::Device device{};
  vk::Queue queue{};
  vk::Format color_format{}; // single color attachment.
  vk::SampleCountFlagBits samples{};
};

class DearImGui {
  public:
  using CreateInfo = DearImGuiCreateInfo;

  explicit DearImGui(CreateInfo const& create_info);

  void new_frame();
  void end_frame();
  void render(vk::CommandBuffer command_buffer) const;

  private:
  enum class State : std::int8_t { Ended, Begun };

  struct Deleter {
    void operator()(vk::Device device) const;
  };

  State m_state{};

  Scoped<vk::Device, Deleter> m_device{};
};
```

생성자에서는 ImGui 컨텍스트를 생성하고, Vulkan 함수를 불러와 Vulkan을 위한 GLFW 초기화를 진행합니다

```cpp
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
```

그 후 Vulkan용 Dear ImGui를 초기화합니다.

```cpp
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
```

sRGB 포맷을 사용하고 있지만 Dear ImGui는 색상 공간에 대한 인식이 없기 때문에, 스타일 색상들을 선형 공간으로 변환해주어야 합니다. 이렇게 하면 감마 보정 과정을 통해 의도한 색상이 출력됩니다.

```cpp
ImGui::StyleColorsDark();
// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
for (auto& colour : ImGui::GetStyle().Colors) {
  auto const linear = glm::convertSRGBToLinear(
    glm::vec4{colour.x, colour.y, colour.z, colour.w});
  colour = ImVec4{linear.x, linear.y, linear.z, linear.w};
}
ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.99f; // more opaque
```

마지막으로 삭제자(Deleter)를 생성하고 구현합니다.

```cpp
m_device = Scoped<vk::Device, Deleter>{create_info.device};

// ...
void DearImGui::Deleter::operator()(vk::Device const device) const {
  device.waitIdle();
  ImGui_ImplVulkan_DestroyFontsTexture();
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
```

이 외의 나머지 함수들은 비교적 단순합니다.

```cpp
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
```
