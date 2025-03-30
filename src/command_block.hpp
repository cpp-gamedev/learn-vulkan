#pragma once
#include <vulkan/vulkan.hpp>

namespace lvk {
class CommandBlock {
  public:
	explicit CommandBlock(vk::Device device, vk::Queue queue,
						  vk::CommandPool command_pool);

	[[nodiscard]] auto command_buffer() const -> vk::CommandBuffer {
		return *m_command_buffer;
	}

	void submit_and_wait();

  private:
	vk::Device m_device{};
	vk::Queue m_queue{};
	vk::UniqueCommandBuffer m_command_buffer{};
};
} // namespace lvk
