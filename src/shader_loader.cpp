#include <shader_loader.hpp>
#include <fstream>
#include <print>

namespace lvk {
auto ShaderLoader::load(fs::path const& path) -> vk::UniqueShaderModule {
	// open the file at the end, to get the total size.
	auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
	if (!file.is_open()) {
		std::println(stderr, "Failed to open file: '{}'",
					 path.generic_string());
		return {};
	}

	auto const size = file.tellg();
	auto const usize = static_cast<std::uint64_t>(size);
	// file data must be uint32 aligned.
	if (usize % sizeof(std::uint32_t) != 0) {
		std::println(stderr, "Invalid SPIR-V size: {}", usize);
		return {};
	}

	// seek to the beginning before reading.
	file.seekg({}, std::ios::beg);
	m_code.resize(usize / sizeof(std::uint32_t));
	void* data = m_code.data();
	file.read(static_cast<char*>(data), size);

	auto shader_module_ci = vk::ShaderModuleCreateInfo{};
	shader_module_ci.setCode(m_code);
	return m_device.createShaderModuleUnique(shader_module_ci);
}
} // namespace lvk
