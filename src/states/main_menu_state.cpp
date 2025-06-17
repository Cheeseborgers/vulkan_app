/**
 * @file main_menu_state.cpp
 * @author GoudaCheeseburgers
 * @date 2025-07-06
 * @brief Application main menu state module implementation
 */
#include "states/main_menu_state.hpp"

#include "debug/logger.hpp"

MainMenuState::MainMenuState(SharedContext &context, StateStack &state_stack) : State(context, state_stack, "MainMenuState")
{
    // Get framebuffer size
    gouda::Vec2 screen_size;
    screen_size.x = static_cast<f32>(m_context.renderer->GetFramebufferSize().width);
    screen_size.y = static_cast<f32>(m_context.renderer->GetFramebufferSize().height);

    // Validate screen size
    if (screen_size.x <= 0 || screen_size.y <= 0) {
        APP_LOG_ERROR("Invalid framebuffer size: %f x %f", screen_size.x, screen_size.y);
        throw std::runtime_error("Invalid framebuffer size");
    }

    // Create background quad (full screen)
    gouda::InstanceData background{};
    background.position = {0.f, 0.f, -0.599f};
    background.size = screen_size;
    background.apply_camera_effects = 0;
    background.texture_index = 2;
    m_quad_instances.push_back(background);

    // Create centered menu panel quad
    gouda::Vec2 menu_size{screen_size.x * 0.5f, screen_size.y * 0.5f}; // 50% width, 40% height
    gouda::Vec3 menu_position{
        screen_size.x / 2.f - menu_size.x / 2.f, // X: Center minus half width
        screen_size.y / 2.f - menu_size.y / 2.f, // Y: Center minus half height
        -0.598f                                  // Z: Slightly above background
    };

    gouda::InstanceData menu_panel_instance{};
    menu_panel_instance.position = menu_position;
    menu_panel_instance.size = menu_size;
    menu_panel_instance.texture_index = 1;
    menu_panel_instance.apply_camera_effects = 0;
    m_quad_instances.emplace_back(menu_panel_instance);

    // Add four button quads
    constexpr f32 button_padding{10.0f};
    const gouda::Colour text_colour{1.0f, 1.0f, 0.0f, 1.0f};
    constexpr std::array<StringView, 4> button_labels{"START GAME", "OPTIONS", "EDITOR", "EXIT"};

    gouda::Vec2 button_size{menu_size.x * 0.8f, menu_size.y / 5.f}; // Buttons are 80% of panel width, 1/5 panel height
    const f32 total_button_height =
        button_size.y * button_labels.size() + button_padding * (button_labels.size() - 1); // 4 buttons, 3 gaps
    const f32 start_y =
        menu_position.y + (menu_size.y - total_button_height) / 2.f; // Center buttons vertically in panel

    for (int i = 0; i < button_labels.size(); ++i) {
        // Center horizontally in panel, Stack vertically with padding, Z: above panel
        gouda::Vec3 button_position{menu_position.x + (menu_size.x - button_size.x) / 2.f,
                                    start_y + static_cast<f32>(i) * (button_size.y + button_padding), -0.597f};

        gouda::InstanceData button_instance_data{};
        button_instance_data.position = button_position;
        button_instance_data.size = button_size;
        button_instance_data.texture_index = 3;
        button_instance_data.apply_camera_effects = 0;
        m_quad_instances.emplace_back(button_instance_data);
        // Store button bounds for input handling
        m_button_bounds.emplace_back(button_position.x, button_position.y, button_size.x, button_size.y);

        // Add button text
        gouda::Vec3 text_position{button_position.x + button_size.x / 2.f, button_position.y + button_size.y / 2.f,
                                  -0.596f};
        m_context.renderer->DrawText(button_labels[i], text_position, text_colour, 20.0f, 2, m_text_instances,
                                     gouda::TextAlign::Center, false);
    }

    // Add menu title text, slightly above buttons.
    const gouda::Vec3 title_position{menu_position.x + menu_size.x / 2.f, -(menu_position.y - 20.f), -0.596f};
    m_context.renderer->DrawText("GOUDA RENDERER MENU", title_position, text_colour, 50.0f, 2, m_text_instances,
                                 gouda::TextAlign::Center, false);
}

void MainMenuState::HandleInput() {}

void MainMenuState::Update(const f32 delta_time) {}

void MainMenuState::Render(const f32 delta_time)
{
    m_context.renderer->Render(delta_time, *m_context.uniform_data, m_quad_instances, m_text_instances,
                               m_particles_instances);
}

void MainMenuState::OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size)
{
    m_framebuffer_size = new_framebuffer_size;
}

void MainMenuState::OnEnter()
{
    APP_LOG_INFO("Entered MainMenuState");
    State::OnEnter();
}

void MainMenuState::OnExit()
{
    APP_LOG_INFO("Exiting MainMenuState");
    State::OnExit();
}