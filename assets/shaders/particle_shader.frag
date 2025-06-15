#version 460

layout(location = 0) in vec4 colour;
layout(location = 1) in vec2 uv;
layout(location = 2) in flat uint texture_index;
layout(location = 3) in vec4 sprite_rect;
layout(location = 4) in flat uint is_atlas;

#define MAX_TEXTURES 32
layout(binding = 3) uniform sampler2D texture_samplers[MAX_TEXTURES];

layout(location = 0) out vec4 out_colour;

void main()
{
    vec2 sampled_coord = uv;
    if (is_atlas == 1) {
        // Map uv from [0,1] to sprite rect [sprite_rect.xy, sprite_rect.zw]
        sampled_coord = sprite_rect.xy + uv * (sprite_rect.zw - sprite_rect.xy);
    }

    switch (texture_index) {
        case 0:
            out_colour = texture(texture_samplers[0], sampled_coord) * colour;
            break;
        case 1:
            out_colour = texture(texture_samplers[1], sampled_coord) * colour;
            break;
        case 2:
            out_colour = texture(texture_samplers[2], sampled_coord) * colour;
            break;
        case 3:
            out_colour = texture(texture_samplers[3], sampled_coord) * colour;
            break;
        case 4:
            out_colour = texture(texture_samplers[4], sampled_coord) * colour;
            break;
        case 5:
            out_colour = texture(texture_samplers[5], sampled_coord) * colour;
            break;
        case 6:
            out_colour = texture(texture_samplers[6], sampled_coord) * colour;
            break;
        case 7:
            out_colour = texture(texture_samplers[7], sampled_coord) * colour;
            break;
        case 8:
            out_colour = texture(texture_samplers[8], sampled_coord) * colour;
            break;
        case 9:
            out_colour = texture(texture_samplers[9], sampled_coord) * colour;
            break;
        case 10:
            out_colour = texture(texture_samplers[10], sampled_coord) * colour;
            break;
        case 11:
            out_colour = texture(texture_samplers[11], sampled_coord) * colour;
            break;
        case 12:
            out_colour = texture(texture_samplers[12], sampled_coord) * colour;
            break;
        case 13:
            out_colour = texture(texture_samplers[13], sampled_coord) * colour;
            break;
        case 14:
            out_colour = texture(texture_samplers[14], sampled_coord) * colour;
            break;
        case 15:
            out_colour = texture(texture_samplers[15], sampled_coord) * colour;
            break;
        case 16:
            out_colour = texture(texture_samplers[16], sampled_coord) * colour;
            break;
        case 17:
            out_colour = texture(texture_samplers[17], sampled_coord) * colour;
            break;
        case 18:
            out_colour = texture(texture_samplers[18], sampled_coord) * colour;
            break;
        case 19:
            out_colour = texture(texture_samplers[19], sampled_coord) * colour;
            break;
        case 20:
            out_colour = texture(texture_samplers[20], sampled_coord) * colour;
            break;
        case 21:
            out_colour = texture(texture_samplers[21], sampled_coord) * colour;
            break;
        case 22:
            out_colour = texture(texture_samplers[22], sampled_coord) * colour;
            break;
        case 23:
            out_colour = texture(texture_samplers[23], sampled_coord) * colour;
            break;
        case 24:
            out_colour = texture(texture_samplers[24], sampled_coord) * colour;
            break;
        case 25:
            out_colour = texture(texture_samplers[25], sampled_coord) * colour;
            break;
        case 26:
            out_colour = texture(texture_samplers[26], sampled_coord) * colour;
            break;
        case 27:
            out_colour = texture(texture_samplers[27], sampled_coord) * colour;
            break;
        case 28:
            out_colour = texture(texture_samplers[28], sampled_coord) * colour;
            break;
        case 29:
            out_colour = texture(texture_samplers[29], sampled_coord) * colour;
            break;
        case 30:
            out_colour = texture(texture_samplers[30], sampled_coord) * colour;
            break;
        case 31:
            out_colour = texture(texture_samplers[31], sampled_coord) * colour;
            break;
        default:
            out_colour = vec4(1, 0, 0, 1); // Red for error
    }
}