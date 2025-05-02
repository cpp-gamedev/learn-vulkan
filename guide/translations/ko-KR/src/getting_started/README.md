# 시작하기

Vulkan은 플랫폼에 독립적인 API입니다. 이로 인해 다양한 구현을 포괄해야 하므로, 다소 장황해질 수 밖에 없습니다. 이 가이드에서는 Windows와 Linux(x64 혹은 aarch64)에 집중하고, 외장 GPU를 중심으로 설명할 것입니다. 이를 통해 Vulkan의 복잡함을 어느 정도 완화할 수 있습니다. Vulkan 1.3은 우리가 다룰 데스크탑 플랫폼과 대부분의 최신 그래픽 카드에서 널리 지원됩니다.

> 이것이 내장 GPU를 지원하지 않는다는 뜻은 아닙니다. 다만 특별히 그에 맞게 설계하거나 최적화하지는 않습니다.

## 요구사항

1. Vulkan 1.3 버전 이상을 지원하는 GPU와 로더
1. [Vulkan 1.3 이상의 SDK](https://vulkan.lunarg.com/sdk/home)
    1. 이는 검증 레이어에 필요하며, Vulkan 애플리케이션 개발 시 중요한 구성요소입니다. 단, 프로젝트 자체는 SDK에 직접 의존하지 않습니다.
    1. 항상 최신 SDK 사용이 권장됩니다. (이 문서를 작성하는 시점 기준으로는 1.4)
1. Vulkan을 기본적으로 지원하는 데스크탑 운영체제
    1. Windows 혹은 최신 패키지를 제공하는 Linux 배포판 사용이 권장됩니다.
    2. MacOS는 Vulkan을 기본적으로 지원하지 않습니다. MoltenVk를 통해 사용할 수는 있으나, 작성 시점 기준 MoltenVk는 Vulkan 1.3을 완전히 지원하지 않기 때문에, 이 환경에서는 제약이 있을 수 있습니다.
2. C++23을 지원하는 컴파일러 및 표준 라이브러리
    1. GCC14+, Clang18+, 최신 MSVC가 권장되며, MinGW/MSYS는 권장되지 않습니다.
    2. C++20을 사용하고 C++23의 특정 기능을 대체하는 것도 가능합니다.(예: `std::print()` 대신 `fmt::print()` 사용, 람다식에 `()` 추가 등)
3. CMake 3.24 이상

## 개요

C++ 모듈에 대한 지원은 점차 확대되고 있지만, 우리가 목표로 하는 모든 플랫폼과 IDE에서는 아직 완전히 지원되지 않습니다. 따라서 당분간은 헤더 파일을 사용할 수 밖에 없습니다. 이는 가까운 미래에 도구들이 개선되면, 가이드를 리팩토링하면서 변경될 수 있습니다.

이 프로젝트는 "Build the World" 접근 방식을 사용합니다. 이는 Sanitizer 사용을 가능하게 하고, 모든 지원 플랫폼에서 재현 가능한 빌드를 제공하며, 대상 시스템에서의 사전 설치 요구사항을 최소화합니다. 물론, 미리 빌드된 바이너리를 사용하는 것도 가능하며, Vulkan을 사용하는 방식에는 영향을 주지 않습니다.

## 라이브러리

1. 창과 입력, Surface를 위한 [GLFW](https://github.com/glfw/glfw)
1. Vulkan과 상호작용하기 위한 [VulkanHPP](https://github.com/KhronosGroup/Vulkan-Hpp), [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers)를 통해 사용.
    1. Vulkan은 C API이지만, 공식 C++ 래핑 라이브러리가 제공되어 많은 편리한 기능을 제공합니다. 이 가이드는 C API를 사용하는 다른 라이브러리들(예: GLFW,VMA)을 사용할 때를 제외하고는 거의 C++만 사용합니다.
2. Vulkan 메모리 힙을 다루기 위한 [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/)
3. C++에서 GLSL과 유사한 선형대수학을 위한 [GLM](https://github.com/g-truc/glm)
4. UI를 위한 [Dear ImGui](https://github.com/ocornut/imgui)
