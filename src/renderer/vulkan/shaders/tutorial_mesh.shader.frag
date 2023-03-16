#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vert_color;
layout(location = 1) in vec3 vert_normal;
layout(location = 2) in vec2 vert_uv;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(vert_color, 1.0);
}