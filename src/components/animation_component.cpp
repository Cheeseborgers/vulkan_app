/**
 * @file animation_component.cpp
 * @author GoudaCheeseburgers
 * @date 2025-05-07
 * @brief Application animation component module implementation
 */
#include "components/animation_component.hpp"

#include "debug/logger.hpp"

#include <utility>

Animation::Animation(std::string_view name_, gouda::Vector<gouda::math::Vec4> frames_,
                     const gouda::Vector<f32> &frame_durations_, bool looping_)
    : name{name_}, frames{std::move(frames_)}, frame_durations{frame_durations_}, looping{looping_}
{
}

void AnimationComponent::Update(const f32 delta_time, gouda::InstanceData &render_data) {
    if (animations.empty() || !animations.contains(current_animation)) {
        return;
    }

    auto &anim = animations.at(current_animation);
    if (anim.frames.empty() || anim.frame_durations.empty()) {
        return;
    }

    // Advance animation time
    animation_time += delta_time;

    // Calculate total duration of one loop
    f32 total_duration = 0.0f;
    for (const auto duration : anim.frame_durations) {
        total_duration += duration;
    }

    // Handle looping or clamp to last frame
    if (animation_time >= total_duration) {
        if (anim.looping) {
            animation_time = std::fmod(animation_time, total_duration);
        } else {
            animation_time = total_duration;
            current_frame = static_cast<int>(anim.frames.size()) - 1;
            if (animations.contains("idle")) {
                current_animation = "idle";
                animation_time = 0.0f;
                current_frame = 0;
            }
            if (render_data.is_atlas) {
                render_data.sprite_rect = anim.frames[current_frame];
            } else {
                render_data.sprite_rect = {0.0f, 0.0f, 1.0f, 1.0f}; // Full texture for non-atlas
            }
            return;
        }
    }

    // Find current frame based on accumulated time
    f32 accumulated_time = 0.0f;
    current_frame = 0;
    for (size_t i = 0; i < anim.frame_durations.size() && i < anim.frames.size(); ++i) {
        accumulated_time += anim.frame_durations[i];
        if (animation_time < accumulated_time) {
            current_frame = static_cast<int>(i);
            break;
        }
    }

    // Ensure current_frame is valid
    current_frame = std::min(current_frame, static_cast<int>(anim.frames.size()) - 1);

    // Update render data with current frame's UV rect
    if (render_data.is_atlas) {
        render_data.sprite_rect = anim.frames[current_frame];
    } else {
        render_data.sprite_rect = {0.0f, 0.0f, 1.0f, 1.0f}; // Full texture for non-atlas
    }

    APP_LOG_DEBUG("animation: {}, frame: {}, rect: {}:{}:{}:{}", current_animation, current_frame,
                  render_data.sprite_rect.x, render_data.sprite_rect.y, render_data.sprite_rect.z,
                  render_data.sprite_rect.w);
}