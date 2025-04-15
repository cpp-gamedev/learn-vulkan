# Learn Vulkan

[![Build status](https://github.com/cpp-gamedev/learn-vulkan/actions/workflows/ci.yml/badge.svg)](https://github.com/cpp-gamedev/learn-vulkan/actions/workflows/ci.yml)

This repository hosts the [learn-vulkan](https://cpp-gamedev.github.io/learn-vulkan/) guide's C++ source code. It also hosts the sources of the [guide](./guide) itself.

## Building

### Requirements

- CMake 3.24+
- C++23 compiler and standard library
- [Linux] [GLFW dependencies](https://www.glfw.org/docs/latest/compile_guide.html#compile_deps_wayland) for X11 and Wayland

### Steps

Standard CMake workflow. Using presets is recommended, in-source builds are not recommended. See the [CI script](.github/workflows/ci.yml) for building on the command line.

## Branches

1. `main`^: latest, stable code (builds and runs), stable history (never rewritten)
1. `production`^: guide deployment (live), stable code and history
1. `section/*`^: reflection of source at the end of corresponding section in the guide, stable code
1. `feature/*`: potential upcoming feature, shared contributions, stable history
1. others: unstable

_^ rejects direct pushes (PR required)_

[Original Repository](https://github.com/cpp-gamedev/learn-vulkan)
