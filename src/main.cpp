#include <app.hpp>
#include <exception>
#include <print>
#include <span>

auto main(int argc, char** argv) -> int {
	try {
		// skip the first argument.
		auto args = std::span{argv, static_cast<std::size_t>(argc)}.subspan(1);
		while (!args.empty()) {
			auto const arg = std::string_view{args.front()};
			if (arg == "-x" || arg == "--force-x11") {
				glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
			}
			args = args.subspan(1);
		}
		lvk::App{}.run();
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println("PANIC!");
		return EXIT_FAILURE;
	}
}
