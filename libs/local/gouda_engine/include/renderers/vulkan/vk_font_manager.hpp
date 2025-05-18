#pragma once
/**
 * @file font_manager.hpp
 * @author GoudaCheeseburgers
 * @date 2025-05-17
 * @brief Engine vulkan font manager module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "containers/small_vector.hpp"
#include "core/types.hpp"
#include "renderers/text.hpp"

namespace gouda::vk {

class BufferManager;
class Device;
class Texture;

enum class FontType : u8 { MSDF, BITMAP };

using Glyphs = std::unordered_map<char, Glyph>;

class FontManager {
public:
    FontManager(u32 max_fonts, BufferManager *buffer_manager, Device *device);
    ~FontManager();

    void Load(StringView font_texture_filepath,  StringView font_metadata_filepath);

    [[nodiscard]] bool GetFontsDirty() const { return m_fonts_dirty; }

    void SetClean() { m_fonts_dirty = false; }

private:
    BufferManager *p_buffer_manager;
    Device *p_device;

    Vector<std::unique_ptr<Texture>> m_textures;
    std::unordered_map<u32, Glyphs> m_fonts;
    u32 m_max_fonts;
    bool m_fonts_dirty;
};



}


