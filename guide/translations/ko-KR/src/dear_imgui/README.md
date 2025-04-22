# Dear ImGui

Dear ImGui는 네이티브 CMake를 지원하지 않기 때문에, 소스를 실행 파일에 직접 추가하는 방법도 있지만, 컴파일 경고 등에서 우리 코드와 분리하기 위해 외부 라이브러리 타겟인 `imgui`로 추가할 예정입니다. 이를 위해 `imgui`는 GLFW 및 Vulkan-Headers에 연결되어야 하고, `VK_NO_PROTOTYPES`도 정의되어야 하므로 `ext` 타겟 구조에 약간의 변경이 필요합니다. 이후 `learn-vk-ext`는 `imgui` 및 기타 라이브러리들(현재는 `glm`만 있음)과 연결됩니다. 우리는 동적 렌더링을 지원하는 Dear ImGui v1.91.9 버전을 사용할 예정입니다.
