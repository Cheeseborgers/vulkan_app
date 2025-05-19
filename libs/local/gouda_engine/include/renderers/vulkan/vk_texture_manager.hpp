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
#include "vk_texture.hpp"

#include "containers/small_vector.hpp"
#include "core/types.hpp"

namespace gouda::vk {

class BufferManager;
class Device;

class TextureManager {
public:
    TextureManager(BufferManager *buffer_manager, Device *device);
    ~TextureManager();

    u32 LoadSingleTexture(StringView filepath);
    u32 LoadAtlasTexture(StringView image_filepath, StringView json_filepath);

    [[nodiscard]] const Sprite* GetSprite(u32 texture_id, StringView sprite_name) const;
    [[nodiscard]] const TextureMetadata& GetTextureMetadata(u32 texture_id) const;
    [[nodiscard]] u32 GetTextureCount() const { return m_textures.size(); }
    const Vector<std::unique_ptr<Texture>> &GetTextures() { return m_textures; }

    [[nodiscard]] bool IsDirty() const { return m_textures_dirty; }
    void SetClean() { m_textures_dirty = false; }

private:
    void ParseAtlasJson(StringView json_filepath, TextureMetadata &metadata);
    UVRect<f32> NormalizeRect(f32 x, f32 y, f32 w, f32 h, f32 atlas_width, f32 atlas_height);
    void CreateDefaultTexture();

private:
    BufferManager *p_buffer_manager;
    Device *p_device;

    Vector<std::unique_ptr<Texture>> m_textures;
    Vector<TextureMetadata> m_metadata;
    bool m_textures_dirty;
};

} // namespace gouda