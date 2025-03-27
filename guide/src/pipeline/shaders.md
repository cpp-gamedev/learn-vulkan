# Shaders

Shaders work in NDC space: -1 to +1 for X and Y. We set up a triangle's coordinates and output that in the vertex shader save it to `src/glsl/shader.vert`:

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

## Shader Modules

SPIR-V modules are binary files with a stride/alignment of 4 bytes. The Vulkan API accepts a span of `std::uint32_t`s, so we need to load it into such a buffer (and _not_ `std::vector<std::byte>` or other 1-byte equivalents).

Add a new `class ShaderLoader`:

```cpp
class ShaderLoader {
 public:
  explicit ShaderLoader(vk::Device const device) : m_device(device) {}

  [[nodiscard]] auto load(fs::path const& path) -> vk::UniqueShaderModule;

 private:
  vk::Device m_device{};
  std::vector<std::uint32_t> m_code{};
};
```

Implement `load()`:

```cpp
auto ShaderLoader::load(fs::path const& path) -> vk::UniqueShaderModule {
  // open the file at the end, to get the total size.
  auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
  if (!file.is_open()) {
    std::println(stderr, "Failed to open file: '{}'",
                 path.generic_string());
    return {};
  }

  auto const size = file.tellg();
  auto const usize = static_cast<std::uint64_t>(size);
  // file data must be uint32 aligned.
  if (usize % sizeof(std::uint32_t) != 0) {
    std::println(stderr, "Invalid SPIR-V size: {}", usize);
    return {};
  }

  // seek to the beginning before reading.
  file.seekg({}, std::ios::beg);
  m_code.resize(usize / sizeof(std::uint32_t));
  void* data = m_code.data();
  file.read(static_cast<char*>(data), size);

  auto shader_module_ci = vk::ShaderModuleCreateInfo{};
  shader_module_ci.setCode(m_code);
  return m_device.createShaderModuleUnique(shader_module_ci);
}
```

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
