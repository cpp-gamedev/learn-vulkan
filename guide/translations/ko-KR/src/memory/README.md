# 메모리 할당

명시적인 API인 Vulkan에서는 디바이스가 사용할 메모리를 애플리케이션이 직접 [메모리 할당](https://docs.vulkan.org/guide/latest/memory_allocation.html)을 해야 합니다. 이 과정은 다소 복잡할 수 있기 때문에, Vulkan 사양에서 권장하듯이 이 모든 세부 사항을 [Vulkan Memory Allocator (VMA)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)에 맡겨 간단하게 처리할 것입니다.

Vulkan은 할당된 메모리를 사용하는 두 가지 객체 유형을 제공합니다. 버퍼와 이미지입니다. VMA는 이 둘에 대해 투명한(transparent) 지원을 제공합니다. 우리는 그저 버퍼와 이미지를 디바이스를 통해 직접 할당하는 대신, VMA를 통해 할당 및 해제하면 됩니다. CPU에서의 메모리 할당과 객체 생성과는 달리, Vulkan에는 버퍼와 이미지 생성에는 정렬(alignment)이나 크기(size) 외에도 훨씬 더 많은 파라미터가 필요합니다. 예상하셨겠지만, 여기서는 정점 버퍼, 유니폼/스토리지 버퍼, 그리고 텍스쳐 이미지와 같은 셰이더 자원과 관련된 하위 집합만을 다룰 예정입니다.
