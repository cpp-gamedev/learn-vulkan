# (中文 WIP) Getting Started

Vulkan is platform agnostic, which is one of the main reasons for its verbosity: it has to account for a wide range of implementations in its API. We shall be constraining our approach to Windows and Linux (x64 or aarch64), and focusing on discrete GPUs, enabing us to sidestep quite a bit of that verbosity. Vulkan 1.3 is widely supported by the target desktop platforms and reasonably recent graphics cards.

> This doesn't mean that eg an integrated graphics chip will not be supported, it will just not be particularly designed/optimized for.

## Technical Requirements

1. Vulkan 1.3+ capable GPU and loader
1. [Vulkan 1.3+ SDK](https://vulkan.lunarg.com/sdk/home)
    1. This is required for validation layers, a critical component/tool to use when developing Vulkan applications. The project itself does not use the SDK.
    1. Always using the latest SDK is recommended (1.4.x at the time of writing).
1. Desktop operating system that natively supports Vulkan
    1. Windows and/or Linux (distros that use repos with recent packages) is recommended.
    1. MacOS does _not_ natively support Vulkan. It _can_ be used through MoltenVk, but at the time of writing MoltenVk does not fully support Vulkan 1.3, so if you decide to take this route, you may face some roadblocks.
1. C++23 compiler and standard library
    1. GCC14+, Clang18+, and/or latest MSVC are recommended. MinGW/MSYS is _not_ recommended.
    1. Using C++20 with replacements for C++23 specific features is possible. Eg replace `std::print()` with `fmt::print()`, add `()` to lambdas, etc.
1. CMake 3.24+

## Overview

While support for C++ modules is steadily growing, tooling is not yet ready on all platforms/IDEs we want to target, so we will unfortuntely still be using headers. This might change in the near future, followed by a refactor of this guide.

The project uses a "Build the World" approach, enabling usage of sanitizers, reproducible builds on any supported platform, and requiring minimum pre-installed things on target machines. Feel free to use pre-built binaries instead, it doesn't change anything about how you would use Vulkan.

## Dependencies

1. [GLFW](https://github.com/glfw/glfw) for windowing, input, and Surface creation
1. [VulkanHPP](https://github.com/KhronosGroup/Vulkan-Hpp) (via [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers)) for interacting with Vulkan
    1. While Vulkan is a C API, it offers an official C++ wrapper library with many quality-of-life features. This guide almost exclusively uses that, except at the boundaries of other C libraries that themselves use the C API (eg GLFW and VMA).
1. [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/) for dealing with Vulkan memory heaps
1. [GLM](https://github.com/g-truc/glm) for GLSL-like linear algebra in C++
1. [Dear ImGui](https://github.com/ocornut/imgui) for UI
