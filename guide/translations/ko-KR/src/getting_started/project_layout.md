# 프로젝트 레이아웃

이 페이지는 이 가이드에서 사용하는 코드 레이아웃에 대해 설명합니다. 여기서 설명하는 내용은 이 가이드에서의 참고사항일 뿐이며, Vulkan 사용과는 관련이 없습니다.

외부 의존성은 zip파일로 묶여있으며, CMake가 구성(configure) 단계에서 이를 압축 해제합니다. FetchContent를 사용하는 것도 유효한 대안입니다.

`Ninja Multi-Config`는 OS나 컴파일러에 관계없이 사용되는 기본 생성기로 가정합니다. 이는 프로젝트 루트 디렉토리에 있는 `CMakePresets.json`파일에 설정되어 있으며, 사용자 정의 프리셋은 `CMakeUserPresets.json`을 통해 추가할 수 있습니다.

> Windows에서는 Visual Studio의 CMake Mode가 이 생성기를 사용하며, 자동으로 프리셋을 불러옵니다. Visual Studio Code에서는 CMake Tools 확장이 자동으로 프리셋을 사용합니다. 그 외의 IDE에서는 CMake 프리셋 사용 방법에 대한 해당 IDE의 문서를 참고하세요.

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