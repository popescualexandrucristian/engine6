#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 frag_uv;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 position;

mat4 rotation(vec3 axis, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

void main() {
    float angle = 45.0 / 180.0 * 3.141592;
    mat4 rotation_matrix = rotation(vec3(0.0, 1.0, 0.0), angle);

    vec3 new_position = position * 20.0;
    gl_Position = rotation_matrix * vec4(new_position.x + 0.5, new_position.y - 0.5, new_position.z + 0.5, 1.0);
    frag_uv = uv;
}