/**
 * @file vk_texture_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-05-10
 * @brief Engine vulkan texture manager module implementation
 */
#include "renderers/vulkan/vk_texture_manager.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

#include "debug/logger.hpp"
#include "debug/throw.hpp"
#include "renderers/vulkan/vk_buffer_manager.hpp"
#include "renderers/vulkan/vk_texture.hpp"
#include "renderers/vulkan/vk_device.hpp"

namespace gouda::vk {

TextureManager::TextureManager(BufferManager *buffer_manager, Device *device)
    : p_buffer_manager{buffer_manager}, p_device{device}, m_textures_dirty{true}
{
    m_textures.reserve(MAX_TEXTURES); // Prevent reallocation
    m_metadata.reserve(MAX_TEXTURES);
    CreateDefaultTexture();
}

TextureManager::~TextureManager()
{
    for (const auto &texture : m_textures) {
        texture->Destroy(p_device);
    }
}

u32 TextureManager::LoadSingleTexture(StringView filepath)
{
    ENGINE_LOG_DEBUG("Loading texture: image={}", filepath);
    auto texture = p_buffer_manager->CreateTexture(filepath);
    const u32 texture_id = m_textures.size();

    TextureMetadata metadata;
    metadata.is_atlas = false;
    metadata.texture = texture.get();

    m_textures.push_back(std::move(texture));
    m_metadata.push_back(std::move(metadata));
    m_textures_dirty = true;

    return texture_id;
}

u32 TextureManager::LoadAtlasTexture(StringView image_filepath, StringView json_filepath)
{
    ENGINE_LOG_DEBUG("Loading atlas: image={}, json={}", image_filepath, json_filepath);
    auto texture = p_buffer_manager->CreateTexture(image_filepath);
    const u32 texture_id = m_textures.size();

    TextureMetadata metadata;
    metadata.is_atlas = true;
    metadata.texture = texture.get();

    ParseAtlasJson(json_filepath, metadata);

    m_textures.push_back(std::move(texture));
    m_metadata.push_back(std::move(metadata));
    m_textures_dirty = true;

    return texture_id;
}

const Sprite *TextureManager::GetSprite(u32 texture_id, StringView sprite_name) const
{
    if (texture_id >= m_metadata.size()) {
        ENGINE_LOG_ERROR("Invalid texture_id: {}", texture_id);
        return nullptr;
    }

    const auto &sprites = m_metadata[texture_id].sprites;
    const auto it = sprites.find(std::string(sprite_name));
    if (it == sprites.end()) {
        ENGINE_LOG_ERROR("Sprite not found: {}", sprite_name);
        return nullptr;
    }

    return &it->second;
}

void TextureManager::ParseAtlasJson(StringView json_filepath, TextureMetadata &metadata)
{
    std::ifstream file{std::string(json_filepath)};
    if (!file.is_open()) {
        ENGINE_LOG_ERROR("Failed to open JSON file: {}", json_filepath);
        return;
    }

    nlohmann::json json;
    try {
        file >> json;
    } catch (const std::exception& e) {
        ENGINE_LOG_ERROR("JSON parsing error: {}", e.what());
        return;
    }

    if (!json.contains("meta") || !json["meta"].contains("size") ||
        !json["meta"]["size"].contains("w") || !json["meta"]["size"].contains("h")) {
        ENGINE_LOG_ERROR("Invalid JSON: missing meta.size.w/h");
        return;
        }

    const float atlas_width = json["meta"]["size"]["w"].get<float>();
    const float atlas_height = json["meta"]["size"]["h"].get<float>();
    ENGINE_LOG_DEBUG("Atlas size: {}:{}", atlas_width, atlas_height);

     if (!json.contains("sprites")) {
        ENGINE_LOG_ERROR("Invalid JSON: missing sprites");
        return;
    }

    for (const auto &sprite_group : json["sprites"].items()) {
        const auto group_name = std::string(sprite_group.key()); // Copy to avoid temporary
        if (const auto &group = sprite_group.value(); group.contains("x")) { // Single sprite
            ENGINE_LOG_DEBUG("Single Sprite: {}", group_name);
            Sprite sprite;
            if (!group.contains("y") || !group.contains("w") || !group.contains("h")) {
                ENGINE_LOG_ERROR("Invalid single sprite {}: missing x/y/w/h", group_name);
                continue;
            }
            const float x = group["x"].get<float>();
            const float y = group["y"].get<float>();
            const float w = group["w"].get<float>();
            const float h = group["h"].get<float>();
            SpriteFrame frame;
            frame.uv_rect = NormalizeRect(x, y, w, h, atlas_width, atlas_height);
            sprite.frames.push_back(frame);
            metadata.sprites[group_name] = sprite;
        } else { // Animation group
            ENGINE_LOG_DEBUG("Animation group: {}", group_name);
            for (const auto &anim : group.items()) {
                const auto anim_name = std::string(anim.key()); // Copy to avoid temporary
                const auto &anim_data = anim.value();
                ENGINE_LOG_DEBUG("Found animation: {}.{}", group_name, anim_name);

                if (!anim_data.contains("loop") || !anim_data.contains("timings")) {
                    ENGINE_LOG_ERROR("Invalid animation {}.{}: missing loop/timings", group_name, anim_name);
                    continue;
                }

                Sprite sprite;
                sprite.looping = anim_data["loop"].get<bool>();

                // Collect and sort frames
                std::vector<std::pair<int, nlohmann::json>> frames;
                for (const auto &frame : anim_data.items()) {
                    if (frame.key().starts_with("frame_")) {
                        ENGINE_LOG_DEBUG("Found frame: {}", frame.key());
                        try {
                            int frame_num = std::stoi(frame.key().substr(6));
                            frames.emplace_back(frame_num, frame.value());
                        } catch (const std::exception& e) {
                            ENGINE_LOG_ERROR("Invalid frame number in {}.{}: {}", group_name, anim_name, frame.key());
                        }
                    }
                }

                std::ranges::sort(frames);

                for (const auto& frame_data : frames | std::views::values) {
                    if (!frame_data.contains("x") || !frame_data.contains("y") ||
                        !frame_data.contains("w") || !frame_data.contains("h")) {
                        ENGINE_LOG_ERROR("Invalid frame in {}.{}: missing x/y/w/h", group_name, anim_name);
                        continue;
                    }
                    const float x = frame_data["x"].get<float>();
                    const float y = frame_data["y"].get<float>();
                    const float w = frame_data["w"].get<float>();
                    const float h = frame_data["h"].get<float>();
                    SpriteFrame sprite_frame;
                    sprite_frame.uv_rect = NormalizeRect(x, y, w, h, atlas_width, atlas_height);
                    sprite.frames.push_back(sprite_frame);
                }

                // Parse frame timings
                for (const auto &timing : anim_data["timings"]) {
                    float duration = timing.get<float>();
                    sprite.frame_durations.push_back(duration > 0.0f ? duration : 0.1f);
                }

                // Use explicit string for map key
                String key = group_name;
                key.append(".");
                key.append(anim_name);
                metadata.sprites[key] = sprite;
            }
        }
    }
    ENGINE_LOG_DEBUG("Loaded sprite: {}", json_filepath);
}

// Private functions ----------------------------------------------------

const TextureMetadata &TextureManager::GetTextureMetadata(const u32 texture_id) const
{
    ASSERT(texture_id < m_metadata.size(), "Invalid texture ID: {}", texture_id);

    if (texture_id >= m_metadata.size()) {
        // ENGINE_THROW("Invalid texture ID: {}", texture_id);
    }

    return m_metadata[texture_id];
}

UVRect<f32> TextureManager::NormalizeRect(f32 x, f32 y, f32 w, f32 h, f32 atlas_width, f32 atlas_height)
{
    if (x < 0 || y < 0 || w <= 0 || h <= 0 || atlas_width <= 0 || atlas_height <= 0) {
        ENGINE_LOG_ERROR("Invalid sprite rect: x={}, y={}, w={}, h={}, atlas_w={}, atlas_h={}", x, y, w, h, atlas_width,
                         atlas_height);
        return UVRect{0.0f, 0.0f, 0.0f, 0.0f};
    }

    const f32 u_min = x / atlas_width;
    const f32 u_max = (x + w) / atlas_width;
    const f32 v_min = (atlas_height - (y + h)) / atlas_height; // Flip y for Vulkan
    const f32 v_max = (atlas_height - y) / atlas_height;

    return UVRect{u_min, v_min, u_max, v_max};
}

void TextureManager::CreateDefaultTexture()
{
    auto default_texture = p_buffer_manager->CreateDefaultTexture();
    const u32 texture_id = m_textures.size();

    TextureMetadata metadata;
    metadata.is_atlas = false;
    metadata.texture = default_texture.get();

    m_textures.push_back(std::move(default_texture));
    m_metadata.push_back(std::move(metadata));

    m_textures_dirty = true; // Ensure textures are set dirty

    ENGINE_LOG_DEBUG("Default texture created and added to textures at id: {}.", texture_id);
}

} // namespace gouda::vk
