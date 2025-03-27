# Switching Pipelines

We can use an ImGui window to inspect / tweak some pipeline state:

```cpp
ImGui::ShowDemoWindow();

ImGui::SetNextWindowSize({200.0f, 100.0f}, ImGuiCond_Once);
if (ImGui::Begin("Inspect")) {
  ImGui::Checkbox("wireframe", &m_wireframe);
  if (m_wireframe) {
    auto const& line_width_range =
      m_gpu.properties.limits.lineWidthRange;
    ImGui::SetNextItemWidth(100.0f);
    ImGui::DragFloat("line width", &m_line_width, 0.25f,
              line_width_range[0], line_width_range[1]);
  }
}
ImGui::End();
```

Modify `draw()` to use the selected pipeline, and also set the line width:

```cpp
auto const pipeline =
  m_wireframe ? *m_pipelines.wireframe : *m_pipelines.standard;
command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

// ...
// line width is also a dynamic state in our pipelines, but setting it is
// not required (defaults to 1.0f or the minimum reported limit).
command_buffer.setLineWidth(m_line_width);
```

And that's it.

![sRGB Triangle (wireframe)](./srgb_triangle_wireframe.png)

In a system with dynamic pipeline creation, the first frame where `m_wireframe == true` will trigger the complex pipeline building step. This can cause stutters, especially if many different shaders/pipelines are involved in a short period of "experience" time. This is why many modern games/engines have the dreaded "Building/Compiling Shaders" screen, where all the pipelines are being built and cached.
