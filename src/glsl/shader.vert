#version 450 core

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec3 a_color;
layout (location = 2) in vec2 a_uv;

layout (set = 0, binding = 0) uniform View {
	mat4 mat_vp;
};

layout (set = 2, binding = 0) readonly buffer Instances {
	mat4 mat_ms[];
};

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec2 out_uv;

void main() {
	const mat4 mat_m = mat_ms[gl_InstanceIndex];
	const vec4 world_pos = mat_m * vec4(a_pos, 0.0, 1.0);

	out_color = a_color;
	out_uv = a_uv;
	gl_Position = mat_vp * world_pos;
}
