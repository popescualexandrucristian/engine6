#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vert_uv;

layout(binding = 0) uniform sampler2D color_texture;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(color_texture, vert_uv);
}