#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vert_color;
layout(location = 1) in vec3 vert_normal;
layout(location = 2) in vec2 vert_uv;

layout(set = 2, binding = 0) uniform sampler2D  texture_sampler;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(texture_sampler, vert_uv);
}