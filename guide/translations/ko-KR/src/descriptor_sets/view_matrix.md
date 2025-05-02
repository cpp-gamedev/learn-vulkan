# 뷰 행렬

뷰 행렬을 통합하는 작업은 꽤 간단합니다. 먼저, 오브젝트와 카메라/뷰의 변환 정보를 하나의 구조체로 캡슐화합니다.

```cpp
struct Transform {
  glm::vec2 position{};
  float rotation{};
  glm::vec2 scale{1.0f};

  [[nodiscard]] auto model_matrix() const -> glm::mat4;
  [[nodiscard]] auto view_matrix() const -> glm::mat4;
};
```

공통된 로직을 함수로 사용하도록 두 가지 멤버 함수를 추가하겠습니다.

```cpp
namespace {
struct Matrices {
  glm::mat4 translation;
  glm::mat4 orientation;
  glm::mat4 scale;
};

[[nodiscard]] auto to_matrices(glm::vec2 const position, float rotation,
                 glm::vec2 const scale) -> Matrices {
  static constexpr auto mat_v = glm::identity<glm::mat4>();
  static constexpr auto axis_v = glm::vec3{0.0f, 0.0f, 1.0f};
  return Matrices{
    .translation = glm::translate(mat_v, glm::vec3{position, 0.0f}),
    .orientation = glm::rotate(mat_v, glm::radians(rotation), axis_v),
    .scale = glm::scale(mat_v, glm::vec3{scale, 1.0f}),
  };
}
} // namespace

auto Transform::model_matrix() const -> glm::mat4 {
  auto const [t, r, s] = to_matrices(position, rotation, scale);
  // right to left: scale first, then rotate, then translate.
  return t * r * s;
}

auto Transform::view_matrix() const -> glm::mat4 {
  // view matrix is the inverse of the model matrix.
  // instead, perform translation and rotation in reverse order and with
  // negative values. or, use glm::lookAt().
  //  scale is kept unchanged as the first transformation for
  // "intuitive" scaling on cameras.
  auto const [t, r, s] = to_matrices(-position, -rotation, scale);
  return r * t * s;
}
```

`App`에 `Transform` 멤버를 추가하여 뷰/카메라를 나타내고, 해당 멤버를 확인하여 기존의 프로젝션 행렬과 결합합니다.

```cpp
Transform m_view_transform{}; // generates view matrix.

// ...
ImGui::Separator();
if (ImGui::TreeNode("View")) {
  ImGui::DragFloat2("position", &m_view_transform.position.x);
  ImGui::DragFloat("rotation", &m_view_transform.rotation);
  ImGui::DragFloat2("scale", &m_view_transform.scale.x);
  ImGui::TreePop();
}

// ...
auto const mat_view = m_view_transform.view_matrix();
auto const mat_vp = mat_projection * mat_view;
auto const bytes =
  std::bit_cast<std::array<std::byte, sizeof(mat_vp)>>(mat_vp);
m_view_ubo->write_at(m_frame_index, bytes);
```

자연스럽게 뷰를 왼쪽으로 이동하면 현재는 사각형 하나뿐이지만 이것이 오른쪽으로 이동한 것으로 보일 것입니다.

![View Matrix](./view_matrix.png)
