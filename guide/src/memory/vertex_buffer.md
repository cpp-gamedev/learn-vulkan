# Vertex Buffer

The goal here is to move the hard-coded vertices in the shader to application code. For the time being we will use an ad-hoc Host `vma::Buffer` and focus more on the rest of the infrastructure like vertex attributes.

First add a new header, `vertex.hpp`:

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

Add the vertex attributes and bindings to the Shader Create Info:

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

With the vertex input defined, we can update the vertex shader and recompile it:

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

Add a VBO (Vertex Buffer Object) member and create it:

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

Bind the VBO before recording the draw call:

```cpp
// single VBO at binding 0 at no offset.
command_buffer.bindVertexBuffers(0, m_vbo->get_raw().buffer,
                                 vk::DeviceSize{});
// m_vbo has 3 vertices.
command_buffer.draw(3, 1, 0, 0);
```

You should see the same triangle as before. But now we can use whatever set of vertices we like! The Primitive Topology is Triange List by default, so every three vertices in the array is drawn as a triangle, eg for 9 vertices: `[[0, 1, 2], [3, 4, 5], [6, 7, 8]]`, where each inner `[]` represents a triangle comprised of the vertices at those indices. Try playing around with customized vertices and topologies, use Render Doc to debug unexpected outputs / bugs.

Host Vertex Buffers are useful for primitives that are temporary and/or frequently changing, such as UI objects. A 2D framework can use such VBOs exclusively: a simple approach would be a pool of buffers per virtual frame where for each draw a buffer is obtained from the current virtual frame's pool and vertices are copied in.
