# Project Layout

本頁說明這份教學中所使用的程式碼結構。 這些安排只是本教學採用的一種主觀選擇，與 Vulkan 的使用方式本身無關

外部的依賴庫會被打包成一個 zip 檔，並在 CMake 的 configure 階段中被解壓縮。 你也可以考慮將 FetchContent 作為替代方案

預設使用的建構器（generator）是 `Ninja Multi-Config`，不論哪個作業系統或編譯器都是如此。 這會在專案根目錄下的 `CMakePresets.json` 中設定。 你也可以透過 `CMakeUserPresets.json` 加入自定的 preset 設定

> 在 Windows 上，Visual Studio 的 CMake 模式會使用這個建構器並自動載入 preset。 若使用 Visual Studio Code，CMake Tools 擴充套件也會自動使用這些 preset。 至於其他 IDE 則請參考它們對於 CMake preset 的使用說明

**檔案系統結構**

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
