#pragma once
/**
 * @file default_allocator.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-25
 * @brief Engine module
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
#include "core/types.hpp"

namespace gouda {

template <typename T>
class DefaultAllocator {
public:
    using value_type = T;

    DefaultAllocator() noexcept = default;
    template <typename U>
    DefaultAllocator(const DefaultAllocator<U> &) noexcept
    {
    }

    T *allocate(size_t n)
    {
        if (n > Constants::size_t_max / sizeof(T)) {
            throw std::bad_alloc();
        }
        return static_cast<T *>(::operator new[](n * sizeof(T)));
    }

    void deallocate(T *p, size_t) noexcept { ::operator delete[](p); }
};

template <typename T, typename U>
bool operator==(const DefaultAllocator<T> &, const DefaultAllocator<U> &) noexcept
{
    return true;
}

template <typename T, typename U>
bool operator!=(const DefaultAllocator<T> &, const DefaultAllocator<U> &) noexcept
{
    return false;
}
} // namespace gouda