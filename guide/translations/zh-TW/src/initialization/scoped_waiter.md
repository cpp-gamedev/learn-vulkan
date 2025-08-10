# Scoped Waiter

有個實用的抽象做法是讓物件在解構子中等待／阻塞，直到 Device 進入閒置狀態。 因為你不能在 GPU 仍在使用某些 Vulkan 物件時就銷毀它們，這麼做的話，就能在銷毀相依資源之前，確保裝置已進入閒置狀態

能在離開作用域時執行任意動作，在其他情境也很有用，因此我們會把這個概念封裝成一個基礎的類別模板 `Scoped`。 它有點像 `unique_ptr<Type, Deleter>`，但儲存的是值（`Type`），而不是指標（`Type*`），並有以下限制：

1. `Type` 必須可預設建構（default constructible）
2. 視預設建構得到的 `Type` 等同於 `null`（因此不會呼叫 `Deleter`）

```cpp
template <typename Type>
concept Scopeable =
  std::equality_comparable<Type> && std::is_default_constructible_v<Type>;

template <Scopeable Type, typename Deleter>
class Scoped {
 public:
  Scoped(Scoped const&) = delete;
  auto operator=(Scoped const&) = delete;

  Scoped() = default;

  constexpr Scoped(Scoped&& rhs) noexcept
    : m_t(std::exchange(rhs.m_t, Type{})) {}

  constexpr auto operator=(Scoped&& rhs) noexcept -> Scoped& {
    if (&rhs != this) { std::swap(m_t, rhs.m_t); }
    return *this;
  }

  explicit(false) constexpr Scoped(Type t) : m_t(std::move(t)) {}

  constexpr ~Scoped() {
    if (m_t == Type{}) { return; }
    Deleter{}(m_t);
  }

  [[nodiscard]] constexpr auto get() const -> Type const& { return m_t; }
  [[nodiscard]] constexpr auto get() -> Type& { return m_t; }

 private:
  Type m_t{};
};
```

這看起來不太直覺，但不用擔心，實作細節並不重要，重要的是它的作用以及如何使用它

現在我們可以很容易地實作一個 `ScopedWaiter` 了：

```cpp
struct ScopedWaiterDeleter {
  void operator()(vk::Device const device) const noexcept {
    device.waitIdle();
  }
};

using ScopedWaiter = Scoped<vk::Device, ScopedWaiterDeleter>;
```

接著在 `App` 成員列表的「最後面」新增一個 `ScopedWaiter` 成員：它必須放在最後，這樣它會成為第一個被解構的成員，從而確保在解構其他成員之前，Device 已經進入閒置狀態了。 建立 Device 後初始化它：

```cpp
m_waiter = *m_device;
```
