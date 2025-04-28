# 셰이더 프로그램

셰이더 오브젝트를 사용하려면 디바이스 생성 시 대응되는 기능과 확장을 활성화해야 합니다.

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

## 에뮬레이션 레이어

현재 사용중인 드라이버나 물리 디바이스가 `VK_EXT_shader_object`를 지원하지 않을 수 있기 때문에(특히 인텔에서 자주 발생합니다) 디바이스 생성이 실패할 수 있습니다. Vulkan SDK는 이 확장을 구현하는 레이어 [`VK_LAYER_KHRONOS_shader_object`](https://github.com/KhronosGroup/Vulkan-ExtensionLayer/blob/main/docs/shader_object_layer.md)를 제공합니다. 이 레이어를 InstanceCreateInfo에 추가하면 해당 기능을 사용할 수 있습니다.


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
이 레이어는 표준 Vulkan 드라이버 설치에 포함되어 있지 <em>않기</em> 때문에, Vulkan SDK나 Vulkan Configurator가 없는 환경에서도 실행할 수 있도록 애플리케이션과 함께 패키징해야 합니다. 자세한 내용은<a href="https://docs.vulkan.org/samples/latest/samples/extensions/shader_object/README.html#_emulation_layer">여기</a>를 참고하세요.
</div>

원하는 레이어가 사용 불가능할 수 있으므로, 이를 확인하는 코드를 추가하는 것이 좋습니다.

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

## `class ShaderProgram`

정점 셰이더와 프래그먼트 셰이더를 하나의 `ShaderProgram`으로 캡슐화하여, 그리기 전에 셰이더를 바인딩하고 다양한 동적 상태를 설정할 수 있도록 하겠습니다.

`shader_program.hpp`에서 가장 먼저 `ShaderProgramCreateInfo` 구조체를 추가합니다.

```cpp
struct ShaderProgramCreateInfo {
  vk::Device device;
  std::span<std::uint32_t const> vertex_spirv;
  std::span<std::uint32_t const> fragment_spirv;
  std::span<vk::DescriptorSetLayout const> set_layouts;
};
```

> 디스크립터 셋과 레이아웃은 이후에 다루겠습니다.

간단한 형태의 정의부터 시작합니다.

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

생성자의 정의는 꽤 단순합니다.

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

몇 가지 동적 상태를 public 멤버를 통해 나타냅니다.

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

불 값을 비트 플래그로 캡슐화합니다.

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

파이프라인 상태에 필요한 요소가 하나 남아있습니다. 정점 입력입니다. 이는 셰이더마다 고정된 값이 될 것이므로 생성자에 저장할 것입니다.

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
  // ...
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

바인딩할 API는 Viewport와 Scissor 설정을 위해 커맨드 버퍼와 프레임 버퍼 크기를 받습니다

```cpp
void bind(vk::CommandBuffer command_buffer,
          glm::ivec2 framebuffer_size) const;
```

멤버 함수를 추가하고 순차적으로 이를 호출하여 `bind()`를 구현합니다.

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

구현은 길지만 꽤 단순합니다.

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
