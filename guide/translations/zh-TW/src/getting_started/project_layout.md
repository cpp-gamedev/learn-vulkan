# (中文 WIP) Project Layout

This page describes the layout used by the code in this guide. Everything here is just an opinionated option used by the guide, and is not related to Vulkan usage.

External dependencies are stuffed into a zip file that's decompressed by CMake during the configure stage. Using FetchContent is a viable alternative.

`Ninja Multi-Config` is the assumed generator used, regardless of OS/compiler. This is set up in a `CMakePresets.json` file in the project root. Additional custom presets can be added via `CMakeUserPresets.json`.

> On Windows, Visual Studio CMake Mode uses this generator and automatically loads presets. With Visual Studio Code, the CMake Tools extension automatically uses presets. For other IDEs, refer to their documentation on using CMake presets.

**Filesystem**

```
.
|-- CMakeLists.txt         <== executable target
|-- CMakePresets.json
|-- [other project files]
|-- ext/
│   |-- CMakeLists.txt     <== external dependencies target
|-- src/
    |-- [sources and headers]
```