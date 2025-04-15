# Pipeline Layout

A [Vulkan Pipeline Layout](https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineLayout.html) represents a sequence of descriptor sets (and push constants) associated with a shader program. Even when using Shader Objects, a Pipeline Layout is needed to utilize descriptor sets.

Starting with the layout of a single descriptor set containing a uniform buffer to set the view/projection matrices in, store a descriptor pool in `App` and create it before the shader:

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

Add new members to `App` to store the set layouts and pipeline layout. `m_set_layout_views` is just a copy of the descriptor set layout handles in a contiguous vector:

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

Add a helper function that allocates a set of descriptor sets for the entire layout:

```cpp
auto App::allocate_sets() const -> std::vector<vk::DescriptorSet> {
  auto allocate_info = vk::DescriptorSetAllocateInfo{};
  allocate_info.setDescriptorPool(*m_descriptor_pool)
    .setSetLayouts(m_set_layout_views);
  return m_device->allocateDescriptorSets(allocate_info);
}
```

Store a Buffered copy of descriptor sets for one drawable object:

```cpp
Buffered<std::vector<vk::DescriptorSet>> m_descriptor_sets{};

// ...

void App::create_descriptor_sets() {
  for (auto& descriptor_sets : m_descriptor_sets) {
    descriptor_sets = allocate_sets();
  }
}
```
