# Dear ImGui

Dear ImGui does not have native CMake support, and while adding the sources to the executable is an option, we will add it as an external library target: `imgui` to isolate it (and compile warnings etc) from our own code. This requires some changes to the `ext` target structure, since `imgui` will itself need to link to GLFW and Vulkan-Headers, have `VK_NO_PROTOTYPES` defined, etc. `learn-vk-ext` then links to `imgui` and any other libraries (currently only `glm`). We are using Dear ImGui v1.91.9, which has decent support for Dynamic Rendering.
