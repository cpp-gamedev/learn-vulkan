#pragma once
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace lvk {
namespace fs = std::filesystem;

class ShaderLoader {
  public:
	explicit ShaderLoader(vk::Device const device) : m_device(device) {}

	[[nodiscard]] auto load(fs::path const& path) -> vk::UniqueShaderModule;

  private:
	vk::Device m_device{};
	std::vector<std::uint32_t> m_code{};
};
} // namespace lvk
