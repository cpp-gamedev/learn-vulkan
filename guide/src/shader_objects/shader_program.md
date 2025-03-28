# Shader Program

To use Shader Objects we need to enable the corresponding feature and extension during device creation:

```cpp
auto shader_object_feature =
  vk::PhysicalDeviceShaderObjectFeaturesEXT{vk::True};
dynamic_rendering_feature.setPNext(&shader_object_feature);

// ...
// we need two device extensions: Swapchain and Shader Object.
static constexpr auto extensions_v = std::array{
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  "VK_EXT_shader_object",
};
```

## Emulation Layer

It's possible device creation now fails because the driver or physical device does not support `VK_EXT_shader_object` (especially likely with Intel). Vulkan SDK provides a layer that implements this extension: [`VK_LAYER_KHRONOS_shader_object`](https://github.com/KhronosGroup/Vulkan-ExtensionLayer/blob/main/docs/shader_object_layer.md). Adding this layer to the Instance Create Info should unblock usage of this feature:

```cpp
// ...
// add the Shader Object emulation layer.
static constexpr auto layers_v = std::array{
  "VK_LAYER_KHRONOS_shader_object",
};
instance_ci.setPEnabledLayerNames(layers_v);

m_instance = vk::createInstanceUnique(instance_ci);
// ...
```

<div class="warning">
This layer <em>is not</em> part of standard Vulkan driver installs, you must package the layer with the application for it to run on environments without Vulkan SDK / Vulkan Configurator. Read more <a href="https://docs.vulkan.org/samples/latest/samples/extensions/shader_object/README.html#_emulation_layer">here</a>.
</div>

Since desired layers may not be available, we can set up a defensive check:

```cpp
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

// ...
auto const layers = get_layers(layers_v);
instance_ci.setPEnabledLayerNames(layers);
```

## `class ShaderObject`

We will encapsulate both vertex and fragment shaders into a single `ShaderProgram`, which will also bind the shaders before a draw, and expose/set various dynamic states.

In `shader_program.hpp`, first add a `ShaderProgramCreateInfo` struct:

```cpp
struct ShaderProgramCreateInfo {
  vk::Device device;
  std::span<std::uint32_t const> vertex_spirv;
  std::span<std::uint32_t const> fragment_spirv;
  std::span<vk::DescriptorSetLayout const> set_layouts;
};
```

Start with a skeleton definition:

```cpp
class ShaderProgram {
 public:
  using CreateInfo = ShaderProgramCreateInfo;

  explicit ShaderProgram(CreateInfo const& create_info);

 private:
  std::vector<vk::UniqueShaderEXT> m_shaders{};

  ScopedWaiter m_waiter{};
};
```

The definition of the constructor is fairly straightforward:

```cpp
ShaderProgram::ShaderProgram(CreateInfo const& create_info) {
  auto const create_shader_ci =
    [&create_info](std::span<std::uint32_t const> spirv) {
      auto ret = vk::ShaderCreateInfoEXT{};
      ret.setCodeSize(spirv.size_bytes())
        .setPCode(spirv.data())
        // set common parameters.
        .setSetLayouts(create_info.set_layouts)
        .setCodeType(vk::ShaderCodeTypeEXT::eSpirv)
        .setPName("main");
      return ret;
    };

  auto shader_cis = std::array{
    create_shader_ci(create_info.vertex_spirv),
    create_shader_ci(create_info.fragment_spirv),
  };
  shader_cis[0]
    .setStage(vk::ShaderStageFlagBits::eVertex)
    .setNextStage(vk::ShaderStageFlagBits::eFragment);
  shader_cis[1].setStage(vk::ShaderStageFlagBits::eFragment);

  auto result = create_info.device.createShadersEXTUnique(shader_cis);
  if (result.result != vk::Result::eSuccess) {
    throw std::runtime_error{"Failed to create Shader Objects"};
  }
  m_shaders = std::move(result.value);
  m_waiter = create_info.device;
}
```

Expose some dynamic states via public members:

```cpp
static constexpr auto color_blend_equation_v = [] {
  auto ret = vk::ColorBlendEquationEXT{};
  ret.setColorBlendOp(vk::BlendOp::eAdd)
    // standard alpha blending:
    // (alpha * src) + (1 - alpha) * dst
    .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
    .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
  return ret;
}();

// ...
vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};
float line_width{1.0f};
vk::ColorBlendEquationEXT color_blend_equation{color_blend_equation_v};
vk::CompareOp depth_compare_op{vk::CompareOp::eLessOrEqual};
```

Encapsulate booleans into bit flags:

```cpp
// bit flags for various binary states.
enum : std::uint8_t {
  None = 0,
  AlphaBlend = 1 << 0, // turn on alpha blending.
  DepthTest = 1 << 1,  // turn on depth write and test.
};

// ...
static constexpr auto flags_v = AlphaBlend | DepthTest;

// ...
std::uint8_t flags{flags_v};
```

There is one more piece of pipeline state needed: vertex input. We will consider this to be constant per shader and take it in the constructor:

