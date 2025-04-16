# Application

`class App`은 전체 애플리케이션의 소유자이자 실행 주체 역할을 합니다. 인스턴스는 하나뿐이지만, 클래스를 사용함으로써 RAII의 이점을 활용해 자원을 올바른 순서로 자동 해제할 수 있으며, 전역 변수를 사용할 필요도 없습니다.

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

`main.cpp`는 많은 역할을 하지 않습니다. 주로 실제 진입점으로 제어를 넘기고, 치명적인 예외를 처리하는 역할을 합니다.

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
