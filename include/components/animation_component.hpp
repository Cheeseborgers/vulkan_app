#pragma once
/**
 * @file component/animation_component.hpp
 * @author GoudaCheeseburgers
 * @date 2025-05-07
 * @brief Application animation component module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <string>
#include <unordered_map>

#include "containers/small_vector.hpp"
#include "core/types.hpp"
#include "math/math.hpp"
#include "renderers/render_data.hpp"

struct Animation {
    std::string name{};
    gouda::Vector<gouda::math::Vec4> frames{}; // List of UV rects
    gouda::Vector<f32> frame_durations{};      // Seconds per frame
    bool looping = true;

    constexpr Animation() = default;
    Animation(std::string_view name_, gouda::Vector<gouda::math::Vec4> frames_, gouda::Vector<f32> frame_durations_,
              bool looping);
};

struct AnimationComponent {
    std::string current_animation{};
    f32 animation_time = 0.0f;
    int current_frame = 0;
    std::unordered_map<std::string, Animation> animations{};

    constexpr AnimationComponent() = default;

    void Update(f32 delta_time, gouda::InstanceData &render_data);
};
