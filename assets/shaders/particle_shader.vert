#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

layout(location = 2) in vec3 instance_position;
layout(location = 3) in vec2 instance_size;
layout(location = 4) in vec4 instance_colour;
layout(location = 5) in uint instance_texture_index;
layout(location = 6) in vec4 instance_sprite_rect; // (offsetX, offsetY, sizeX, sizeY)
layout(location = 7) in float instance_lifetime;
layout(location = 8) in vec3 instance_velocity;
layout(location = 9) in uint instance_is_atlas;    // 1 = Atlas-based, 0 = Full texture
layout(location = 10) in uint instance_apply_camera_effects;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 wvp;
    mat4 wvp_no_camera_effects;
}
ubo;

layout(location = 0) out vec4 out_colour;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out flat uint out_texture_index;
layout(location = 3) out vec4 out_sprite_rect;
layout(location = 4) out flat uint out_is_atlas;

void main()
{
    vec3 world_position = position * vec3(instance_size, 1.0) + instance_position;
    mat4 wvp_matrix = (instance_apply_camera_effects == 1) ? ubo.wvp : ubo.wvp_no_camera_effects;

    gl_Position = wvp_matrix * vec4(world_position, 1.0);

    out_colour = instance_colour;
    out_uv = uv;
    out_texture_index = instance_texture_index;
    out_sprite_rect = instance_sprite_rect;
    out_is_atlas = instance_is_atlas;
}