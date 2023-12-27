#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 frag_uv;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 position;

void main() {
    vec3 new_position = position * 20.0;
    gl_Position = vec4(new_position.x, new_position.y - 0.5, new_position.z + 1.0, 1.0);
    frag_uv = uv;
}