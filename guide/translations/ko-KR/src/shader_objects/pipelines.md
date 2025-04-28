# 그래픽스 파이프라인

여기서는 셰이더 오브젝트 대신 그래픽스 파이프라인을 사용하는 방법을 설명합니다. 이 가이드는 셰이더 오브젝트를 사용한다고 가정하지만, 그래픽스 파이프라인을 대신 사용하더라도 나머지 코드에는 큰 변화가 없을 것입니다. 다만, 알아둬야할 예외 사항은 디스크립터 셋 레이아웃 설정 방식입니다. 셰이더 오브젝트에서는 ShaderEXT의 CreateInfo에 포함되지만, 파이프라인을 사용할 경우 파이프라인 레이아웃에 지정해야 합니다.

## 파이프라인 상태

셰이더 오브젝트는 대부분의 동적 상태를 런타임에 설정할 수 있었지만, 파이프라인에서는 파이프라인 생성 시점에 고정되기 때문에 정적입니다. 파이프라인은 또한 어태치먼트 포맷과 샘플 수 같은 추가 파라미터를 요구합니다. 이러한 값들은 상수로 간주되어 이후의 추상화 클래스에 담길 것입니다. 동적 상태의 일부를 구조체를 통해 나타냅시다.

```cpp
// bit flags for various binary Pipeline States.
struct PipelineFlag {
  enum : std::uint8_t {
    None = 0,
    AlphaBlend = 1 << 0, // turn on alpha blending.
    DepthTest = 1 << 1,  // turn on depth write and test.
  };
};

// specification of a unique Graphics Pipeline.
struct PipelineState {
  using Flag = PipelineFlag;

  [[nodiscard]] static constexpr auto default_flags() -> std::uint8_t {
    return Flag::AlphaBlend | Flag::DepthTest;
  }

  vk::ShaderModule vertex_shader;   // required.
  vk::ShaderModule fragment_shader; // required.

  std::span<vk::VertexInputAttributeDescription const> vertex_attributes{};
  std::span<vk::VertexInputBindingDescription const> vertex_bindings{};

  vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
  vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};
  vk::CullModeFlags cull_mode{vk::CullModeFlagBits::eNone};
  vk::CompareOp depth_compare{vk::CompareOp::eLess};
  std::uint8_t flags{default_flags()};
};
```

파이프라인을 구성하는 과정을 클래스로 캡슐화합시다.

```cpp
struct PipelineBuilderCreateInfo {
  vk::Device device{};
  vk::SampleCountFlagBits samples{};
  vk::Format color_format{};
  vk::Format depth_format{};
};

class PipelineBuilder {
  public:
  using CreateInfo = PipelineBuilderCreateInfo;

  explicit PipelineBuilder(CreateInfo const& create_info)
    : m_info(create_info) {}

  [[nodiscard]] auto build(vk::PipelineLayout layout,
               PipelineState const& state) const
    -> vk::UniquePipeline;

  private:
  CreateInfo m_info{};
};
```

구현은 다소 길어질 수 있으니, 여러 함수로 나누어 작성하는 것이 좋습니다.

