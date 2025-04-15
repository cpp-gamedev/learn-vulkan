# GLSL to SPIR-V

Shaders work in NDC space: -1 to +1 for X and Y. We output a triangle's coordinates in a new vertex shader and save it to `src/glsl/shader.vert`:

```glsl
#version 450 core

void main() {
  const vec2 positions[] = {
    vec2(-0.5, -0.5),
    vec2(0.5, -0.5),
    vec2(0.0, 0.5),
  };

  const vec2 position = positions[gl_VertexIndex];

  gl_Position = vec4(position, 0.0, 1.0);
}
```

The fragment shader just outputs white for now, in `src/glsl/shader.frag`:

```glsl
#version 450 core

layout (location = 0) out vec4 out_color;

void main() {
  out_color = vec4(1.0);
}
```

Compile both shaders into `assets/`:

```
glslc src/glsl/shader.vert -o assets/shader.vert
glslc src/glsl/shader.frag -o assets/shader.frag
```

> glslc is part of the Vulkan SDK.

## Loading SPIR-V

SPIR-V shaders are binary files with a stride/alignment of 4 bytes. As we have seen, the Vulkan API accepts a span of `std::uint32_t`s, so we need to load it into such a buffer (and _not_ `std::vector<std::byte>` or other 1-byte equivalents). Add a helper function in `app.cpp`:

```cpp
[[nodiscard]] auto to_spir_v(fs::path const& path)
  -> std::vector<std::uint32_t> {
  // open the file at the end, to get the total size.
  auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
  if (!file.is_open()) {
    throw std::runtime_error{
      std::format("Failed to open file: '{}'", path.generic_string())};
  }

  auto const size = file.tellg();
  auto const usize = static_cast<std::uint64_t>(size);
  // file data must be uint32 aligned.
  if (usize % sizeof(std::uint32_t) != 0) {
    throw std::runtime_error{std::format("Invalid SPIR-V size: {}", usize)};
  }

  // seek to the beginning before reading.
  file.seekg({}, std::ios::beg);
  auto ret = std::vector<std::uint32_t>{};
  ret.resize(usize / sizeof(std::uint32_t));
  void* data = ret.data();
  file.read(static_cast<char*>(data), size);
  return ret;
}
```
