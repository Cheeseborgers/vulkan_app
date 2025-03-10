#pragma once

#include <stdexcept>
#include <string_view>

#include "stb_image.h"

#include "core/types.hpp"

class StbiImage {
public:
    StbiImage(const std::string_view filename, int desired_channels = STBI_rgb_alpha) : p_data{nullptr}
    {
        p_data = stbi_load(filename.data(), &m_size.width, &m_size.height, &m_channels, desired_channels);
        if (!p_data) {
            throw std::runtime_error("Failed to load image: " + std::string(filename));
        }
    }

    ~StbiImage() { free(); }

    void free()
    {
        if (p_data) {
            stbi_image_free(p_data);
            p_data = nullptr;
            // std::print("Image freed\n");
        }
    }

    stbi_uc *GetData() const { return p_data; }
    int GetWidth() const { return m_size.width; }
    int GetHeight() const { return m_size.height; }
    int GetChannels() const { return m_channels; }
    ImageSize GetSize() const { return m_size; }

private:
    stbi_uc *p_data;
    ImageSize m_size;
    int m_channels;
};
