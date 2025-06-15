#version 460

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_colour;
layout(location = 2) in flat uint in_glyph_index;
layout(location = 3) in vec4 in_sdf_params;
layout(location = 4) in flat uint in_texture_index;
layout(location = 5) in vec2 in_atlas_size;
layout(location = 6) in float in_px_range;

layout(location = 0) out vec4 out_colour;

#define MAX_TEXTURES 32
layout(binding = 4) uniform sampler2D texture_samplers[MAX_TEXTURES];

float median(float r, float g, float b) { return max(min(r, g), min(max(r, g), b)); }

void main()
{
    // sdf_params = (left, bottom, right, top) in pixels
    vec4 bounds = in_sdf_params;
    vec2 atlas_min = vec2(bounds.x, bounds.y);
    vec2 atlas_max = vec2(bounds.z, bounds.w);

    vec2 atlas_min_uv = vec2(atlas_min.x / in_atlas_size.x, atlas_min.y / in_atlas_size.y);
    vec2 atlas_max_uv = vec2(atlas_max.x / in_atlas_size.x, atlas_max.y / in_atlas_size.y);
    vec2 atlas_uv_size = atlas_max_uv - atlas_min_uv;

    vec2 atlas_uv = atlas_min_uv + in_uv * atlas_uv_size;

    vec3 msdf;
    switch (in_texture_index) {
        case 0:
            msdf = texture(texture_samplers[0], atlas_uv).rgb;
            break;
        case 1:
            msdf = texture(texture_samplers[1], atlas_uv).rgb;
            break;
        case 2:
            msdf = texture(texture_samplers[2], atlas_uv).rgb;
            break;
        case 3:
            msdf = texture(texture_samplers[3], atlas_uv).rgb;
            break;
        case 4:
            msdf = texture(texture_samplers[4], atlas_uv).rgb;
            break;
        case 5:
            msdf = texture(texture_samplers[5], atlas_uv).rgb;
            break;
        case 6:
            msdf = texture(texture_samplers[6], atlas_uv).rgb;
            break;
        case 7:
            msdf = texture(texture_samplers[7], atlas_uv).rgb;
            break;
        case 8:
            msdf = texture(texture_samplers[8], atlas_uv).rgb;
            break;
        case 9:
            msdf = texture(texture_samplers[9], atlas_uv).rgb;
            break;
        case 10:
            msdf = texture(texture_samplers[10], atlas_uv).rgb;
            break;
        case 11:
            msdf = texture(texture_samplers[11], atlas_uv).rgb;
            break;
        case 12:
            msdf = texture(texture_samplers[12], atlas_uv).rgb;
            break;
        case 13:
            msdf = texture(texture_samplers[13], atlas_uv).rgb;
            break;
        case 14:
            msdf = texture(texture_samplers[14], atlas_uv).rgb;
            break;
        case 15:
            msdf = texture(texture_samplers[15], atlas_uv).rgb;
            break;
        case 16:
            msdf = texture(texture_samplers[16], atlas_uv).rgb;
            break;
        case 17:
            msdf = texture(texture_samplers[17], atlas_uv).rgb;
            break;
        case 18:
            msdf = texture(texture_samplers[18], atlas_uv).rgb;
            break;
        case 19:
            msdf = texture(texture_samplers[19], atlas_uv).rgb;
            break;
        case 20:
            msdf = texture(texture_samplers[20], atlas_uv).rgb;
            break;
        case 21:
            msdf = texture(texture_samplers[21], atlas_uv).rgb;
            break;
        case 22:
            msdf = texture(texture_samplers[22], atlas_uv).rgb;
            break;
        case 23:
            msdf = texture(texture_samplers[23], atlas_uv).rgb;
            break;
        case 24:
            msdf = texture(texture_samplers[24], atlas_uv).rgb;
            break;
        case 25:
            msdf = texture(texture_samplers[25], atlas_uv).rgb;
            break;
        case 26:
            msdf = texture(texture_samplers[26], atlas_uv).rgb;
            break;
        case 27:
            msdf = texture(texture_samplers[27], atlas_uv).rgb;
            break;
        case 28:
            msdf = texture(texture_samplers[28], atlas_uv).rgb;
            break;
        case 29:
            msdf = texture(texture_samplers[29], atlas_uv).rgb;
            break;
        case 30:
            msdf = texture(texture_samplers[30], atlas_uv).rgb;
            break;
        case 31:
            msdf = texture(texture_samplers[31], atlas_uv).rgb;
            break;
        default:
            msdf = vec3(1.0, 0.0, 0.0);
            break; // Red for error
    }

    // Debug: Visualize MSDF texture
    // out_colour = vec4(msdf, 1.0);
    // return;

    // Debug: Visualize UVs
    // out_colour = vec4(atlas_uv, 0.0, 1.0);
    // return;

    // Debug: Color by glyph_index
    // out_colour = vec4(float(glyph_index % 3) / 2.0, float((glyph_index / 3) % 3) / 2.0, float(glyph_index / 9)
    // / 2.0, 1.0); return;


    float distance = median(msdf.r, msdf.g, msdf.b);

    // Improve antialiasing using fwidth
    float screen_px_distance = in_px_range * (distance - 0.5) / fwidth(distance);
    float alpha = clamp(screen_px_distance + 0.5, 0.0, 1.0);

    alpha = pow(alpha, 1.0 / 2.2);

    // TODO: Add gradients
    //vec3 gradient = mix(vec3(1.0, 0.2, 0.2), vec3(0.2, 0.2, 1.0), uv.y);
    //out_colour = vec4(gradient, colour.a * alpha);

    // TODO: Add soft shadows
    //vec2 shadow_offset = vec2(2.0 / atlas_size.x, -2.0 / atlas_size.y);
    //vec3 shadow_msdf = texture(texture_samplers[texture_index], atlas_uv + shadow_offset).rgb;
    //float shadow_distance = median(shadow_msdf.r, shadow_msdf.g, shadow_msdf.b);
    //float shadow_alpha = clamp(px_range * (shadow_distance - 0.5) / fwidth(shadow_distance) + 0.5, 0.0, 1.0);
    //vec4 shadow_color = vec4(0.0, 0.0, 0.0, 0.5 * shadow_alpha);
    //out_colour = mix(shadow_color, vec4(colour.rgb, colour.a * alpha), alpha);

    out_colour = vec4(in_colour.rgb, in_colour.a * alpha);
}