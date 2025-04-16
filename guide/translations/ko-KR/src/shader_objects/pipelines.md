# Graphics Pipelines

This page describes the usage of Graphics Pipelines _instead of_ Shader Objects. While the guide assumes Shader Object usage, not much should change in the rest of the code if you instead choose to use Graphics Pipelines. A notable exception is the setup of Descriptor Set Layouts: with pipelines it needs to be specified as part of the Pipeline Layout, whereas with Shader Objects it is part of each ShaderEXT's CreateInfo.

## Pipeline State

Most dynamic state with Shader Objects is static with pipelines: specified at pipeline creation time. Pipelines also require additional parameters, like attachment formats and sample count: these will be considered constant and stored in the builder later. Expose a subset of dynamic states through a struct:

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

Encapsulate building pipelines into a class:

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

The implementation is quite verbose, splitting it into multiple functions helps a bit:

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

Finally, `App::draw()`:

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
