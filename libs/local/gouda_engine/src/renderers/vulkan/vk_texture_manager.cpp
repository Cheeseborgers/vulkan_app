/**
 * @file vk_texture_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-05-10
 * @brief Engine vulkan texture manager module implementation
 */
#include "renderers/vulkan/vk_texture_manager.hpp"

#include <nlohmann/json.hpp>

#include "debug/logger.hpp"
#include "renderers/vulkan/vk_buffer_manager.hpp"
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_texture.hpp"

namespace gouda::vk {

TextureManager::TextureManager(BufferManager *buffer_manager, Device *device, const u32 max_textures)
    : p_buffer_manager{buffer_manager}, p_device{device}, m_max_textures{max_textures}, m_textures_dirty{true}
{
}

TextureManager::~TextureManager()
{
    for (const auto &texture : m_textures) {
        texture->Destroy(p_device);
    }
}

u32 TextureManager::LoadSingleTexture(StringView filepath)
{
    ASSERT(m_textures.size() != m_max_textures, "Attempting to load a texture when max textures are loaded");

    if (m_textures.size() == m_max_textures) {
        ENGINE_LOG_ERROR("Could not load texture '{}' as max textures are loaded: {} textures", filepath,
                         m_max_textures);
        return 0; // Return default texture
    }

    auto texture = p_buffer_manager->CreateTexture(filepath);
    m_textures.push_back(std::move(texture));
    m_textures_dirty = true;

    return static_cast<u32>(m_textures.size() - 1);
}

u32 TextureManager::LoadAtlasTexture(StringView image_filepath, StringView json_filepath)
{
    ASSERT(m_textures.size() != m_max_textures, "Attempting to load a texture when max textures are loaded");

    if (m_textures.size() == m_max_textures) {
        ENGINE_LOG_ERROR("Could not load texture '{}' as max textures are loaded: {} textures", image_filepath,
                         m_max_textures);
        return 0; // Return default texture
    }

    auto texture = p_buffer_manager->CreateTexture(image_filepath);
    m_textures.push_back(std::move(texture));
    m_textures_dirty = true;

    TextureMetadata meta;
    meta.is_atlas = true;
    meta.sprites = ParseAtlasJson(json_filepath); // Parse JSON for sprite rects
    m_metadata.push_back(std::move(meta));

    return static_cast<u32>(m_textures.size() - 1);
}

SpriteRect TextureManager::GetSpriteRect(u32 texture_id, StringView sprite_name) const
{
    if (texture_id >= m_metadata.size() || !m_metadata[texture_id].is_atlas) {
        return SpriteRect{0, 0, 1, 1}; // Default to full texture
    }

    const auto it = m_metadata[texture_id].sprites.find(sprite_name.data());
    return it != m_metadata[texture_id].sprites.end() ? it->second : SpriteRect{0, 0, 1, 1};
}

// Private functions ----------------------------------------------------
std::unordered_map<std::string, SpriteRect> TextureManager::ParseAtlasJson(StringView json_filepath)
{
    std::ifstream file(json_filepath.data());
    if (!file.is_open()) {
        APP_LOG_ERROR("Failed to open texture atlas metadata file '{}'.", m_filepath.string());
        return {}; // FIXME: Return something other than an empty map
    }

    try {
        nlohmann::json json_data;
        file >> json_data;

        if (!json_data.is_object()) {
            throw std::runtime_error("Invalid JSON format (not an object).");
        }
    }
    catch ([[maybe_unused]] const std::exception &e) {
        APP_LOG_WARNING("Failed to parse settings file '{}'. Error: {}, using default settings.", m_filepath.string(),
                        e.what());
    }

    // Parse JSON (e.g., { "sprites": { "name": { "x": 0, "y": 0, "w": 32, "h": 32 } } })
    // Do we need to convert these pixel coords to normalized UVs?
    return {};
}

} // namespace gouda::vk
