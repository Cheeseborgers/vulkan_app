#pragma once
/**
 * @file ui/button.hpp
 * @author GoudaCheeseburgers
 * @date 2025-11-06
 * @brief Engine math easing module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "math.hpp"

#include <cmath>

namespace gouda::math {
    // Easing functions: Input t is [0, 1], output is [0, 1]
    namespace easing {
        inline float Linear(const float t) {
            return t;
        }

        inline float EaseInQuad(const float t) {
            return t * t;
        }

        inline float EaseOutQuad(const float t) {
            return 1.0f - (1.0f - t) * (1.0f - t);
        }

        inline float EaseInOutQuad(const float t) {
            return t < 0.5f ? 2.0f * t * t : 1.0f - math::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        }

        inline float SmoothStep(const float t) {
            return t * t * (3.0f - 2.0f * t);
        }

        inline float EaseInOutCubic(const float t) {
            return t < 0.5f ? 4.0f * t * t * t : 1.0f - math::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
        }

        inline float EaseOutElastic(const float t) {
            if (t <= 0.0f) return 0.0f;
            if (t >= 1.0f) return 1.0f;
            const float c4 = (2.0f * 3.14159f) / 3.0f;
            return math::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
        }

        inline float EaseOutBounce(float t) {
            constexpr float n1 = 7.5625f;
            constexpr float d1 = 2.75f;
            if (t < 1.0f / d1) {
                return n1 * t * t;
            } else if (t < 2.0f / d1) {
                t -= 1.5f / d1;
                return n1 * t * t + 0.75f;
            } else if (t < 2.5f / d1) {
                t -= 2.25f / d1;
                return n1 * t * t + 0.9375f;
            } else {
                t -= 2.625f / d1;
                return n1 * t * t + 0.984375f;
            }
        }

        // Enum to select easing function
        enum class EasingType {
            Linear,
            EaseInQuad,
            EaseOutQuad,
            EaseInOutQuad,
            SmoothStep,
            EaseInOutCubic,
            EaseOutElastic,
            EaseOutBounce
        };

        // Helper to apply easing based on type
        inline float ApplyEasing(const float t, const EasingType type) {
            switch (type) {
                case EasingType::Linear: return Linear(t);
                case EasingType::EaseInQuad: return EaseInQuad(t);
                case EasingType::EaseOutQuad: return EaseOutQuad(t);
                case EasingType::EaseInOutQuad: return EaseInOutQuad(t);
                case EasingType::SmoothStep: return SmoothStep(t);
                case EasingType::EaseInOutCubic: return EaseInOutCubic(t);
                case EasingType::EaseOutElastic: return EaseOutElastic(t);
                case EasingType::EaseOutBounce: return EaseOutBounce(t);
                default: return t;
            }
        }
    } // namespace easing

} // namespace gouda::math
