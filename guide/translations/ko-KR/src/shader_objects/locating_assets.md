# 에셋 위치

셰이더를 사용하기 전에, 에셋 파일들을 불러와야 합니다. 이를 제대로 수행하려면 우선 에셋들이 위치한 경로를 알아야 합니다. 에셋 경로를 찾는 방법에는 여러 가지가 있지만, 우리는 현재 작업 디렉토리에서 시작하여 상위 디렉토리로 올라가며 특정 하위 폴더(`assets/`)를 찾는 방식을 사용할 것입니다. 이렇게 하면 프로젝트나 빌드 디렉토리 어디에서 `app`이 실행되더라도 `assets/` 디렉토리를 자동으로 찾아 접근할 수 있게 됩니다.

```
.
|-- assets/
|-- app
|-- build/
    |-- app
|-- out/
    |-- default/Release/
        |-- app
    |-- ubsan/Debug/
        |-- app
```

릴리즈 패키지에서는 일반적으로 실행 파일의 경로를 기준으로 에셋 경로를 설정하며, 상위 경로로 거슬러 올라가는 방식은 사용하지 않는 것이 보통입니다. 작업 경로에 상관없이 패키지에 포함된 에셋은 보통 실행 파일과 같은 위치나 그 주변에 위치하기 때문입니다.

## 에셋 경로
:
`App`에 `assets/` 경로를 담을 멤버를 추가합니다.

```cpp
namespace fs = std::filesystem;

// ...
fs::path m_assets_dir{};
```

에셋 경로를 찾는 함수를 추가하고, 그 반환값을 `run()` 함수 상단에서 `m_assets_dir`에 저장하세요.

```cpp
[[nodiscard]] auto locate_assets_dir() -> fs::path {
  // look for '<path>/assets/', starting from the working
  // directory and walking up the parent directory tree.
  static constexpr std::string_view dir_name_v{"assets"};
  for (auto path = fs::current_path();
     !path.empty() && path.has_parent_path(); path = path.parent_path()) {
    auto ret = path / dir_name_v;
    if (fs::is_directory(ret)) { return ret; }
  }
  std::println("[lvk] Warning: could not locate '{}' directory", dir_name_v);
  return fs::current_path();
}

// ...
m_assets_dir = locate_assets_dir();
```
