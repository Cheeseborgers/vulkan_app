/**
 * @file image.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-01
 * @brief Implementation of the Image class.
 */
#include "utils/image.hpp"

#include "stb_image_resize.h"
#include "stb_image_write.h"

namespace gouda {

Expect<Image, String> Image::Load(StringView filename, const int desired_channels, const bool flip_horizontally)
{
    int actual_channels{0};
    ImageSize size{0, 0};

    // Enable vertical flipping for loading
    stbi_set_flip_vertically_on_load(flip_horizontally); // Flip image so bottom row is first

    // Load image with the desired number of channels
    stbi_uc *data{stbi_load(filename.data(), &size.width, &size.height, &actual_channels, desired_channels)};
    if (!data) {
        stbi_set_flip_vertically_on_load(0); // Reset to avoid affecting other loads
        return std::unexpected("Failed to load image");
    }

    const int stored_channels = desired_channels != 0 ? desired_channels : actual_channels;

    std::vector<stbi_uc> image_data(data, data + size.width * size.height * stored_channels);

    // Free the original data loaded by stbi_load
    stbi_image_free(data);

    // Reset flip setting to avoid affecting other image loads
    stbi_set_flip_vertically_on_load(0);

    // Return the Image object with the loaded data
    return Image(std::move(image_data), ImageSize{size.width, size.height}, stored_channels);
}

bool Image::Save(StringView filename) const
{
    return stbi_write_png(filename.data(), m_size.width, m_size.height, m_channels, p_data.data(),
                          m_size.width * m_channels);
}

Image Image::ToGrayscale() const
{
    auto grayscale = *this;
    for (size_t i = 0; i < m_size.width * m_size.height; ++i) {
        const stbi_uc r = grayscale.p_data[i * m_channels];
        const stbi_uc g = grayscale.p_data[i * m_channels + 1];
        const stbi_uc b = grayscale.p_data[i * m_channels + 2];
        const auto gray = static_cast<stbi_uc>(0.299 * r + 0.587 * g + 0.114 * b);
        grayscale.p_data[i * m_channels] = grayscale.p_data[i * m_channels + 1] = grayscale.p_data[i * m_channels + 2] =
            gray;
    }
    return grayscale;
}

void Image::FlipHorizontal()
{
    const int row_size = m_size.width * m_channels;
    for (int y = 0; y < m_size.height; ++y) {
        std::reverse(p_data.begin() + y * row_size, p_data.begin() + (y + 1) * row_size);
    }
}

Image Image::Rotate90() const
{
    Image rotated({}, ImageSize{m_size.height, m_size.width}, m_channels);
    rotated.p_data.resize(m_size.width * m_size.height * m_channels);
    for (int y = 0; y < m_size.height; ++y) {
        for (int x = 0; x < m_size.width; ++x) {
            for (int c = 0; c < m_channels; ++c) {
                rotated.p_data[(x * m_size.height + (m_size.height - y - 1)) * m_channels + c] =
                    p_data[(y * m_size.width + x) * m_channels + c];
            }
        }
    }
    return rotated;
}

Expect<Image, String> Image::Resize(const int new_width, const int new_height) const
{
    if (new_width <= 0 || new_height <= 0) {
        return std::unexpected("Invalid image dimensions for resizing.");
    }

    const size_t new_size{static_cast<size_t>(new_width * new_height * m_channels)};
    std::vector<stbi_uc> resized_data(new_size);
    if (!stbir_resize_uint8(p_data.data(), m_size.width, m_size.height, 0, resized_data.data(), new_width, new_height,
                            0, m_channels)) {
        return std::unexpected("Failed to resize image.");
    }

    return Image(std::move(resized_data), ImageSize{new_width, new_height}, m_channels);
}

std::span<const stbi_uc> Image::data() const
{
    return {p_data.data(), static_cast<size_t>(m_size.width * m_size.height * m_channels)};
}

int Image::GetWidth() const noexcept { return m_size.width; }
int Image::GetHeight() const noexcept { return m_size.height; }
int Image::GetChannels() const noexcept { return m_channels; }
int Image::GetArea() const noexcept { return m_size.width * m_size.height; }
ImageSize Image::GetSize() const noexcept { return m_size; }

Image::Image(std::vector<stbi_uc> data, const ImageSize size, const int channels)
    : p_data(std::move(data)), m_size(size), m_channels(channels)
{
}

Image::Image(const Image &other) : m_size(other.m_size), m_channels(other.m_channels)
{
    p_data = other.p_data; // Simple vector copy
}

Image &Image::operator=(const Image &other)
{
    if (this != &other) {
        reset();
        p_data = other.p_data; // Simple vector copy
        m_size = other.m_size;
        m_channels = other.m_channels;
    }
    return *this;
}

Image::Image(Image &&other) noexcept
    : p_data(std::move(other.p_data)), m_size(other.m_size), m_channels(other.m_channels)
{
    other.reset();
}

Image &Image::operator=(Image &&other) noexcept
{
    if (this != &other) {
        p_data.swap(other.p_data); // Efficient move for vectors
        m_size = other.m_size;
        m_channels = other.m_channels;
        other.reset();
    }
    return *this;
}

Image::~Image() { reset(); }

void Image::reset()
{
    p_data.clear();
    m_size = ImageSize{0, 0};
    m_channels = 0;
}

} // namespace gouda
