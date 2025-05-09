/**
 * @file components/animation_component.cpp
 * @author GoudaCheeseburgers
 * @date 2025-05-07
 * @brief Application animation component module implementation
 */
#include "components/animation_component.hpp"

Animation::Animation(std::string_view name_, gouda::Vector<gouda::math::Vec4> frames_,
                     gouda::Vector<f32> frame_durations_, bool looping_)
    : name{name_}, frames{frames_}, frame_durations{frame_durations_}, looping{looping_}
{
}

void AnimationComponent::Update(f32 delta_time, gouda::InstanceData &render_data)
{
    if (animations.empty() || !animations.contains(current_animation))
        return;

    auto &anim = animations.at(current_animation);
    if (anim.frames.empty() || anim.frame_durations.empty())
        return;

    animation_time += delta_time;

    f32 frame_duration{anim.frame_durations[current_frame]};
    if (animation_time >= frame_duration) {
        animation_time -= frame_duration;
        ++current_frame;

        if (current_frame >= static_cast<int>(anim.frames.size())) {
            if (anim.looping) {
                current_frame = 0;
            }
            else {
                current_animation = "idle"; // Or handle differently
                current_frame = 0;
            }
        }

        render_data.sprite_rect = anim.frames[current_frame];
    }
}