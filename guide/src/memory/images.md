# Images

Images have a lot more properties and creation parameters than buffers. We shall constrain ourselves to just two kinds: sampled images (textures) for shaders, and depth images for rendering. For now add the foundation types and functions:

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

Implementation:

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

For creating sampled images, we need both the image bytes and size (extent). Wrap that into a struct:

```cpp
struct Bitmap {
  std::span<std::byte const> bytes{};
  glm::ivec2 size{};
};
```

The creation process is similar to device buffers: requiring a staging copy, but it also needs layout transitions. In short:

1. Create the image and staging buffer
1. Transition the layout from Undefined to TransferDst
1. Record a buffer image copy operation
1. Transition the layout from TransferDst to ShaderReadOnlyOptimal

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
