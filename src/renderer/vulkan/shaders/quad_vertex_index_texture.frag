#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vert_color;
layout(location = 1) in vec2 vert_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D texture_sampler;

void main() {
    vec4 texture_color = texture(texture_sampler, vert_uv);
    out_color = texture_color * vec4(vert_color,1.0);
}