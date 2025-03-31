#include <texture.hpp>
#include <array>

namespace lvk {
namespace {
// 4-channels.
constexpr auto white_pixel_v = std::array{std::byte{0xff}, std::byte{0xff},
										  std::byte{0xff}, std::byte{0xff}};
// fallback bitmap.
constexpr auto white_bitmap_v = Bitmap{
	.bytes = white_pixel_v,
	.size = {1, 1},
};
} // namespace

Texture::Texture(CreateInfo create_info) {
	if (create_info.bitmap.bytes.empty() || create_info.bitmap.size.x <= 0 ||
		create_info.bitmap.size.y <= 0) {
		create_info.bitmap = white_bitmap_v;
	}

	auto const image_ci = vma::ImageCreateInfo{
		.allocator = create_info.allocator,
		.queue_family = create_info.queue_family,
	};
	m_image = vma::create_sampled_image(
		image_ci, std::move(create_info.command_block), create_info.bitmap);

	auto image_view_ci = vk::ImageViewCreateInfo{};
	auto subresource_range = vk::ImageSubresourceRange{};
	subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setLayerCount(1)
		.setLevelCount(m_image.get().levels);

	image_view_ci.setImage(m_image.get().image)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(m_image.get().format)
		.setSubresourceRange(subresource_range);
	m_view = create_info.device.createImageViewUnique(image_view_ci);

	m_sampler = create_info.device.createSamplerUnique(create_info.sampler);
}

auto Texture::descriptor_info() const -> vk::DescriptorImageInfo {
	auto ret = vk::DescriptorImageInfo{};
	ret.setImageView(*m_view)
		.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
		.setSampler(*m_sampler);
	return ret;
}
} // namespace lvk
