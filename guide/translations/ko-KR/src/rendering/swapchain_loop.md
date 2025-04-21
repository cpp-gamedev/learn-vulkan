# 스왑체인 루프

렌더링 루프의 핵심 요소 중 하나는 스왑체인 루프입니다. 이는 다음과 같은 고수준 단계로 구성됩니다.

1. 스왑체인으로부터 이미지를 받아옵니다.
2. 받아온 이미지에 렌더링합니다.
3. 렌더링이 끝난 이미지를 표시합니다(이미지를 다시 스왑체인으로 돌려줍니다).

![WSI Engine](./wsi_engine.png)

여기서 몇 가지 고려해야할 점이 있습니다.

1. 이미지를 받아오거나 표시하는 과정은 실패할 수 있습니다(스왑체인을 사용할 수 없는 경우). 이 때 남은 단계들은 생략해야 합니다.
2. 받아오는 명령은 이미지가 실제로 사용할 준비가 되기 전에 반환될 수 있으며, 렌더링은 해당 이미지를 받아온 이후에 시작하도록 동기화되어야 합니다.
3. 마찬가지로, 표시하는 작업 또한 렌더링이 끝난 이후에 수행되도록 동기화해야 합니다.
4. 이미지들은 각 단계에 맞는 적절한 레이아웃으로 전환되어야 합니다.

또한, 스왑체인의 이미지의 수는 시스템에 따라 달라질 수 있지만, 엔진은 일반적으로 고정된 개수의 가상 프레임을 사용합니다. 더블 버퍼링에는 2개의 가상 프레임, 트리플 버퍼링에는 3개(보통은 3개로 충분합니다). 자세한 내용은 [여기](https://docs.vulkan.org/samples/latest/samples/performance/swapchain_images/README.html#_double_buffering_or_triple_buffering)서 확인할 수 있습니다. 또한 스왑체인이 (Vsync라 알려진)Mailbox Present 모드를 사용중이라면 메인 루프 중에 이전 렌더링 명령이 끝나기 전 동일한 이미지를 가져오는 것도 가능합니다.

## 가상 프레임

프레임마다 사용되는 모든 동적 자원들은 가상 프레임에 포함됩니다. 애플리케이션은 고정된 개수의 가상 프레임을 가지고 있으며, 매 렌더 패스마다 이를 순환하며 사용합니다. 동기화를 위해 각 프레임은 이전 프레임의 렌더링이 끝날 때 까지 대기하게 만드는 [`vk::Fence`](https://docs.vulkan.org/spec/latest/chapters/synchronization.html#synchronization-fences)가 있어야 합니다. 또한 GPU에서의 이미지를 받아오고, 렌더링, 화면에 나타내는 작업을 동기화하기 위한 2개의[`vk::Semaphore`](https://docs.vulkan.org/spec/latest/chapters/synchronization.html#synchronization-semaphores)가 필요합니다(이 작업들은 CPU측에서 대기할 필요는 없습니다). 명령을 기록하기 위해 가상 프레임마다 [`vk::CommandBuffer`](https://docs.vulkan.org/spec/latest/chapters/cmdbuffers.html)를 두어 해당 프레임의 (레이아웃 전환을 포함한) 모든 렌더링 명령을 기록할 것입니다.

## 이미지 레이아웃


Vulkan 이미지에는 [이미지 레이아웃](https://docs.vulkan.org/spec/latest/chapters/resources.html#resources-image-layouts)이라 알려진 속성이 있습니다. 대부분의 이미지 작업과 이미지의 서브리소스는 특정 레이아웃에서만 수행될 수 있으므로, 작업 전후에 레이아웃 전환이 필요합니다. 레이아웃 전환은 파이프라인 배리어(GPU의 메모리 배리어를 생각하세요)역할도 수행하며, 전환 전후의 작업을 동기화할수 있게 합니다.

Vulkan 동기화는 아마도 API의 가장 복잡한 부분 중 하나일 것입니다. 충분한 학습이 권장되며, [이 글](https://gpuopen.com/learn/vulkan-barriers-explained/)에서 배리어에 대해 자세히 설명하고 있습니다.

