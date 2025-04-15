# Locating Assets

Before we can use shaders, we need to load them as asset/data files. To do that correctly, first the asset directory needs to be located. There are a few ways to go about this, we will use the approach of looking for a particular subdirectory, starting from the working directory and walking up the parent directory tree. This enables `app` in any project/build subdirectory to locate `assets/` in the various examples below:

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

In a release package you would want to use the path to the executable instead (and probably not perform an "upfind" walk), the working directory could be anywhere whereas assets shipped with the package will be in the vicinity of the executable.

## Assets Directory

Add a member to `App` to store this path to `assets/`:

```cpp
namespace fs = std::filesystem;

// ...
fs::path m_assets_dir{};
```

Add a helper function to locate the assets dir, and assign `m_assets_dir` to its return value at the top of `run()`:

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
