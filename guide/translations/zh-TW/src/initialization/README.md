# Initialization

本節將處理所有必要系統的初始化工作，包括：

- 初始化 GLFW 並建立視窗
- 建立 Vulkan Instance
- 建立 Vulkan Surface
- 選擇 Vulkan Physical Device
- 建立 Vulkan logical Device
- 建立 Vulkan Swapchain

如果其中任何一步失敗，都會是致命錯誤，之後的操作都會變得沒有意義
