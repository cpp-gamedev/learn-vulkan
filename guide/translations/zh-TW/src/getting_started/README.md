# Getting Started

Vulkan 是跨平台的 API，這也是它之所以冗長的一個主要原因：它的 API 必須涵蓋各式各樣的實作方式。 我們這裡會將範圍限縮在 Windows 和 Linux（x64 或 aarch64），並專注於獨立顯示卡，這樣可以避開不少冗長的細節，而且這些桌面平台與近幾代的顯示卡上都已廣泛支援 Vulkan 1.3 了

> 這並不代表像是整合型顯示晶片（integrated graphics）就不被支援，只是它們的設計或最佳化方向通常不是針對 Vulkan 而來

## Technical Requirements

1. 支援 Vulkan 1.3 以上的 GPU 與 loader
2. [Vulkan 1.3+ SDK](https://vulkan.lunarg.com/sdk/home)
   1. 在開發 Vulkan 的應用程式時 validation layer 是必要的組件/工具，但本專案本身不會直接使用 SDK
   2. 建議始終使用最新版的 SDK（撰寫本文時為 1.4.x）
3. 原生支援（natively support）Vulkan 的桌面作業系統
   1. 推薦使用 Windows 或有新套件庫的 Linux 發行版
   2. MacOS 並不原生支援 Vulkan。 雖然可以透過 MoltenVk 使用，但撰寫本文時 MoltenVk 尚未完整支援 Vulkan 1.3，因此如果你選擇走這條路，可能會遇到一些阻礙
4. 支援 C++23 的編譯器與標準函式庫
   1. 推薦使用 GCC14+、Clang18+ 或最新版本的 MSVC。 不推薦使用 MinGW/MSYS
   2. 也可以使用 C++20 並搭配替代方案來補齊 C++23 的功能，例如將 `std::print()` 改成 `fmt::print()`，或是在 lambda 後面加上 `()` 等等
5. CMake 3.24 以上的版本

## Overview

雖然 C++ modules 正慢慢的開始普及了，但於我們的目標平台與 IDE 上，其工具鏈仍不夠完善，因此目前仍會使用傳統的 header 檔。 未來這情況可能會改變，到時這份教學也會跟著重構

本專案採用「Build the World」的方式，這讓我們可以使用 sanitizer、在所有目標平台上都能重現編譯結果，並且不必於目標機器上預先安裝太多東西。 當然你也可以選擇使用預先編好的 binary，這不會影響你使用 Vulkan 的方式

## Dependencies

1. [GLFW](https://github.com/glfw/glfw) 用來建立視窗、處理輸入與建立 Vulkan Surface
2. [VulkanHPP](https://github.com/KhronosGroup/Vulkan-Hpp)（透過 [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers)）用來與 Vulkan 互動
    1. 雖然 Vulkan 是 C 的 API，但官方提供了 C++ 的 wrapper library，加入了許多提升開發體驗的功能。 本教學幾乎都使用 C++ 版本的包裝，只有在需要與其他使用 C API 的函式庫互動時（如 GLFW 和 VMA）才會使用原生的 C API
3. [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/) 用來處理 Vulkan 的記憶體配置與管理
4. [GLM](https://github.com/g-truc/glm) 提供類似 GLSL 的線性代數功能給 C++ 使用
5. [Dear ImGui](https://github.com/ocornut/imgui) 用來建立 UI 介面
