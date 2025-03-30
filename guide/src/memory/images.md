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
