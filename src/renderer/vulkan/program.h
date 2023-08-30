#pragma once
struct renderer_context;

#include <vulkan/vulkan.h>
#include <initializer_list>
#include <vector>
#include <string>

struct shader
{
    VkShaderStageFlagBits type;
    VkShaderModule module;
    bool uses_constants;
    struct meta
    {
        enum class type {
            void_type,
            bool_type,
            int_type,
            uint_type,
            float_type,
            double_type,
            bvec2_type,
            ivec2_type,
            uvec2_type,
            vec2_type,
            dvec2_type,
            bvec3_type,
            ivec3_type,
            uvec3_type,
            vec3_type,
            dvec3_type,
            bvec4_type,
            ivec4_type,
            uvec4_type,
            vec4_type,
            dvec4_type,
            types_count
        };
        struct binding
        {
            VkDescriptorType resource_type;
            uint32_t binding;
            uint32_t set;
        };
        struct attribute
        {
            shader::meta::type field_type;
            uint32_t location;
        };

        uint32_t group_size_x;
        uint32_t group_size_y;
        uint32_t group_size_z;
        int32_t group_size_x_id = -1;
        int32_t group_size_y_id = -1;
        int32_t group_size_z_id = -1;
        std::vector<binding> resource_bindings;
        std::string entry_point;
        std::vector<attribute> inputs;
        std::vector<attribute> outputs;
    } metadata;
};

typedef std::initializer_list<const shader*> shaders;
shader* shader_init(renderer_context* context, const uint32_t* const data, size_t data_size);
shader* shader_init(renderer_context* context, const char* path);
void shader_destroy(renderer_context* context, shader* shader);


struct graphics_program
{
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    std::vector<VkDescriptorSetLayout> descriptor_layouts;
};

struct input_attribute_data
{
	uint32_t				binding;
	std::vector<uint32_t>	offsets;
	std::vector<uint32_t>	locations;
	uint32_t				stride;
	VkVertexInputRate		input_rate;
};

typedef std::initializer_list<input_attribute_data> input_attributes;
//alex(todo) : add options for blending and move all the bools to flags.
graphics_program* graphics_program_init(renderer_context* context, shaders shaders, input_attributes vertex_input_attributes, size_t push_constant_size, bool use_depth, bool write_to_depth, bool sharedDescriptorSets);
void graphics_program_destroy(renderer_context* context, graphics_program* program);