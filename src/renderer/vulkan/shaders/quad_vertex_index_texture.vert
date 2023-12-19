#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_uv;


void main() {
    gl_Position = vec4(position, 1.0);
    frag_color = color;
    frag_uv = uv;
}