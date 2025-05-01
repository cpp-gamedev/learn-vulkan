# 셰이더 오브젝트

[Vulkan의 그래픽스 파이프라인](https://docs.vulkan.org/spec/latest/chapters/pipelines.html)은 전체 렌더링 과정을 아우르는 거대한 객체로, `draw()` 호출 한 번에 여러 단계를 수행합니다. 하지만 [`VK_EXT_shader_object`](https://www.khronos.org/blog/you-can-use-vulkan-without-pipelines-today)라는 확장을 사용하면, 이러한 그래픽스 파이프라인 자체를 완전히 생략할 수 있습니다. 이 확장을 사용할 경우 대부분의 파이프라인 상태가 동적으로 설정되며, 그리는 시점에 설정됩니다. 이때 개발자가 직접 다뤄야할 Vulkan 핸들은 `ShaderEXT` 객체뿐입니다. 더 자세한 정보는 [여기](https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/extensions/shader_object)를 참고하세요.

Vulkan에서는 셰이더 코드를 SPIR-V 형태로 제공해야 합니다. 우리는 Vulkan SDK에 포함된 `glslc`를 사용해 GLSL 코드를 필요할 때 수동으로 SPIR-V 파일로 컴파일하겠습니다.
