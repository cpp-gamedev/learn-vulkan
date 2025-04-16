# Shader Objects

A [Vulkan Graphics Pipeline](https://docs.vulkan.org/spec/latest/chapters/pipelines.html) is a large object that encompasses the entire graphics pipeline. It consists of many stages - all this happens during a single `draw()` call. There is however an extension called [`VK_EXT_shader_object`](https://www.khronos.org/blog/you-can-use-vulkan-without-pipelines-today) which enables avoiding graphics pipelines entirely. Almost all pipeline state becomes dynamic, ie set at draw time, and the only Vulkan handles to own are `ShaderEXT` objects. For a comprehensive guide, check out the [Vulkan Sample from Khronos](https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/extensions/shader_object).

Vulkan requires shader code to be provided as SPIR-V (IR). We shall use `glslc` (part of the Vulkan SDK) to compile GLSL to SPIR-V manually when required.
