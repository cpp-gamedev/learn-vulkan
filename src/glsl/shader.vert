#version 450 core

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec3 a_color;

layout (location = 0) out vec3 out_color;

void main() {
	const vec2 position = a_pos;

	out_color = a_color;
	gl_Position = vec4(position, 0.0, 1.0);
}
