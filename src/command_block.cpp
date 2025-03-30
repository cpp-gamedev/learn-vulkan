#include <command_block.hpp>
#include <chrono>
#include <print>

namespace lvk {
using namespace std::chrono_literals;

CommandBlock::CommandBlock(vk::Device const device, vk::Queue const queue,
						   vk::CommandPool const command_pool)
	: m_device(device), m_queue(queue) {
	// allocate a UniqueCommandBuffer which will free the underlying command
	// buffer from its owning pool on destruction.
	auto allocate_info = vk::CommandBufferAllocateInfo{};
	allocate_info.setCommandPool(command_pool)
		.setCommandBufferCount(1)
		.setLevel(vk::CommandBufferLevel::ePrimary);
	// all the current VulkanHPP functions for UniqueCommandBuffer allocation
	// return vectors.
	auto command_buffers = m_device.allocateCommandBuffersUnique(allocate_info);
	m_command_buffer = std::move(command_buffers.front());

	// start recording commands before returning.
	auto begin_info = vk::CommandBufferBeginInfo{};
	begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	m_command_buffer->begin(begin_info);
}

void CommandBlock::submit_and_wait() {
	if (!m_command_buffer) { return; }

	// end recording and submit.
	m_command_buffer->end();
	auto submit_info = vk::SubmitInfo2KHR{};
	auto const command_buffer_info =
		vk::CommandBufferSubmitInfo{*m_command_buffer};
	submit_info.setCommandBufferInfos(command_buffer_info);
	auto fence = m_device.createFenceUnique({});
	m_queue.submit2(submit_info, *fence);

	// wait for submit fence to be signaled.
	static constexpr auto timeout_v =
		static_cast<std::uint64_t>(std::chrono::nanoseconds(30s).count());
	auto const result = m_device.waitForFences(*fence, vk::True, timeout_v);
	if (result != vk::Result::eSuccess) {
		std::println(stderr, "Failed to submit Command Buffer");
	}
	// free the command buffer.
	m_command_buffer.reset();
}
} // namespace lvk
