# Application

`class App` 會作為整個應用程式的擁有者與執行者。 雖然其實際上只會建立一個實例，但使用 class 可以讓我們善用 RAII，自動且按正確順序釋放所有資源，並避免使用全域變數

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

`main.cpp` 不會負責太多事：它主要的工作是將控制權交給真正的進入點，並攔截致命的例外錯誤

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
