# GLSL 에서 SPIR-V

Shaders work in NDC space: -1 to +1 for X and Y. We output a triangle's coordinates in a new vertex shader and save it to `src/glsl/shader.vert`:
셰이더는 NDC 공간 X축과 Y축에서 -1에서 1까지 작동합니다. 새로운 정점 셰이더의 삼각형 좌표계를 출력하고 이를 `src/glsl/shader.vert`에 저장합니다.

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
`src/glsl/shader.frag`의 프래그먼트 셰이더는 지금은 흰 색을 출력하기만 할 것입니다.

```glsl
#version 450 core

layout (location = 0) out vec4 out_color;

void main() {
  out_color = vec4(1.0);
}
```

Compile both shaders into `assets/`:
이 둘을 `assets/`로 컴파일합니다.

```
glslc src/glsl/shader.vert -o assets/shader.vert
glslc src/glsl/shader.frag -o assets/shader.frag
```

> glslc is part of the Vulkan SDK.
> glslc는 Vulkan SDK의 일부입니다.

## SPIR-V 불러오기

SPIR-V shaders are binary files with a stride/alignment of 4 bytes. As we have seen, the Vulkan API accepts a span of `std::uint32_t`s, so we need to load it into such a buffer (and _not_ `std::vector<std::byte>` or other 1-byte equivalents). Add a helper function in `app.cpp`:
SPIR-V 셰이더는 4바이트 단위로 정렬이 되어있는 바이너리 파일입니다. 지금까지 봐왔던 대로, Vulkan API는 `std::uint32_t`의 묶음을 받습니다. 따라서 이러한 종류의 버퍼(단, `std::vector<std::byte>` 혹은 다른 종류의 1바이트 컨테이너는 아닙니다)에 담습니다. 이를 돕는 함수를 `app.cpp`에 추가합니다.

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
