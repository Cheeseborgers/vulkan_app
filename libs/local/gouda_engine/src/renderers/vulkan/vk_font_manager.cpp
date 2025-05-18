/**
 * @file font_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-05-17
 * @brief Engine vulkan font manager module implementation
 */
#include "../../../include/renderers/vulkan/vk_font_manager.hpp"

#include "../../../include/renderers/vulkan/vk_texture.hpp"

namespace gouda::vk {
FontManager::FontManager(const u32 max_fonts, BufferManager *buffer_manager, Device *device)
    : p_buffer_manager{buffer_manager}, p_device{device}, m_max_fonts{max_fonts}, m_fonts_dirty{true}
{
}

FontManager::~FontManager() {
    for (const auto &texture : m_textures) {
        texture->Destroy(p_device);
    }
}

void FontManager::Load(StringView font_texture_filepath, StringView font_metadata_filepath)
{

}

}