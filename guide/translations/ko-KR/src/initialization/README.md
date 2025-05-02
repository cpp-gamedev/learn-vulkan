# 초기화

여기서는 다음을 포함하여, 애플리케이션 실행에 필요한 모든 시스템의 초기화 과정을 다룹니다.

- GLFW를 초기화하고 창 생성하기
- Vulkan Instance 생성하기
- Vulkan Surface 생성하기
- Vulkan Physical Device 선택하기
- Vulkan logical Device 생성하기
- Vulkan Swapchain 생성하기

여기서 어느 한 단계라도 실패하면, 그 이후에는 의미 있는 작업을 진행할 수 없기 때문에 치명적인 오류로 간주됩니다.