```cpp
// shader_program.hpp

// vertex attributes and bindings.
struct ShaderVertexInput {
  std::span<vk::VertexInputAttributeDescription2EXT const> attributes{};
  std::span<vk::VertexInputBindingDescription2EXT const> bindings{};
};

struct ShaderProgramCreateInfo {
  // ...
 ShaderVertexInput vertex_input{};
};

class ShaderProgram {
  // ...
  ShaderVertexInput m_vertex_input{};
  std::vector<vk::UniqueShaderEXT> m_shaders{};
  // ...
};

// shader_program.cpp
ShaderProgram::ShaderProgram(CreateInfo const& create_info)
  : m_vertex_input(create_info.vertex_input) {
  // ...
}
```

The API to bind will take the command buffer and the framebuffer size (to set the viewport and scissor):

```cpp
void bind(vk::CommandBuffer command_buffer,
          glm::ivec2 framebuffer_size) const;
```

Add helper member functions and implement `bind()` by calling them in succession:

```cpp
static void set_viewport_scissor(vk::CommandBuffer command_buffer,
                                 glm::ivec2 framebuffer);
static void set_static_states(vk::CommandBuffer command_buffer);
void set_common_states(vk::CommandBuffer command_buffer) const;
void set_vertex_states(vk::CommandBuffer command_buffer) const;
void set_fragment_states(vk::CommandBuffer command_buffer) const;
void bind_shaders(vk::CommandBuffer command_buffer) const;

// ...
void ShaderProgram::bind(vk::CommandBuffer const command_buffer,
                         glm::ivec2 const framebuffer_size) const {
  set_viewport_scissor(command_buffer, framebuffer_size);
  set_static_states(command_buffer);
  set_common_states(command_buffer);
  set_vertex_states(command_buffer);
  set_fragment_states(command_buffer);
  bind_shaders(command_buffer);
}
```

Implementations are long but straightforward:

```cpp
namespace {
constexpr auto to_vkbool(bool const value) {
  return value ? vk::True : vk::False;
}
} // namespace

// ...
void ShaderProgram::set_viewport_scissor(vk::CommandBuffer const command_buffer,
                     glm::ivec2 const framebuffer_size) {
  auto const fsize = glm::vec2{framebuffer_size};
  auto viewport = vk::Viewport{};
  // flip the viewport about the X-axis (negative height):
  // https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
  viewport.setX(0.0f).setY(fsize.y).setWidth(fsize.x).setHeight(-fsize.y);
  command_buffer.setViewportWithCount(viewport);

  auto const usize = glm::uvec2{framebuffer_size};
  auto const scissor =
    vk::Rect2D{vk::Offset2D{}, vk::Extent2D{usize.x, usize.y}};
  command_buffer.setScissorWithCount(scissor);
}

void ShaderProgram::set_static_states(vk::CommandBuffer const command_buffer) {
  command_buffer.setRasterizerDiscardEnable(vk::False);
  command_buffer.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1);
  command_buffer.setSampleMaskEXT(vk::SampleCountFlagBits::e1, 0xff);
  command_buffer.setAlphaToCoverageEnableEXT(vk::False);
  command_buffer.setCullMode(vk::CullModeFlagBits::eNone);
  command_buffer.setFrontFace(vk::FrontFace::eCounterClockwise);
  command_buffer.setDepthBiasEnable(vk::False);
  command_buffer.setStencilTestEnable(vk::False);
  command_buffer.setPrimitiveRestartEnable(vk::False);
  command_buffer.setColorWriteMaskEXT(0, ~vk::ColorComponentFlags{});
}

void ShaderProgram::set_common_states(
  vk::CommandBuffer const command_buffer) const {
  auto const depth_test = to_vkbool((flags & DepthTest) == DepthTest);
  command_buffer.setDepthWriteEnable(depth_test);
  command_buffer.setDepthTestEnable(depth_test);
  command_buffer.setDepthCompareOp(depth_compare_op);
  command_buffer.setPolygonModeEXT(polygon_mode);
  command_buffer.setLineWidth(line_width);
}

void ShaderProgram::set_vertex_states(
  vk::CommandBuffer const command_buffer) const {
  command_buffer.setVertexInputEXT(m_vertex_input.bindings,
                   m_vertex_input.attributes);
  command_buffer.setPrimitiveTopology(topology);
}

void ShaderProgram::set_fragment_states(
  vk::CommandBuffer const command_buffer) const {
  auto const alpha_blend = to_vkbool((flags & AlphaBlend) == AlphaBlend);
  command_buffer.setColorBlendEnableEXT(0, alpha_blend);
  command_buffer.setColorBlendEquationEXT(0, color_blend_equation);
}

void ShaderProgram::bind_shaders(vk::CommandBuffer const command_buffer) const {
  static constexpr auto stages_v = std::array{
    vk::ShaderStageFlagBits::eVertex,
    vk::ShaderStageFlagBits::eFragment,
  };
  auto const shaders = std::array{
    *m_shaders[0],
    *m_shaders[1],
  };
  command_buffer.bindShadersEXT(stages_v, shaders);
}
```
