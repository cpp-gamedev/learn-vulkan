# Summary

[Introduction](README.md)

# Basics

- [Getting Started](getting_started/README.md)
  - [Project Layout](getting_started/project_layout.md)
  - [Validation Layers](getting_started/validation_layers.md)
  - [class App](getting_started/class_app.md)
- [Initialization](initialization/README.md)
  - [GLFW Window](initialization/glfw_window.md)
  - [Vulkan Instance](initialization/instance.md)
  - [Vulkan Surface](initialization/surface.md)
  - [Vulkan Physical Device](initialization/gpu.md)
  - [Vulkan Device](initialization/device.md)
  - [Scoped Waiter](initialization/scoped_waiter.md)
  - [Swapchain](initialization/swapchain.md)

# Hello Triangle

- [Rendering](rendering/README.md)
  - [Swapchain Loop](rendering/swapchain_loop.md)
  - [Render Sync](rendering/render_sync.md)
  - [Swapchain Update](rendering/swapchain_update.md)
  - [Dynamic Rendering](rendering/dynamic_rendering.md)
- [Dear ImGui](dear_imgui/README.md)
  - [class DearImGui](dear_imgui/dear_imgui.md)
  - [ImGui Integration](dear_imgui/imgui_integration.md)
- [Shader Objects](shader_objects/README.md)
  - [Locating Assets](shader_objects/locating_assets.md)
  - [Shader Program](shader_objects/shader_program.md)
  - [GLSL to SPIR-V](shader_objects/glsl_to_spir_v.md)
  - [Drawing a Triangle](shader_objects/drawing_triangle.md)
  - [Graphics Pipelines](shader_objects/pipelines.md)

# Shader Resources

- [Memory Allocation](memory/README.md)
  - [Vulkan Memory Allocator](memory/vma.md)
  - [Buffers](memory/buffers.md)
  - [Vertex Buffer](memory/vertex_buffer.md)
  - [Command Block](memory/command_block.md)
  - [Device Buffers](memory/device_buffers.md)
  - [Images](memory/images.md)
- [Descriptor Sets](descriptor_sets/README.md)
  - [Pipeline Layout](descriptor_sets/pipeline_layout.md)
