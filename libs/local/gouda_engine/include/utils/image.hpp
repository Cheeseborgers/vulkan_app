#pragma once
/**
 * @file utils/image.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-01
 *
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <expected>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "core/types.hpp"
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"

namespace gouda {

enum class ImageChannels { RGB, RGBA };

/**
 * @brief Image class for loading, manipulating, and saving images.
 */
class Image {
public:
    /// Loads an image from file.
    static Expect<Image, std::string> Load(std::string_view filename, int desired_channels = STBI_rgb_alpha);

    /// Saves the image to a file.
    bool Save(std::string_view filename) const;

    /// Converts the image to grayscale.
    Image ToGrayscale() const;

    /// Flips the image horizontally.
    void FlipHorizontal();

    /// Rotates the image 90 degrees clockwise.
    Image Rotate90() const;

    /// Resizes the image.
    Expect<Image, std::string> Resize(int new_width, int new_height) const;

    /// Returns raw pixel data.
    std::span<const stbi_uc> data() const;

    /// Accessor functions.
    int GetWidth() const noexcept;
    int GetHeight() const noexcept;
    int GetChannels() const noexcept;
    int GetArea() const noexcept;
    ImageSize GetSize() const noexcept;

    /// Copy constructor.
    Image(const Image &other);

    /// Copy assignment operator.
    Image &operator=(const Image &other);

    /// Move constructor.
    Image(Image &&other) noexcept;

    /// Move assignment operator.
    Image &operator=(Image &&other) noexcept;

    /// Destructor.
    ~Image();

private:
    /// Private constructor for internal use.
    explicit Image(std::vector<stbi_uc> data, ImageSize size, int channels);

    /// Clears image data.
    void reset();

    std::vector<stbi_uc> p_data; ///< Pixel data.
    ImageSize m_size;            ///< Image dimensions.
    int m_channels;              ///< Number of color channels.
};

} // namespace gouda
