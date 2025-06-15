#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

layout(location = 2) in vec3 instance_position;
layout(location = 3) in vec2 instance_size;
layout(location = 4) in float instance_rotation;
layout(location = 5) in uint instance_texture_index;
layout(location = 6) in vec4 instance_colour;
layout(location = 7) in vec4 instance_sprite_rect; // (offsetX, offsetY, sizeX, sizeY)
layout(location = 8) in uint instance_is_atlas;    // 1 = Atlas-based, 0 = Full texture
layout(location = 9) in uint instance_apply_camera_effects;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_colour;
layout(location = 2) out flat uint out_texture_index;
layout(location = 3) out vec4 out_sprite_rect;
layout(location = 4) out flat uint out_is_atlas;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 wvp;
    mat4 wvp_no_camera_effects;
}
ubo;

void main()
{
    vec3 pos = position;
    float cosR = cos(instance_rotation);
    float sinR = sin(instance_rotation);
    vec2 rotated_position = vec2(pos.x * cosR - pos.y * sinR, pos.x * sinR + pos.y * cosR);
    vec2 scaled_position = rotated_position * instance_size;
    vec3 final_position = vec3(scaled_position + instance_position.xy, pos.z + instance_position.z);
    mat4 wvp_matrix = (instance_apply_camera_effects == 1) ? ubo.wvp : ubo.wvp_no_camera_effects;

    gl_Position = wvp_matrix * vec4(final_position, 1.0);

    out_uv = uv;
    out_texture_index = instance_texture_index;
    out_colour = instance_colour;
    out_sprite_rect = instance_sprite_rect;
    out_is_atlas = instance_is_atlas;
}