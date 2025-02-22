#include "image.hpp"

#include <stdexcept>
#include <utility>

std::expected<Image, std::string> Image::Load(const std::string_view filename, int desired_channels)
{
    int width, height, channels;
    stbi_uc *data = stbi_load(filename.data(), &width, &height, &channels, desired_channels);
    if (!data) {
        return std::unexpected(std::string("Failed to load image: ") + std::string(filename));
    }
    return Image(data, {width, height}, channels);
}

Image::Image(stbi_uc *data, ImageSize size, int channels) : p_data(data), m_size(size), m_channels(channels) {}

Image::Image(Image &&other) noexcept
    : p_data{std::move(other.p_data)}, m_size{other.m_size}, m_channels{other.m_channels}
{
    other.m_size = {0, 0};
    other.m_channels = 0;
}

Image &Image::operator=(Image &&other) noexcept
{
    if (this != &other) {
        reset();
        p_data = std::move(other.p_data);
        m_size = other.m_size;
        m_channels = other.m_channels;
        other.m_size = {0, 0};
        other.m_channels = 0;
    }
    return *this;
}

Image::~Image() { reset(); }

void Image::reset()
{
    p_data.reset();
    m_size = {0, 0};
    m_channels = 0;
}

std::span<const stbi_uc> Image::GetData() const
{
    return {p_data.get(), static_cast<size_t>(m_size.width * m_size.height * m_channels)};
}

int Image::GetWidth() const noexcept { return m_size.width; }
int Image::GetHeight() const noexcept { return m_size.height; }
int Image::GetChannels() const noexcept { return m_channels; }
int Image::GetArea() const noexcept { return m_size.width * m_size.height; }
ImageSize Image::GetSize() const noexcept { return m_size; }
