#pragma once

#include "stb_image.h"

#include <expected>
#include <memory>
#include <span>
#include <string_view>

// Define ImageData properly
using ImageData = stbi_uc;

struct ImageSize {
    int width{};
    int height{};
};

// Custom deleter for stb_image memory management
struct StbImageDeleter {
    void operator()(stbi_uc *data) const { stbi_image_free(data); }
};

class Image {
public:
    static std::expected<Image, std::string> Load(const std::string_view filename,
                                                  int desired_channels = STBI_rgb_alpha);

    Image(Image &&other) noexcept;
    Image &operator=(Image &&other) noexcept;

    Image(const Image &) = delete;
    Image &operator=(const Image &) = delete;
    ~Image();

    [[nodiscard]] std::span<const stbi_uc> GetData() const;
    [[nodiscard]] int GetWidth() const noexcept;
    [[nodiscard]] int GetHeight() const noexcept;
    [[nodiscard]] int GetChannels() const noexcept;
    [[nodiscard]] int GetArea() const noexcept;
    [[nodiscard]] ImageSize GetSize() const noexcept;

private:
    explicit Image(stbi_uc *data, ImageSize size, int channels);
    void reset();

    std::unique_ptr<stbi_uc, StbImageDeleter> p_data{nullptr}; // Correct ownership management
    ImageSize m_size{};
    int m_channels{};
};
