#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 particle_pos;
layout(location = 2) in vec3 particle_color;

layout(location = 0) out vec3 frag_color;


void main() {
    vec3 pos = position;
    pos.xy = pos.xy/10.0 + particle_pos;
    gl_Position = vec4(pos, 1.0);
    frag_color = particle_color;
}