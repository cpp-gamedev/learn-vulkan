#version 450 core

void main() {
	const vec2 positions[] = {
		vec2(-0.5, -0.5),
		vec2(0.5, -0.5),
		vec2(0.0, 0.5),
	};

	const vec2 position = positions[gl_VertexIndex];

	gl_Position = vec4(position, 0.0, 1.0);
}
