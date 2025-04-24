#pragma once

/**
 * @file defer.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-24
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
#include <functional>
#include <utility>

namespace gouda {
namespace utils {

template <typename F>
class Defer {
public:
    explicit Defer(F &&f) noexcept : func_(std::forward<F>(f)), active_(true) {}

    Defer(Defer &&other) noexcept : func_(std::move(other.func_)), active_(other.active_) { other.dismiss(); }

    // Non-copyable
    Defer(const Defer &) = delete;
    Defer &operator=(const Defer &) = delete;

    ~Defer()
    {
        if (active_) {
            func_();
        }
    }

    void dismiss() noexcept { active_ = false; }

private:
    F func_;
    bool active_;
};

// Deduction guide for C++23
template <typename F>
Defer(F) -> Defer<F>;

} // namespace utils

} // namespace gouda

// Convenience macro
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define DEFER auto CONCAT(_defer_, __LINE__) = ::gouda::utils::Defer([&]()