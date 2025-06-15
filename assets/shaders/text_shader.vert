#version 460
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

layout(location = 2) in vec3 instance_position;// VK_FORMAT_R32G32B32_SFLOAT
layout(location = 3) in vec2 instance_size;// VK_FORMAT_R32G32_SFLOAT
layout(location = 4) in vec4 instance_colour;// VK_FORMAT_R32G32B32A32_SFLOAT
layout(location = 5) in uint instance_glyph_index;// VK_FORMAT_R32_UINT
layout(location = 6) in vec4 instance_sdf_params;// VK_FORMAT_R32G32B32A32_SFLOAT
layout(location = 7) in uint instance_texture_index;// VK_FORMAT_R32_UINT
layout(location = 8) in vec2 instance_atlas_size;// VK_FORMAT_R32G32_SFLOAT
layout(location = 9) in float instance_px_range;// VK_FORMAT_R32_SFLOAT
layout(location = 10) in uint instance_apply_camera_effects;// VK_FORMAT_R32_UINT

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_colour;
layout(location = 2) out flat uint out_glyph_index;
layout(location = 3) out vec4 out_sdf_params;
layout(location = 4) out flat uint out_texture_index;
layout(location = 5) out vec2 out_atlas_size;
layout(location = 6) out float out_px_range;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 wvp;
    mat4 wvp_no_camera_effects;
}
ubo;

void main()
{
    vec2 pos = position.xy * instance_size;// instance_size is already scaled
    vec2 final_position = pos + instance_position.xy;// instance_position is already scaled
    mat4 wvp_matrix = (instance_apply_camera_effects == 1) ? ubo.wvp : ubo.wvp_no_camera_effects;

    gl_Position = wvp_matrix * vec4(final_position, instance_position.z, 1.0);

    out_uv = uv;
    out_colour = instance_colour;
    out_glyph_index = instance_glyph_index;
    out_sdf_params = instance_sdf_params;
    out_texture_index = instance_texture_index;
    out_atlas_size = instance_atlas_size;
    out_px_range = instance_px_range;
}