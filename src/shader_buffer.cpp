#include <shader_buffer.hpp>

namespace lvk {
ShaderBuffer::ShaderBuffer(VmaAllocator allocator,
						   std::uint32_t const queue_family,
						   vk::BufferUsageFlags const usage)
	: m_allocator(allocator), m_queue_family(queue_family), m_usage(usage) {
	// ensure buffers are created and can be bound after returning.
	for (auto& buffer : m_buffers) { write_to(buffer, {}); }
}

void ShaderBuffer::write_at(std::size_t const frame_index,
							std::span<std::byte const> bytes) {
	write_to(m_buffers.at(frame_index), bytes);
}

auto ShaderBuffer::descriptor_info_at(std::size_t const frame_index) const
	-> vk::DescriptorBufferInfo {
	auto const& buffer = m_buffers.at(frame_index);
	auto ret = vk::DescriptorBufferInfo{};
	ret.setBuffer(buffer.buffer.get().buffer).setRange(buffer.size);
	return ret;
}

void ShaderBuffer::write_to(Buffer& out,
							std::span<std::byte const> bytes) const {
	static constexpr auto blank_byte_v = std::array{std::byte{}};
	// fallback to an empty byte if bytes is empty.
	if (bytes.empty()) { bytes = blank_byte_v; }
	out.size = bytes.size();
	if (out.buffer.get().size < bytes.size()) {
		// size is too small (or buffer doesn't exist yet), recreate buffer.
		auto const buffer_ci = vma::BufferCreateInfo{
			.allocator = m_allocator,
			.usage = m_usage,
			.queue_family = m_queue_family,
		};
		out.buffer = vma::create_buffer(buffer_ci, vma::BufferMemoryType::Host,
										out.size);
	}
	std::memcpy(out.buffer.get().mapped, bytes.data(), bytes.size());
}
} // namespace lvk
