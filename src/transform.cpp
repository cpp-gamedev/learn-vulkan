#include <glm/gtc/matrix_transform.hpp>
#include <transform.hpp>

namespace lvk {
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
} // namespace lvk
