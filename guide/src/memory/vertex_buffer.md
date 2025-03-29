# Vertex Buffer

The goal here is to move the hard-coded vertices in the shader to application code. For the time being we will use an ad-hoc host `vma::Buffer` and focus more on the rest of the infrastructure like vertex attributes.

First add a new header, `vertex.hpp`:

```cpp
struct Vertex {
  glm::vec2 position{};
  glm::vec3 color{};
};
```

In `app.cpp`, store the vertex input:

```cpp
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

constexpr auto vertex_input_v = ShaderVertexInput{
  .attributes = vertex_attributes_v,
  .bindings = vertex_bindings_v,
};
```

Add `vertex_input_v` to the Shader Create Info:

```cpp
auto const shader_ci = ShaderProgram::CreateInfo{
  // ...
  .vertex_input = vertex_input_v,
  .set_layouts = {},
};
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
  // we want to write 3x Vertex objects to a Host VertexBuffer.
  auto const buffer_ci = vma::Buffer::CreateInfo{
    .usage = vk::BufferUsageFlagBits::eVertexBuffer,
    .size = 3 * sizeof(Vertex),
    .type = vma::BufferType::Host,
  };
  m_vbo.emplace(m_allocator.get(), buffer_ci);

  // vertices that were previously hard-coded in the shader.
  static constexpr auto vertices_v = std::array{
    Vertex{.position = {-0.5f, -0.5f}, .color = {1.0f, 0.0f, 0.0f}},
    Vertex{.position = {0.5f, -0.5f}, .color = {0.0f, 1.0f, 0.0f}},
    Vertex{.position = {0.0f, 0.5f}, .color = {0.0f, 0.0f, 1.0f}},
  };
  // host buffers have a memory-mapped pointer available to memcpy data to.
  std::memcpy(m_vbo->get_raw().mapped, vertices_v.data(), sizeof(vertices_v));
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

You should see the same triangle as before. But now we can use whatever set of vertices we like! To change it to a quad / rectangle, let's utilize an index buffer: this will reduce vertex duplication in general.

```cpp
void App::create_vertex_buffer() {
  // we want to write 4x Vertex objects to a Host VertexBuffer.
  auto buffer_ci = vma::Buffer::CreateInfo{
    .usage = vk::BufferUsageFlagBits::eVertexBuffer,
    .size = 4 * sizeof(Vertex),
    .type = vma::BufferType::Host,
  };
  m_vbo.emplace(m_allocator.get(), buffer_ci);

  // vertices that form a quad.
  static constexpr auto vertices_v = std::array{
    Vertex{.position = {-0.5f, -0.5f}, .color = {1.0f, 0.0f, 0.0f}},
    Vertex{.position = {0.5f, -0.5f}, .color = {0.0f, 1.0f, 0.0f}},
    Vertex{.position = {0.5f, 0.5f}, .color = {0.0f, 0.0f, 1.0f}},
    Vertex{.position = {-0.5f, 0.5f}, .color = {1.0f, 1.0f, 0.0f}},
  };
  // host buffers have a memory-mapped pointer available to memcpy data to.
  std::memcpy(m_vbo->get_raw().mapped, vertices_v.data(), sizeof(vertices_v));

  // prepare to write 6x u32 indices to a Host IndexBuffer.
  buffer_ci = {
    .usage = vk::BufferUsageFlagBits::eIndexBuffer,
    .size = 6 * sizeof(std::uint32_t),
    .type = vma::BufferType::Host,
  };
  m_ibo.emplace(m_allocator.get(), buffer_ci);
  static constexpr auto indices_v = std::array{
    0u, 1u, 2u, 2u, 3u, 0u,
  };
  std::memcpy(m_ibo->get_raw().mapped, indices_v.data(), sizeof(indices_v));
}
```

Bind the index buffer and use the `drawIndexed()` command:

```cpp
// single VBO at binding 0 at no offset.
command_buffer.bindVertexBuffers(0, m_vbo->get_raw().buffer,
                                 vk::DeviceSize{});
// IBO with u32 indices at no offset.
command_buffer.bindIndexBuffer(m_ibo->get_raw().buffer, 0,
                               vk::IndexType::eUint32);
// m_ibo has 6 indices.
command_buffer.drawIndexed(6, 1, 0, 0, 0);
```
