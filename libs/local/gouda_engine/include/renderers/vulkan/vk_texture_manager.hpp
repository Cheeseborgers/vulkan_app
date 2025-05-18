#pragma once
/**
 * @file vk_texture_manager.hpp
 * @author GoudaCheeseburgers
 * @date 2025-05-10
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <string>
#include <unordered_map>

#include "containers/small_vector.hpp"
#include "core/types.hpp"

namespace gouda::vk {

class BufferManager;
class Device;
class Texture;

using SpriteRect = Rect<f32>;

struct TextureMetadata {
    bool is_atlas{false};
    std::unordered_map<String, SpriteRect> sprites;
};

class TextureManager {
public:
    TextureManager(BufferManager *buffer_manager, Device *device, u32 max_textures = 32);
    ~TextureManager();

    u32 LoadSingleTexture(StringView filepath);
    u32 LoadAtlasTexture(StringView image_filepath, StringView json_filepath);

    [[nodiscard]] SpriteRect GetSpriteRect(u32 texture_id, StringView sprite_name) const;
    [[nodiscard]] bool GetTextureDirty() const { return m_textures_dirty; }
    [[nodiscard]] u32 GetTextureCount() const { return m_textures.size(); }
    const Vector<std::unique_ptr<Texture>> &GetTextures() { return m_textures; }

    void SetClean() { m_textures_dirty = false; }

private:
    std::unordered_map<String, SpriteRect> ParseAtlasJson(StringView json_filepath);

private:
    BufferManager *p_buffer_manager;
    Device *p_device;

    Vector<std::unique_ptr<Texture>> m_textures;
    Vector<TextureMetadata> m_metadata;
    u32 m_max_textures;
    bool m_textures_dirty;
};

} // namespace gouda