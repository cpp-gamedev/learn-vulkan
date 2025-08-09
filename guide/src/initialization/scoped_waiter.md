# Scoped Waiter

A useful abstraction to have is an object that in its destructor waits/blocks until the Device is idle. It is incorrect usage to destroy Vulkan objects while they are in use by the GPU, such an object helps with making sure the device is idle before some dependent resource gets destroyed.

Being able to do arbitrary things on scope exit will be useful in other spots too, so we encapsulate that in a basic class template `Scoped`. It's somewhat like a `unique_ptr<Type, Deleter>` that stores the value (`Type`) instead of a pointer (`Type*`), with some constraints:

1. `Type` must be default constructible
1. Assumes a default constructed `Type` is equivalent to null (does not call `Deleter`)

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

Don't worry if this doesn't make a lot of sense: the implementation isn't important, what it does and how to use it is what matters.

A `ScopedWaiter` can now be implemented quite easily:

```cpp
struct ScopedWaiterDeleter {
  void operator()(vk::Device const device) const noexcept {
    device.waitIdle();
  }
};

using ScopedWaiter = Scoped<vk::Device, ScopedWaiterDeleter>;
```

Add a `ScopedWaiter` member to `App` _at the end_ of its member list: this must remain at the end to be the first member that gets destroyed, thus guaranteeing the device will be idle before the destruction of any other members begins. Initialize it after creating the Device:

```cpp
m_waiter = *m_device;
```
