# Summary

[소개](README.md)

# 기초

- [시작하기](getting_started/README.md)
  - [프로젝트 레이아웃](getting_started/project_layout.md)
  - [검증 레이어](getting_started/validation_layers.md)
  - [class App](getting_started/class_app.md)
- [초기화](initialization/README.md)
  - [GLFW Window](initialization/glfw_window.md)
  - [Vulkan 인스턴스](initialization/instance.md)
  - [Vulkan Surface](initialization/surface.md)
  - [Vulkan 물리 디바이스](initialization/gpu.md)
  - [Vulkan 디바이스](initialization/device.md)
  - [Scoped Waiter](initialization/scoped_waiter.md)
  - [스왑체인](initialization/swapchain.md)

# Hello Triangle

- [렌더링](rendering/README.md)
  - [스왑체인 루프](rendering/swapchain_loop.md)
  - [렌더 싱크](rendering/render_sync.md)
  - [스왑체인 업데이트](rendering/swapchain_update.md)
  - [동적 렌더링](rendering/dynamic_rendering.md)
- [Dear ImGui](dear_imgui/README.md)
  - [class DearImGui](dear_imgui/dear_imgui.md)
  - [ImGui 통합](dear_imgui/imgui_integration.md)
- [셰이더 오브젝트](shader_objects/README.md)
  - [에셋 위치](shader_objects/locating_assets.md)
  - [셰이더 프로그램](shader_objects/shader_program.md)
  - [GLSL 에서 SPIR-V](shader_objects/glsl_to_spir_v.md)
  - [삼각형 그리기](shader_objects/drawing_triangle.md)
  - [그래픽스 파이프라인](shader_objects/pipelines.md)

# 셰이더 자원

- [메모리 할당](memory/README.md)
  - [Vulkan Memory Allocator](memory/vma.md)
  - [버퍼](memory/buffers.md)
  - [정점 버퍼](memory/vertex_buffer.md)
  - [Command Block](memory/command_block.md)
  - [디바이스 버퍼](memory/device_buffers.md)
  - [이미지](memory/images.md)
- [디스크립터 셋](descriptor_sets/README.md)
  - [파이프라인 레이아웃](descriptor_sets/pipeline_layout.md)
  - [Descriptor Buffer](descriptor_sets/descriptor_buffer.md)
  - [텍스쳐](descriptor_sets/texture.md)
  - [뷰 행렬](descriptor_sets/view_matrix.md)
  - [인스턴스 렌더링](descriptor_sets/instanced_rendering.md)
