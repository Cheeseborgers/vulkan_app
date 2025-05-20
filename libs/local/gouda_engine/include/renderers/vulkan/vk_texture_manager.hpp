#pragma once
/**
 * @file vk_texture_manager.hpp
 * @author GoudaCheeseburgers
 * @date 2025-05-10
 * @brief Engine vulkan texture manager module
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

/**
 * @class TextureManager
 * @brief Manages Vulkan textures and texture atlases, including loading, reloading, and metadata management.
 */
class TextureManager {
public:
    /**
     * @brief Constructs a TextureManager.
     * @param buffer_manager Pointer to the BufferManager for GPU buffer allocations.
     * @param device Pointer to the Vulkan Device.
     */
    TextureManager(BufferManager *buffer_manager, Device *device);

    /**
     * @brief Destroys the TextureManager and cleans up resources.
     */
    ~TextureManager();

    /**
     * @brief Loads a single texture from file.
     * @param filepath Path to the texture file.
     * @return ID of the loaded texture.
     */
    u32 LoadSingleTexture(StringView filepath);

    /**
     * @brief Loads a texture atlas from an image and associated JSON metadata.
     * @param image_filepath Path to the atlas image file.
     * @param json_filepath Path to the JSON file describing sprite regions.
     * @return ID of the loaded atlas texture.
     */
    u32 LoadAtlasTexture(StringView image_filepath, StringView json_filepath);

    /**
     * @brief Reloads a specific texture.
     * @param texture_id ID of the texture to reload.
     * @param force If true, forces reloading of the default texture.
     * @return True if reload was successful.
     */
    bool ReloadTexture(u32 texture_id, bool force = false);

    /**
     * @brief Reloads all loaded textures.
     * @return True if all textures were successfully reloaded.
     */
    bool ReloadAllTextures();

    /**
     * @brief Checks if a texture needs to be updated.
     * @param texture_id ID of the texture to check.
     * @return True if the texture is marked for update.
     */
    bool CheckTextureForUpdate(u32 texture_id);

    /**
     * @brief Retrieves a specific sprite from a texture atlas.
     * @param texture_id ID of the texture containing the sprite.
     * @param sprite_name Name of the sprite to retrieve.
     * @return Pointer to the Sprite if found, nullptr otherwise.
     */
    [[nodiscard]] const Sprite* GetSprite(u32 texture_id, StringView sprite_name) const;

    /**
     * @brief Retrieves metadata associated with a texture.
     * @param texture_id ID of the texture.
     * @return Reference to the TextureMetadata.
     */
    [[nodiscard]] const TextureMetadata& GetTextureMetadata(u32 texture_id) const;

    /**
     * @brief Returns the number of loaded textures.
     * @return Texture count.
     */
    [[nodiscard]] u32 GetTextureCount() const { return m_textures.size(); }

    /**
     * @brief Returns a reference to the texture collection.
     * @return Vector of unique pointers to Texture objects.
     */
    const Vector<std::unique_ptr<Texture>> &GetTextures() { return m_textures; }

    /**
     * @brief Checks whether any texture has been modified or reloaded.
     * @return True if textures are marked dirty.
     */
    [[nodiscard]] bool IsDirty() const { return m_textures_dirty; }

    /**
     * @brief Marks all textures as clean (not dirty).
     */
    void SetClean() { m_textures_dirty = false; }

    /**
     * @brief Checks if a texture ID is valid.
     * @param id ID to validate.
     * @return True if the ID refers to a valid texture.
     */
    [[nodiscard]] bool IsValidTextureId(const u32 id) const {
        return id < m_textures.size();
    }

private:
    /**
     * @brief Parses a JSON file to populate texture metadata for an atlas.
     * @param json_filepath Path to the JSON file.
     * @param metadata Reference to metadata structure to populate.
     */
    void ParseAtlasJson(StringView json_filepath, TextureMetadata &metadata);

    /**
     * @brief Converts a sprite rectangle and atlas size into normalized UV coordinates.
     * @param sprite_rect Rectangle of the sprite in pixel units.
     * @param atlas_size Size of the atlas image.
     * @return Normalized UV rectangle.
     */
    UVRect<f32> NormalizeRect(SpriteRect sprite_rect, AtlasSize atlas_size);

    /**
     * @brief Creates and loads a default texture used as a fallback.
     */
    void CreateDefaultTexture();

private:
    BufferManager *p_buffer_manager;
    Device *p_device;

    Vector<std::unique_ptr<Texture>> m_textures;
    Vector<TextureMetadata> m_metadata;
    bool m_textures_dirty;
};

} // namespace gouda::vk
