# 이미지

이미지는 버퍼보다 훨씬 더 많은 속성과 생성 파라미터를 가지고 있습니다. 여기서는 두 종류로 나누겠습니다. 셰이더에서 샘플링될 이미지(텍스쳐), 그리고 렌더링에 사용할 깊이 이미지입니다. 지금은 이러한 이미지를 위한 기본 타입과 함수만 추가하겠습니다.

```cpp
struct RawImage {
  auto operator==(RawImage const& rhs) const -> bool = default;

  VmaAllocator allocator{};
  VmaAllocation allocation{};
  vk::Image image{};
  vk::Extent2D extent{};
  vk::Format format{};
  std::uint32_t levels{};
};

struct ImageDeleter {
  void operator()(RawImage const& raw_image) const noexcept;
};

using Image = Scoped<RawImage, ImageDeleter>;

struct ImageCreateInfo {
  VmaAllocator allocator;
  std::uint32_t queue_family;
};

[[nodiscard]] auto create_image(ImageCreateInfo const& create_info,
                                vk::ImageUsageFlags usage, std::uint32_t levels,
                                vk::Format format, vk::Extent2D extent)
  -> Image;
```

구현은 다음과 같습니다.

```cpp
void ImageDeleter::operator()(RawImage const& raw_image) const noexcept {
  vmaDestroyImage(raw_image.allocator, raw_image.image, raw_image.allocation);
}

// ...
auto vma::create_image(ImageCreateInfo const& create_info,
                       vk::ImageUsageFlags const usage,
                       std::uint32_t const levels, vk::Format const format,
                       vk::Extent2D const extent) -> Image {
  if (extent.width == 0 || extent.height == 0) {
    std::println(stderr, "Images cannot have 0 width or height");
    return {};
  }
  auto image_ci = vk::ImageCreateInfo{};
  image_ci.setImageType(vk::ImageType::e2D)
    .setExtent({extent.width, extent.height, 1})
    .setFormat(format)
    .setUsage(usage)
    .setArrayLayers(1)
    .setMipLevels(levels)
    .setSamples(vk::SampleCountFlagBits::e1)
    .setTiling(vk::ImageTiling::eOptimal)
    .setInitialLayout(vk::ImageLayout::eUndefined)
    .setQueueFamilyIndices(create_info.queue_family);
  auto const vk_image_ci = static_cast<VkImageCreateInfo>(image_ci);

  auto allocation_ci = VmaAllocationCreateInfo{};
  allocation_ci.usage = VMA_MEMORY_USAGE_AUTO;
  VkImage image{};
  VmaAllocation allocation{};
  auto const result = vmaCreateImage(create_info.allocator, &vk_image_ci,
                     &allocation_ci, &image, &allocation, {});
  if (result != VK_SUCCESS) {
    std::println(stderr, "Failed to create VMA Image");
    return {};
  }

  return RawImage{
    .allocator = create_info.allocator,
    .allocation = allocation,
    .image = image,
    .extent = extent,
    .format = format,
    .levels = levels,
  };
}
```

샘플링할 이미지(텍스쳐)를 생성하기 위해 이미지 바이트 데이터와 크기(extent)가 필요합니다. 이를 구조체로 감싸 사용하겠습니다.

```cpp
struct Bitmap {
  std::span<std::byte const> bytes{};
  glm::ivec2 size{};
};
```

생성 과정은 디바이스 버퍼와 유사합니다. 스테이징 버퍼를 복사하고, 레이아웃 전환을 수행해야 합니다. 요약하면 다음과 같습니다.

1. 이미지와 스테이징 버퍼를 생성합니다.
2. 이미지의 레이아웃을 Undefined에서 TransferDst로 전환합니다.
3. 버퍼에서 이미지로 복사하는 명령을 기록합니다.
4. 이미지의 레이아웃을 TransferDst에서 ShaderReadOnlyOptimal로 변경합니다.

```cpp
auto vma::create_sampled_image(ImageCreateInfo const& create_info,
                               CommandBlock command_block, Bitmap const& bitmap)
  -> Image {
  // create image.
  // no mip-mapping right now: 1 level.
  auto const mip_levels = 1u;
  auto const usize = glm::uvec2{bitmap.size};
  auto const extent = vk::Extent2D{usize.x, usize.y};
  auto const usage =
    vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
  auto ret = create_image(create_info, usage, mip_levels,
              vk::Format::eR8G8B8A8Srgb, extent);

  // create staging buffer.
  auto const buffer_ci = BufferCreateInfo{
    .allocator = create_info.allocator,
    .usage = vk::BufferUsageFlagBits::eTransferSrc,
    .queue_family = create_info.queue_family,
  };
  auto const staging_buffer = create_buffer(buffer_ci, BufferMemoryType::Host,
                        bitmap.bytes.size_bytes());

  // can't do anything if either creation failed.
  if (!ret.get().image || !staging_buffer.get().buffer) { return {}; }

  // copy bytes into staging buffer.
  std::memcpy(staging_buffer.get().mapped, bitmap.bytes.data(),
        bitmap.bytes.size_bytes());

  // transition image for transfer.
  auto dependency_info = vk::DependencyInfo{};
  auto subresource_range = vk::ImageSubresourceRange{};
  subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
    .setLayerCount(1)
    .setLevelCount(mip_levels);
  auto barrier = vk::ImageMemoryBarrier2{};
  barrier.setImage(ret.get().image)
    .setSrcQueueFamilyIndex(create_info.queue_family)
    .setDstQueueFamilyIndex(create_info.queue_family)
    .setOldLayout(vk::ImageLayout::eUndefined)
    .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
    .setSubresourceRange(subresource_range)
    .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
    .setSrcAccessMask(vk::AccessFlagBits2::eNone)
    .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
    .setDstAccessMask(vk::AccessFlagBits2::eMemoryRead |
              vk::AccessFlagBits2::eMemoryWrite);
  dependency_info.setImageMemoryBarriers(barrier);
  command_block.command_buffer().pipelineBarrier2(dependency_info);

  // record buffer image copy.
  auto buffer_image_copy = vk::BufferImageCopy2{};
  auto subresource_layers = vk::ImageSubresourceLayers{};
  subresource_layers.setAspectMask(vk::ImageAspectFlagBits::eColor)
    .setLayerCount(1)
    .setLayerCount(mip_levels);
  buffer_image_copy.setImageSubresource(subresource_layers)
    .setImageExtent(vk::Extent3D{extent.width, extent.height, 1});
  auto copy_info = vk::CopyBufferToImageInfo2{};
  copy_info.setDstImage(ret.get().image)
    .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
    .setSrcBuffer(staging_buffer.get().buffer)
    .setRegions(buffer_image_copy);
  command_block.command_buffer().copyBufferToImage2(copy_info);

  // transition image for sampling.
  barrier.setOldLayout(barrier.newLayout)
    .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
    .setSrcStageMask(barrier.dstStageMask)
    .setSrcAccessMask(barrier.dstAccessMask)
    .setDstStageMask(vk::PipelineStageFlagBits2::eAllGraphics)
    .setDstAccessMask(vk::AccessFlagBits2::eMemoryRead |
              vk::AccessFlagBits2::eMemoryWrite);
  dependency_info.setImageMemoryBarriers(barrier);
  command_block.command_buffer().pipelineBarrier2(dependency_info);

  command_block.submit_and_wait();

  return ret;
}
```

이미지를 텍스쳐로 사용하기 전에 디스크립터 셋을 구성해야 합니다.
