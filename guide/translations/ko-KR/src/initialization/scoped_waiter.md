# Scoped Waiter

소멸자에서 디바이스가 Idle한 상태가 될 때까지 기다리거나 블록하는 객체는 매우 유용한 추상화입니다. GPU가 Vulkan 객체를 사용 중일 때 해당 객체를 파괴하는 것은 잘못된 사용입니다. 이 객체는 의존성이 있는 자원이 파괴되기 전에 디바이스가 idle 상태임을 보장하는 데 도움이 됩니다.

스코프가 끝날 때 임의의 작업을 수행할 수 있는 기능은 다른 곳에서도 유용할 수 있기 때문에, 이를 기본 템플릿 클래스 `Scoped`로 캡슐화합니다. 이 클래스는 포인터 타입 `Type*` 대신에 값 `Type`을 담는 `unique_ptr<Type, Deleter>`와 유사하지만, 다음과 같은 제약이 있습니다.

1. `Type`은 기본 생성자가 있어야 합니다.
2. 기본 생성자를 통한 `Type`은 null과 동일하다고 가정하며, 이 경우 `Deleter`를 호출하지 않습니다.

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

이 내용이 이해가 되지 않더라도 걱정하지 마세요. 구현 자체는 중요하지 않고, 이 객체가 무엇을 하는지, 그리고 어떻게 사용하는지가 중요합니다.

`ScopeWaiter`는 이제 비교적 간단하게 구현할 수 있습니다.

```cpp
struct ScopedWaiterDeleter {
  void operator()(vk::Device const device) const noexcept {
    device.waitIdle();
  }
};

using ScopedWaiter = Scoped<vk::Device, ScopedWaiterDeleter>;
```

`ScopeWaiter` 멤버를 `App`의 멤버 리스트 맨 마지막에 추가하세요. 이는 반드시 마지막에 선언되어야 하며, 그렇게 해야 이 멤버가 가장 먼저 파괴되기 때문에 다른 멤버들이 파괴되기 전에 idle 상태가 되는 것을 보장할 수 있습니다. 이를 디바이스 생성 후에 초기화합니다.

```cpp
m_waiter = *m_device;
```
