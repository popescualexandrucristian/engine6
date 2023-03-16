#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_uv;

layout( push_constant ) uniform push_constants_t
{
	mat4 transform;
} push_constants;

layout(set = 0, binding = 0) uniform  per_frame_data_t{
	mat4 view;
	mat4 proj;
} per_frame_data;

struct object_data
{
	mat4 transform;
};

layout(std140,set = 1, binding = 0) readonly buffer object_buffer_t{

	object_data objects[];
} object_buffer;

void main() {
	mat4 object_transform = object_buffer.objects[gl_BaseInstance].transform;
	//mat4 object_transform = push_constants.transform;
    gl_Position = per_frame_data.proj * per_frame_data.view * object_transform * vec4(position, 1.0);
    frag_color = color;
    frag_normal = normal;
	frag_uv = uv;
}