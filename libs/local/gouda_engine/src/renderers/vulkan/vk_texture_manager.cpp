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
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_texture.hpp"
#include "utils/filesystem.hpp"

namespace gouda::vk {

TextureManager::TextureManager(BufferManager *buffer_manager, Device *device)
    : p_buffer_manager{buffer_manager}, p_device{device}, m_textures_dirty{true}
{
    m_textures.reserve(MAX_TEXTURES);
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
    ASSERT(m_textures.size() <= MAX_TEXTURES, "Could not create texture. Loaded textures exceeds max textures.");

    if (m_textures.size() >= MAX_TEXTURES) {
        ENGINE_LOG_ERROR("Could not create texture for '{}'. Loaded textures exceeds max textures: {}.", filepath,
                         MAX_TEXTURES);
        return 0; // Return default texture index as fallback
    }

    ENGINE_LOG_DEBUG("Loading texture: image={}", filepath);
    auto texture = p_buffer_manager->CreateTexture(filepath);
    const u32 texture_id{static_cast<u32>(m_textures.size())};

    TextureMetadata metadata;
    metadata.is_atlas = false;
    metadata.texture = texture.get();
    metadata.image_filepath = filepath;

    try {
        metadata.image_last_modified = fs::GetLastWriteTime(filepath);
    } catch (const std::filesystem::filesystem_error& e) {
        ENGINE_LOG_WARNING("Failed to get last modified time for '{}': {}", filepath, e.what());
        metadata.image_last_modified = FileTimeType{};
    }

    m_textures.push_back(std::move(texture));
    m_metadata.push_back(std::move(metadata));
    m_textures_dirty = true;

    return texture_id;
}

u32 TextureManager::LoadAtlasTexture(StringView image_filepath, StringView json_filepath)
{
    ASSERT(m_textures.size() <= MAX_TEXTURES,
           "Could not create the atlas texture. Loaded textures exceeds max textures.");

    if (m_textures.size() >= MAX_TEXTURES) {
        ENGINE_LOG_ERROR("Could not create atlas texture for '{}'. Loaded textures exceeds max textures: {}.",
                         image_filepath, MAX_TEXTURES);
        return 0; // Return default texture index as fallback
    }

    ENGINE_LOG_DEBUG("Loading atlas: image={}, json={}", image_filepath, json_filepath);
    auto texture = p_buffer_manager->CreateTexture(image_filepath);
    const u32 texture_id{static_cast<u32>(m_textures.size())};

    TextureMetadata metadata;
    metadata.is_atlas = true;
    metadata.texture = texture.get();
    metadata.image_filepath = image_filepath;
    metadata.json_filepath = json_filepath;
    try {
        metadata.image_last_modified = fs::GetLastWriteTime(image_filepath);
    } catch (const std::filesystem::filesystem_error& e) {
        ENGINE_LOG_WARNING("Failed to get last modified time for '{}': {}", image_filepath, e.what());
        metadata.image_last_modified = FileTimeType{};
    }
    try {
        metadata.json_last_modified = fs::GetLastWriteTime(json_filepath);
    } catch (const std::filesystem::filesystem_error& e) {
        ENGINE_LOG_WARNING("Failed to get last modified time for '{}': {}", json_filepath, e.what());
        metadata.json_last_modified = FileTimeType{};
    }

    ParseAtlasJson(json_filepath, metadata);

    m_textures.push_back(std::move(texture));
    m_metadata.push_back(std::move(metadata));
    m_textures_dirty = true;

    return texture_id;
}
bool TextureManager::ReloadTexture(u32 texture_id, const bool force)
{
    if (texture_id >= m_textures.size()) {
        ENGINE_LOG_ERROR("Invalid texture_id for reload: {}.", texture_id);
        return false;
    }


    if (texture_id == 0) {
        if (!force) {
            ENGINE_LOG_DEBUG("Skipping default texture reloading (texture_id 0).");
            return true;
        }

        CreateDefaultTexture();
        ENGINE_LOG_DEBUG("Forced reload of default texture.");
        return true;
    }

    TextureMetadata &metadata = m_metadata[texture_id];

    if (metadata.image_filepath.empty()) {
        ENGINE_LOG_ERROR("No image filepath stored for texture_id: {}", texture_id);
        return false;
    }

    // Destroy existing texture
    if (m_textures[texture_id]) {
        m_textures[texture_id]->Destroy(p_device);
    }

    // Recreate texture
    auto new_texture = p_buffer_manager->CreateTexture(metadata.image_filepath);
    if (!new_texture) {
        ENGINE_LOG_ERROR("Failed to recreate texture from '{}'.", metadata.image_filepath);
        return false;
    }
    metadata.texture = new_texture.get();
    m_textures[texture_id] = std::move(new_texture);

    // Update image timestamp
    try {
        metadata.image_last_modified = fs::GetLastWriteTime(metadata.image_filepath);
    } catch (const std::filesystem::filesystem_error& e) {
        ENGINE_LOG_WARNING("Failed to update last modified time for '{}': {}", metadata.image_filepath, e.what());
        metadata.image_last_modified = FileTimeType{};
    }

    // Reload atlas JSON if applicable
    if (metadata.is_atlas && metadata.json_filepath) {
        metadata.sprites.clear();
        ParseAtlasJson(*metadata.json_filepath, metadata);
        try {
            metadata.json_last_modified = fs::GetLastWriteTime(*metadata.json_filepath);
        } catch (const std::filesystem::filesystem_error& e) {
            ENGINE_LOG_WARNING("Failed to update last modified time for '{}': {}", *metadata.json_filepath, e.what());
            metadata.json_last_modified = FileTimeType{};
        }
    }

    m_textures_dirty = true;

    ENGINE_LOG_DEBUG("Reloaded texture_id {}: image={}", texture_id, metadata.image_filepath);
    return true;
}

bool TextureManager::ReloadAllTextures()
{
    return std::ranges::all_of(std::views::iota(0u, static_cast<u32>(m_textures.size())),
                               [this](const u32 id) { return ReloadTexture(id); });
}
bool TextureManager::CheckTextureForUpdate(u32 texture_id)
{
    if (texture_id >= m_textures.size()) {
        ENGINE_LOG_ERROR("Invalid texture_id for update check: {}", texture_id);
        return false;
    }

    TextureMetadata& metadata = m_metadata[texture_id];
    if (metadata.image_filepath.empty()) {
        ENGINE_LOG_DEBUG("No image filepath for texture_id {}, skipping update check", texture_id);
        return false;
    }

    bool needs_reload = false;
    try {
        if (const auto current_image_time = fs::GetLastWriteTime(metadata.image_filepath);
            current_image_time > metadata.image_last_modified) {
            ENGINE_LOG_DEBUG("Image file '{}' has changed for texture_id {}", metadata.image_filepath, texture_id);
            needs_reload = true;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        ENGINE_LOG_WARNING("Failed to check last modified time for '{}': {}", metadata.image_filepath, e.what());
    }

    if (metadata.is_atlas && metadata.json_filepath && metadata.json_last_modified) {
        try {
            if (const auto current_json_time = fs::GetLastWriteTime(*metadata.json_filepath);
                current_json_time > *metadata.json_last_modified) {
                ENGINE_LOG_DEBUG("JSON file '{}' has changed for texture_id {}", *metadata.json_filepath, texture_id);
                needs_reload = true;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            ENGINE_LOG_WARNING("Failed to check last modified time for '{}': {}", *metadata.json_filepath, e.what());
        }
    }

    if (needs_reload) {
        return ReloadTexture(texture_id);
    }

    return false;
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
    metadata.sprites.clear();
    ENGINE_LOG_DEBUG("Parsing atlas metadata: {}", json_filepath);

    std::ifstream file{String{json_filepath}};
    if (!file.is_open()) {
        ENGINE_LOG_ERROR("Failed to open JSON file: {}", json_filepath);
        return;
    }

    nlohmann::json json;
    try {
        file >> json;
    }
    catch (const std::exception &e) {
        ENGINE_LOG_ERROR("JSON parsing error: {}", e.what());
        return;
    }

    if (!json.contains("meta") || !json["meta"].contains("size") || !json["meta"]["size"].contains("w") ||
        !json["meta"]["size"].contains("h")) {
        ENGINE_LOG_ERROR("Invalid JSON: missing meta.size.w/h");
        return;
    }

    const AtlasSize atlas_size{json["meta"]["size"]["w"].get<f32>(), json["meta"]["size"]["h"].get<f32>()};
    ENGINE_LOG_DEBUG("Atlas size: {}:{}", atlas_size.width, atlas_size.height);

    if (!json.contains("sprites")) {
        ENGINE_LOG_ERROR("Invalid JSON: missing sprites");
        return;
    }

    size_t sprite_count{json["sprites"].size()};
    for (const auto &sprite_group : json["sprites"].items()) {
        if (!sprite_group.value().contains("x")) {
            sprite_count += sprite_group.value().size();
        }
    }

    metadata.sprites.reserve(sprite_count);
    ENGINE_LOG_DEBUG("Reserved {} slots for sprites", sprite_count);

    for (const auto &sprite_group : json["sprites"].items()) {
        const auto group_name = String{sprite_group.key()};                  // Copy to avoid temporary
        if (const auto &group = sprite_group.value(); group.contains("x")) { // Single sprite
            ENGINE_LOG_DEBUG("Processing single sprite: {}", group_name);
            Sprite sprite;
            if (!group.contains("y") || !group.contains("w") || !group.contains("h")) {
                ENGINE_LOG_ERROR("Invalid single sprite {}: missing x/y/w/h", group_name);
                continue;
            }

            const SpriteRect sprite_rect{group["x"].get<f32>(), group["y"].get<f32>(), group["w"].get<f32>(),
                                         group["h"].get<f32>()};
            SpriteFrame frame;
            frame.uv_rect = NormalizeRect(sprite_rect, atlas_size);
            sprite.frames.push_back(frame);
            ENGINE_LOG_DEBUG("Adding single sprite: {} pre-normal=({}), normal=({})", group_name, sprite_rect.ToString(),
                             frame.uv_rect.ToString());
            metadata.sprites.emplace(group_name, std::move(sprite));
        }
        else { // Animation group
            ENGINE_LOG_DEBUG("Animation group: {}", group_name);
            for (const auto &anim : group.items()) {
                const auto anim_name = String{anim.key()}; // Copy to avoid temporary
                const auto &anim_data = anim.value();
                ENGINE_LOG_DEBUG("Found animation: {}.{}", group_name, anim_name);

                if (!anim_data.contains("loop") || !anim_data.contains("timings")) {
                    ENGINE_LOG_ERROR("Invalid animation {}.{}: missing loop/timings", group_name, anim_name);
                    continue;
                }

                Sprite sprite;
                sprite.looping = anim_data["loop"].get<bool>();

                // Collect and sort frames
                Vector<std::pair<int, nlohmann::json>> frames;
                for (const auto &frame : anim_data.items()) {
                    if (frame.key().starts_with("frame_")) {
                        ENGINE_LOG_DEBUG("Found frame: {}", frame.key());
                        try {
                            int frame_num = std::stoi(frame.key().substr(6));
                            frames.emplace_back(frame_num, frame.value());
                        }
                        catch ([[maybe_unused]] const std::exception &e) {
                            ENGINE_LOG_FATAL("Invalid frame number in {}.{}: {}", group_name, anim_name, frame.key());
                        }
                    }
                }

                std::ranges::sort(frames);

                sprite.frames.reserve(frames.size());
                sprite.frame_durations.reserve(anim_data["timings"].size());

                for (const auto &[frame_num, frame_data] : frames) {
                    if (!frame_data.contains("x") || !frame_data.contains("y") || !frame_data.contains("w") ||
                        !frame_data.contains("h")) {
                        ENGINE_LOG_ERROR("Invalid frame in {}.{}: missing x/y/w/h", group_name, anim_name);
                        continue;
                    }
                    const SpriteRect sprite_rect{frame_data["x"].get<f32>(), frame_data["y"].get<f32>(),
                                                 frame_data["w"].get<f32>(), frame_data["h"].get<f32>()};

                    SpriteFrame sprite_frame;
                    sprite_frame.uv_rect = NormalizeRect(sprite_rect, atlas_size);
                    sprite.frames.push_back(sprite_frame);
                    ENGINE_LOG_DEBUG("Frame {}.{} #{}: pre-normal=({}), normal=({})", group_name, anim_name, frame_num,
                                     sprite_rect.ToString(), sprite_frame.uv_rect.ToString());
                }

                for (const auto &timing : anim_data["timings"]) {
                    f32 duration{timing.get<f32>()};
                    sprite.frame_durations.push_back(duration > 0.0f ? duration : 0.1f);
                }

                String key{std::format("{}.{}", group_name, anim_name)};
                ENGINE_LOG_DEBUG("Adding animation sprite: {}", key);
                metadata.sprites.emplace(key, std::move(sprite));
            }
        }
    }

    ENGINE_LOG_DEBUG("Loaded sprite: {}", json_filepath);
}

// Private functions ----------------------------------------------------

const TextureMetadata &TextureManager::GetTextureMetadata(const u32 texture_id) const
{
    ASSERT(IsValidTextureId(texture_id), "Invalid texture ID: {}", texture_id);

    if (texture_id >= m_metadata.size()) {
        ENGINE_LOG_ERROR("Invalid texture ID: {}. Using default texture metadata.", texture_id);
        return m_metadata[0];
    }

    return m_metadata[texture_id];
}

UVRect<f32> TextureManager::NormalizeRect(const SpriteRect sprite_rect, const AtlasSize atlas_size)
{
    ASSERT(atlas_size.area() != 0, "Texture atlas cannot have a zero size in metadata.");

    if (sprite_rect.x < 0 || sprite_rect.y < 0 || sprite_rect.width <= 0 || sprite_rect.height <= 0 ||
        atlas_size.area() == 0) {
        ENGINE_LOG_ERROR("Invalid sprite rect: {}, atlas_size: {}", sprite_rect.ToString(), atlas_size.ToString());
        return UVRect{0.0f, 0.0f, 0.0f, 0.0f};
    }

    const f32 u_min{sprite_rect.x / atlas_size.width};
    const f32 u_max{(sprite_rect.x + sprite_rect.width) / atlas_size.width};
    const f32 v_min{(atlas_size.height - (sprite_rect.y + sprite_rect.height)) /
                    atlas_size.height}; // Flip y for Vulkan
    const f32 v_max{(atlas_size.height - sprite_rect.y) / atlas_size.height};

    return UVRect{u_min, v_min, u_max, v_max};
}

void TextureManager::CreateDefaultTexture()
{
    ASSERT(m_textures.size() <= MAX_TEXTURES,
           "Could not create default texture. Loaded textures exceeds max textures.");
    ASSERT(m_textures.empty(), "Could not create default texture. Texture already exists at index 0.");

    if (m_textures.size() >= MAX_TEXTURES) {
        ENGINE_LOG_ERROR("Could not create default texture. Loaded textures exceeds max textures: {}.", MAX_TEXTURES);
        return;
    }

    if (!m_textures.empty()) {
        ENGINE_LOG_ERROR("Texture already exists at index 0.");
        return;
    }

    auto default_texture = p_buffer_manager->CreateDefaultTexture();
    const u32 texture_id{static_cast<u32>(m_textures.size())};

    TextureMetadata metadata;
    metadata.is_atlas = false;
    metadata.texture = default_texture.get();
    metadata.image_filepath = "default_texture.png";
    metadata.image_last_modified = FileTimeType{};

    m_textures.push_back(std::move(default_texture));
    m_metadata.push_back(std::move(metadata));

    m_textures_dirty = true; // Ensure textures are set dirty

    ENGINE_LOG_DEBUG("Default texture created and added to textures at id: {}.", texture_id);
}

} // namespace gouda::vk