```cpp
// single viewport and scissor.
constexpr auto viewport_state_v =
  vk::PipelineViewportStateCreateInfo({}, 1, {}, 1);

// these dynamic states are guaranteed to be available.
constexpr auto dynamic_states_v = std::array{
  vk::DynamicState::eViewport,
  vk::DynamicState::eScissor,
  vk::DynamicState::eLineWidth,
};

[[nodiscard]] auto create_shader_stages(vk::ShaderModule const vertex,
                    vk::ShaderModule const fragment) {
  // set vertex (0) and fragment (1) shader stages.
  auto ret = std::array<vk::PipelineShaderStageCreateInfo, 2>{};
  ret[0]
    .setStage(vk::ShaderStageFlagBits::eVertex)
    .setPName("main")
    .setModule(vertex);
  ret[1]
    .setStage(vk::ShaderStageFlagBits::eFragment)
    .setPName("main")
    .setModule(fragment);
  return ret;
}

[[nodiscard]] constexpr auto
create_depth_stencil_state(std::uint8_t flags,
               vk::CompareOp const depth_compare) {
  auto ret = vk::PipelineDepthStencilStateCreateInfo{};
  auto const depth_test =
    (flags & PipelineFlag::DepthTest) == PipelineFlag::DepthTest;
  ret.setDepthTestEnable(depth_test ? vk::True : vk::False)
    .setDepthCompareOp(depth_compare);
  return ret;
}

[[nodiscard]] constexpr auto
create_color_blend_attachment(std::uint8_t const flags) {
  auto ret = vk::PipelineColorBlendAttachmentState{};
  auto const alpha_blend =
    (flags & PipelineFlag::AlphaBlend) == PipelineFlag::AlphaBlend;
  using CCF = vk::ColorComponentFlagBits;
  ret.setColorWriteMask(CCF::eR | CCF::eG | CCF::eB | CCF::eA)
    .setBlendEnable(alpha_blend ? vk::True : vk::False)
    // standard alpha blending:
    // (alpha * src) + (1 - alpha) * dst
    .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
    .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
    .setColorBlendOp(vk::BlendOp::eAdd)
    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
    .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
    .setAlphaBlendOp(vk::BlendOp::eAdd);
  return ret;
}

// ...
auto PipelineBuilder::build(vk::PipelineLayout const layout,
              PipelineState const& state) const
  -> vk::UniquePipeline {
  auto const shader_stage_ci =
    create_shader_stages(state.vertex_shader, state.fragment_shader);

  auto vertex_input_ci = vk::PipelineVertexInputStateCreateInfo{};
  vertex_input_ci.setVertexAttributeDescriptions(state.vertex_attributes)
    .setVertexBindingDescriptions(state.vertex_bindings);

  auto multisample_state_ci = vk::PipelineMultisampleStateCreateInfo{};
  multisample_state_ci.setRasterizationSamples(m_info.samples)
    .setSampleShadingEnable(vk::False);

  auto const input_assembly_ci =
    vk::PipelineInputAssemblyStateCreateInfo{{}, state.topology};

  auto rasterization_state_ci = vk::PipelineRasterizationStateCreateInfo{};
  rasterization_state_ci.setPolygonMode(state.polygon_mode)
    .setCullMode(state.cull_mode);

  auto const depth_stencil_state_ci =
    create_depth_stencil_state(state.flags, state.depth_compare);

  auto const color_blend_attachment =
    create_color_blend_attachment(state.flags);
  auto color_blend_state_ci = vk::PipelineColorBlendStateCreateInfo{};
  color_blend_state_ci.setAttachments(color_blend_attachment);

  auto dynamic_state_ci = vk::PipelineDynamicStateCreateInfo{};
  dynamic_state_ci.setDynamicStates(dynamic_states_v);

  // Dynamic Rendering requires passing this in the pNext chain.
  auto rendering_ci = vk::PipelineRenderingCreateInfo{};
  // could be a depth-only pass, argument is span-like (notice the plural
  // `Formats()`), only set if not Undefined.
  if (m_info.color_format != vk::Format::eUndefined) {
    rendering_ci.setColorAttachmentFormats(m_info.color_format);
  }
  // single depth attachment format, ok to set to Undefined.
  rendering_ci.setDepthAttachmentFormat(m_info.depth_format);

  auto pipeline_ci = vk::GraphicsPipelineCreateInfo{};
  pipeline_ci.setLayout(layout)
    .setStages(shader_stage_ci)
    .setPVertexInputState(&vertex_input_ci)
    .setPViewportState(&viewport_state_v)
    .setPMultisampleState(&multisample_state_ci)
    .setPInputAssemblyState(&input_assembly_ci)
    .setPRasterizationState(&rasterization_state_ci)
    .setPDepthStencilState(&depth_stencil_state_ci)
    .setPColorBlendState(&color_blend_state_ci)
    .setPDynamicState(&dynamic_state_ci)
    .setPNext(&rendering_ci);

  auto ret = vk::Pipeline{};
  // use non-throwing API.
  if (m_info.device.createGraphicsPipelines({}, 1, &pipeline_ci, {}, &ret) !=
    vk::Result::eSuccess) {
    std::println(stderr, "[lvk] Failed to create Graphics Pipeline");
    return {};
  }

  return vk::UniquePipeline{ret, m_info.device};
}
```

`App` will need to store a builder, a Pipeline Layout, and the Pipeline(s):
`App`은 빌더, 파이프라인 레이아웃, 그리고 파이프라인을 담아야 합니다.

```cpp
std::optional<PipelineBuilder> m_pipeline_builder{};
vk::UniquePipelineLayout m_pipeline_layout{};
vk::UniquePipeline m_pipeline{};

// ...
void create_pipeline() {
  auto const vertex_spirv = to_spir_v(asset_path("shader.vert"));
  auto const fragment_spirv = to_spir_v(asset_path("shader.frag"));
  if (vertex_spirv.empty() || fragment_spirv.empty()) {
    throw std::runtime_error{"Failed to load shaders"};
  }

  auto pipeline_layout_ci = vk::PipelineLayoutCreateInfo{};
  pipeline_layout_ci.setSetLayouts({});
  m_pipeline_layout =
    m_device->createPipelineLayoutUnique(pipeline_layout_ci);

  auto const pipeline_builder_ci = PipelineBuilder::CreateInfo{
    .device = *m_device,
    .samples = vk::SampleCountFlagBits::e1,
    .color_format = m_swapchain->get_format(),
  };
  m_pipeline_builder.emplace(pipeline_builder_ci);

  auto vertex_ci = vk::ShaderModuleCreateInfo{};
  vertex_ci.setCode(vertex_spirv);
  auto fragment_ci = vk::ShaderModuleCreateInfo{};
  fragment_ci.setCode(fragment_spirv);

  auto const vertex_shader =
    m_device->createShaderModuleUnique(vertex_ci);
  auto const fragment_shader =
    m_device->createShaderModuleUnique(fragment_ci);
  auto const pipeline_state = PipelineState{
    .vertex_shader = *vertex_shader,
    .fragment_shader = *fragment_shader,
  };
  m_pipeline =
    m_pipeline_builder->build(*m_pipeline_layout, pipeline_state);
}
```

마지막으로 `App::draw()`를 구현합니다.

```cpp
void draw(vk::CommandBuffer const command_buffer) const {
  command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                *m_pipeline);
  auto viewport = vk::Viewport{};
  viewport.setX(0.0f)
    .setY(static_cast<float>(m_render_target->extent.height))
    .setWidth(static_cast<float>(m_render_target->extent.width))
    .setHeight(-viewport.y);
  command_buffer.setViewport(0, viewport);
  command_buffer.setScissor(0, vk::Rect2D{{}, m_render_target->extent});
  command_buffer.draw(3, 1, 0, 0);
}
```
