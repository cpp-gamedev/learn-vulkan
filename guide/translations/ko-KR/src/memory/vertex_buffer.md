# 정점 버퍼

여기서의 목표는 셰이더에 하드코딩되어 있던 정점 정보를 애플리케이션 코드로 옮기는 것입니다. 당분간은 임시로 호스트 메모리에 위치한 `vma::Buffer`를 사용하고, 정점 속성과 같은 나머지 구조에 더 집중하겠습니다.

먼저 새로운 헤더 `vertex.hpp`를 추가합니다.

```cpp
struct Vertex {
  glm::vec2 position{};
  glm::vec3 color{1.0f};
};

// two vertex attributes: position at 0, color at 1.
constexpr auto vertex_attributes_v = std::array{
  // the format matches the type and layout of data: vec2 => 2x 32-bit floats.
  vk::VertexInputAttributeDescription2EXT{0, 0, vk::Format::eR32G32Sfloat,
                      offsetof(Vertex, position)},
  // vec3 => 3x 32-bit floats
  vk::VertexInputAttributeDescription2EXT{1, 0, vk::Format::eR32G32B32Sfloat,
                      offsetof(Vertex, color)},
};

// one vertex binding at location 0.
constexpr auto vertex_bindings_v = std::array{
  // we are using interleaved data with a stride of sizeof(Vertex).
  vk::VertexInputBindingDescription2EXT{0, sizeof(Vertex),
                      vk::VertexInputRate::eVertex, 1},
};
```

ShaderCreateInfo에 정점 속성과 바인딩 정보를 추가합니다.

```cpp
// ...
static constexpr auto vertex_input_v = ShaderVertexInput{
  .attributes = vertex_attributes_v,
  .bindings = vertex_bindings_v,
};
auto const shader_ci = ShaderProgram::CreateInfo{
  .device = *m_device,
  .vertex_spirv = vertex_spirv,
  .fragment_spirv = fragment_spirv,
  .vertex_input = vertex_input_v,
  .set_layouts = {},
};
// ...
```

정점 입력이 정의되었으므로 정점 셰이더를 업데이트하고 다시 컴파일합니다.

```glsl
#version 450 core

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec3 a_color;

layout (location = 0) out vec3 out_color;

void main() {
  const vec2 position = a_pos;

  out_color = a_color;
  gl_Position = vec4(position, 0.0, 1.0);
}
```

VBO(Vertex Buffer Object) 멤버를 추가하고, 해당 버퍼를 생성합니다.

```cpp
void App::create_vertex_buffer() {
  // vertices moved from the shader.
  static constexpr auto vertices_v = std::array{
    Vertex{.position = {-0.5f, -0.5f}, .color = {1.0f, 0.0f, 0.0f}},
    Vertex{.position = {0.5f, -0.5f}, .color = {0.0f, 1.0f, 0.0f}},
    Vertex{.position = {0.0f, 0.5f}, .color = {0.0f, 0.0f, 1.0f}},
  };

  // we want to write vertices_v to a Host VertexBuffer.
  auto const buffer_ci = vma::BufferCreateInfo{
    .allocator = m_allocator.get(),
    .usage = vk::BufferUsageFlagBits::eVertexBuffer,
    .queue_family = m_gpu.queue_family,
  };
  m_vbo = vma::create_buffer(buffer_ci, vma::BufferMemoryType::Host,
                             sizeof(vertices_v));

  // host buffers have a memory-mapped pointer available to memcpy data to.
  std::memcpy(m_vbo.get().mapped, vertices_v.data(), sizeof(vertices_v));
}
```

드로우 콜을 기록하기 전에 VBO를 바인딩합니다.

```cpp
// single VBO at binding 0 at no offset.
command_buffer.bindVertexBuffers(0, m_vbo->get_raw().buffer,
                                 vk::DeviceSize{});
// m_vbo has 3 vertices.
command_buffer.draw(3, 1, 0, 0);
```

아마 이전과 동일한 삼각형을 볼 수 있을 것입니다. 하지만 이제는 원하는 정점 데이터를 자유롭게 사용할 수 있습니다. 프리미티브 토폴로지는 기본적으로 Triangle List로 설정하며, 정점 배열에서 매 3개의 정점이 삼각형으로 그려질 것입니다. 예를 들어 정점 9개가 `[[0, 1, 2], [3, 4, 5], [6, 7, 8]]` 있다면, 각 3개의 정점이 하나의 삼각형을 형성하게 됩니다. 정점 데이터와 토폴로지를 다양하게 바꿔보며 실험해 보세요. 예상치 못한 출력이나 버그가 발생한다면 RenderDoc을 사용해 디버깅할 수 있습니다.

호스트 정점 버퍼는 UI 객체처럼 임시로 쓰이거나 자주 변경되는 프리미티브에 유용합니다. 2D 프레임워크에서는 이러한 VBO를 독점적으로 사용하는 것도 가능합니다. 예를 들어, 가상 프레임마다 별도의 버퍼 풀을 두고, 각 드로우마다 현재 프레임의 풀에서 버퍼를 하나 가져와 정점을 복사하는 방식이 단순하면서도 효과적입니다.
