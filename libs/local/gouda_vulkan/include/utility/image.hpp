#pragma once

#include <expected>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "core/types.hpp"
#include "stb_image.h"

// Custom deleter for stb_image memory management
struct StbImageDeleter {
    void operator()(stbi_uc *data) const
    {
        if (data) {
            stbi_image_free(data);
        }
    }
};

class Image {
public:
    /// Loads an image from file
    static Expect<Image, std::string> Load(const std::string_view filename, int desired_channels = STBI_rgb_alpha)
    {
        int width, height, channels;
        stbi_uc *data = stbi_load(filename.data(), &width, &height, &channels, desired_channels);
        if (!data) {
            return std::unexpected("Failed to load image");
        }

        return Image(data, {width, height}, channels); // Use the private constructor
    }

    /// Move Constructor
    Image(Image &&other) noexcept : p_data(std::move(other.p_data)), m_size(other.m_size), m_channels(other.m_channels)
    {
        other.reset();
    }

    /// Copy Constructor (Deep Copy)
    Image(const Image &other) : m_size(other.m_size), m_channels(other.m_channels)
    {
        if (other.p_data) {
            size_t dataSize = m_size.width * m_size.height * m_channels;
            p_data = std::unique_ptr<stbi_uc[], StbImageDeleter>(new stbi_uc[dataSize]);
            std::copy(other.p_data.get(), other.p_data.get() + dataSize, p_data.get());
        }
    }

    /// Copy Assignment (Deep Copy)
    Image &operator=(const Image &other)
    {
        if (this != &other) {
            reset();

            if (other.p_data) {
                size_t dataSize = other.m_size.width * other.m_size.height * other.m_channels;
                p_data = std::unique_ptr<stbi_uc[], StbImageDeleter>(new stbi_uc[dataSize]);
                std::copy(other.p_data.get(), other.p_data.get() + dataSize, p_data.get());
            }

            m_size = other.m_size;
            m_channels = other.m_channels;
        }
        return *this;
    }

    /// Move Assignment
    Image &operator=(Image &&other) noexcept
    {
        if (this != &other) {
            p_data = std::move(other.p_data);
            m_size = other.m_size;
            m_channels = other.m_channels;
            other.reset();
        }
        return *this;
    }

    /// Destructor
    ~Image() { reset(); }

    /// Clears the image data
    void reset()
    {
        p_data.reset();
        m_size = {0, 0};
        m_channels = 0;
    }

    /// Returns the raw pixel data as a span
    std::span<const stbi_uc> data() const
    {
        return {p_data.get(), static_cast<size_t>(m_size.width * m_size.height * m_channels)};
    }

    /// Accessor functions
    int GetWidth() const noexcept { return m_size.width; }
    int GetHeight() const noexcept { return m_size.height; }
    int GetChannels() const noexcept { return m_channels; }
    int GetArea() const noexcept { return m_size.width * m_size.height; }
    ImageSize GetSize() const noexcept { return m_size; }

private:
    /// Private constructor for internal use
    explicit Image(stbi_uc *data, ImageSize size, int channels)
        : p_data(data, StbImageDeleter()), m_size(size), m_channels(channels)
    {
    }

    std::unique_ptr<stbi_uc[], StbImageDeleter> p_data{nullptr}; // Correct ownership
    ImageSize m_size{};
    int m_channels{};
};