# 파이프라인 레이아웃

[Vulkan 파이프라인 레이아웃](https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineLayout.html)은 셰이더 프로그램과 연결된 디스크립터 셋과 푸시 상수를 나타냅니다. 셰이더 오브젝트를 사용할 경우에도 디스크립터 셋을 활용하기 위해 파이프라인 레이아웃이 필요합니다.

뷰/프로젝션 행렬을 담기 위한 유니폼 버퍼를 포함하는 단일 디스크립터 셋 레이아웃부터 시작합니다. 디스크립터 풀을 `App`에 추가하고 셰이더보다 먼저 생성합니다.

```cpp
vk::UniqueDescriptorPool m_descriptor_pool{};

// ...
void App::create_descriptor_pool() {
  static constexpr auto pool_sizes_v = std::array{
    // 2 uniform buffers, can be more if desired.
    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2},
  };
  auto pool_ci = vk::DescriptorPoolCreateInfo{};
  // allow 16 sets to be allocated from this pool.
  pool_ci.setPoolSizes(pool_sizes_v).setMaxSets(16);
  m_descriptor_pool = m_device->createDescriptorPoolUnique(pool_ci);
}
```

새로운 멤버를 `App`에 추가해 디스크립터 셋 레이아웃과 파이프라인 레이아웃을 담도록 합니다. `m_set_layout_views`는 디스크립터 셋 레이아웃의 핸들을 연속된 vector로 복사한 것입니다.

```cpp
std::vector<vk::UniqueDescriptorSetLayout> m_set_layouts{};
std::vector<vk::DescriptorSetLayout> m_set_layout_views{};
vk::UniquePipelineLayout m_pipeline_layout{};

// ...
constexpr auto layout_binding(std::uint32_t binding,
                              vk::DescriptorType const type) {
  return vk::DescriptorSetLayoutBinding{
    binding, type, 1, vk::ShaderStageFlagBits::eAllGraphics};
}

// ...
void App::create_pipeline_layout() {
  static constexpr auto set_0_bindings_v = std::array{
    layout_binding(0, vk::DescriptorType::eUniformBuffer),
  };
  auto set_layout_cis = std::array<vk::DescriptorSetLayoutCreateInfo, 1>{};
  set_layout_cis[0].setBindings(set_0_bindings_v);

  for (auto const& set_layout_ci : set_layout_cis) {
    m_set_layouts.push_back(
      m_device->createDescriptorSetLayoutUnique(set_layout_ci));
    m_set_layout_views.push_back(*m_set_layouts.back());
  }

  auto pipeline_layout_ci = vk::PipelineLayoutCreateInfo{};
  pipeline_layout_ci.setSetLayouts(m_set_layout_views);
  m_pipeline_layout =
    m_device->createPipelineLayoutUnique(pipeline_layout_ci);
}
```

레이아웃 전체에 해당하는 디스크립터 셋을 할당하는 함수를 추가합니다.

```cpp
auto App::allocate_sets() const -> std::vector<vk::DescriptorSet> {
  auto allocate_info = vk::DescriptorSetAllocateInfo{};
  allocate_info.setDescriptorPool(*m_descriptor_pool)
    .setSetLayouts(m_set_layout_views);
  return m_device->allocateDescriptorSets(allocate_info);
}
```

그릴 객체에 쓰일 디스크립터 셋을 저장합니다.

```cpp
Buffered<std::vector<vk::DescriptorSet>> m_descriptor_sets{};

// ...

void App::create_descriptor_sets() {
  for (auto& descriptor_sets : m_descriptor_sets) {
    descriptor_sets = allocate_sets();
  }
}
```
