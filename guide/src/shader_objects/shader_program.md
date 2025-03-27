# Shader Program

SPIR-V modules are binary files with a stride/alignment of 4 bytes. The Vulkan API accepts a span of `std::uint32_t`s, so we need to load it into such a buffer (and _not_ `std::vector<std::byte>` or other 1-byte equivalents). We will encapsulate both vertex and fragment shaders into a single `ShaderProgram`, which will also bind the shaders before a draw, and expose/set various dynamic states.

In `shader_program.hpp`, first add a `ShaderProgramCreateInfo` struct:

```cpp
struct ShaderProgramCreateInfo {
  vk::Device device;
  std::span<std::uint32_t const> vertex_spirv;
  std::span<std::uint32_t const> fragment_spirv;
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
  static auto const create_shader_ci =
    [](std::span<std::uint32_t const> spirv) {
      auto ret = vk::ShaderCreateInfoEXT{};
      ret.setCodeSize(spirv.size_bytes())
        .setPCode(spirv.data())
        // set common parameters.
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

## TODO: MOVE

Add new members to `App`:

```cpp
void create_pipelines();

[[nodiscard]] auto asset_path(std::string_view uri) const -> fs::path;
```

Add code to load shaders in `create_pipelines()` and call it before starting the main loop:

```cpp
void App::create_pipelines() {
  auto shader_loader = ShaderLoader{*m_device};
  // we only need shader modules to create the pipelines, thus no need to
  // store them as members.
  auto const vertex = shader_loader.load(asset_path("shader.vert"));
  auto const fragment = shader_loader.load(asset_path("shader.frag"));
  if (!vertex || !fragment) {
    throw std::runtime_error{"Failed to load Shaders"};
  }
  std::println("[lvk] Shaders loaded");

  // TODO
}

auto App::asset_path(std::string_view const uri) const -> fs::path {
  return m_assets_dir / uri;
}
```
