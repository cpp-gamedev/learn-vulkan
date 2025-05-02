# (中文 WIP) Application

`class App` will serve as the owner and driver of the entire application. While there will only be one instance, using a class enables us to leverage RAII and destroy all its resources automatically and in the correct order, and avoids the need for globals.

```cpp
// app.hpp
namespace lvk {
class App {
 public:
  void run();
};
} // namespace lvk

// app.cpp
namespace lvk {
void App::run() {
  // TODO
}
} // namespace lvk
```

## Main

`main.cpp` will not do much: it's mainly responsible for transferring control to the actual entry point, and catching fatal exceptions.

```cpp
// main.cpp
auto main() -> int {
  try {
    lvk::App{}.run();
  } catch (std::exception const& e) {
    std::println(stderr, "PANIC: {}", e.what());
    return EXIT_FAILURE;
  } catch (...) {
    std::println("PANIC!");
    return EXIT_FAILURE;
  }
}
```
