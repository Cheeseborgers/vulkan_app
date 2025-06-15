/**
 * @file editor_state.cpp
 * @author GoudaCheeseburgers
 * @date 2025-07-06
 * @brief Application editor state module implementation
 */
#include "states/editor_state.hpp"
#include "debug/logger.hpp"

EditorState::EditorState(SharedContext &context, StateStack &state_stack)
    : State(context, state_stack),
      p_selected_entity{nullptr},
      m_show_entity_popups{true},
      m_side_panel{context, {0.2f, 1.0f}, {10.0f, 0.0f}, {0.2f, 0.2f, 0.2f, 1.0f}, "PANEL", 1, {0.3f, 0.3f, 0.3f, 1.0f},
                   50.0F},
      m_debug_panel{context, {250.0f, 250.0f}, {0.1f, 0.8f, 0.6f, 1.0f}, 1, 20.0f},
      m_ui_manager{context}
{
    m_quad_instances.reserve(app_constants::max_quads);
    m_text_instances.reserve(app_constants::max_glyphs);

    LoadScene();

    // Setup top menu
    m_top_menu.instance.size = {static_cast<f32>(m_context.window->GetWindowSize().width), 20.0f};
    m_top_menu.instance.position = {
        0.0f, static_cast<f32>(m_context.window->GetWindowSize().height) - m_top_menu.instance.size.y, -0.9999};
    m_top_menu.instance.colour = {0.2f, 0.2f, 0.2f, 1.0f};
    m_top_menu.instance.texture_index = 0;
    m_top_menu.instance.apply_camera_effects = false;

    // TODO: DO this better
    m_debug_panel.instance.position.y -= m_top_menu.instance.size.y;
}

State::StateID EditorState::GetID() const { return "EditorState"; }

void EditorState::HandleInput()
{
    static std::unordered_map<gouda::Key, bool> key_was_pressed;

    auto handle_toggle = [&](const gouda::Key key, auto &&action) {
        const bool is_pressed{m_context.input_handler->IsKeyPressed(key)};
        bool &was_pressed{key_was_pressed[key]};

        if (is_pressed && !was_pressed) {
            action(); // Perform action on key press
        }

        was_pressed = is_pressed;
    };

    handle_toggle(gouda::Key::P, [&] { m_side_panel.Toggle(); });
    handle_toggle(gouda::Key::L, [&] { ToggleSelectedEntityPopups(); });
    handle_toggle(gouda::Key::F3, [&] { m_debug_panel.Toggle(); });
}

void EditorState::Update(const f32 delta_time)
{
    m_side_panel.Update(delta_time);

    // Check if entities are hovered
    const auto mouse_position = m_context.input_handler->GetMousePositionFloat();
    p_selected_entity = PickTopEntityAt(mouse_position);

    // TODO: Handle multi-select
}

void EditorState::Render(const f32 delta_time)
{
    // Render the scene visible in the scene camera frustum.
    const auto &frustum = m_context.scene_camera->GetFrustumData();
    for (const auto &entity : m_editor_entities) {
        if (IsInFrustum(entity.render_data.position, entity.render_data.size, frustum)) {
            m_quad_instances.push_back(entity.render_data);
        }
    }

    if (p_selected_entity) {
        // Outline the hovered entity
        SelectionOutline selection_outline{p_selected_entity->render_data};
        m_quad_instances.emplace_back(selection_outline.instance);

        // Display entity popup if enabled
        if (m_show_entity_popups) {
            EntityPopup entity_popup{m_context, p_selected_entity};
            entity_popup.Render(m_quad_instances, m_text_instances);
        }
    }

    // Render the ui.
    m_debug_panel.Render(m_quad_instances, m_text_instances);

    // TODO: Figure out a document laying once and for all
    m_side_panel.Render(m_quad_instances, m_text_instances);
    m_quad_instances.emplace_back(m_top_menu.instance);

    // Render all the stuff
    m_context.renderer->Render(delta_time, *m_context.uniform_data, m_quad_instances, m_text_instances,
                               m_particles_instances);

    ClearInstances();
}
void EditorState::OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size)
{
    m_framebuffer_size = new_framebuffer_size;
    m_side_panel.OnFramebufferResize(new_framebuffer_size);
}

void EditorState::OnEnter() { State::OnEnter(); }
void EditorState::OnExit() { State::OnExit(); }

void EditorState::LoadScene()
{
    // TODO: Load scene from file
    const gouda::Vector<gouda::InstanceData> instances = {
        {{100.0f, 304.8f, -0.522f}, {81.9f, 453.95f}, 0.0f, 4},
        {{200.0f, 200.8f, -0.587f}, {281.9f, 453.95f}, 0.0f, 4},
        {{567.08f, 69.08f, -0.507f}, {188.0f, 101.51f}, 0.0f, 2},
        {{634.96f, 45.58f, -0.503f}, {246.26f, 227.67f}, 0.0f, 4},
    };

    m_editor_entities.reserve(instances.size());
    for (const auto &instance : instances) {
        m_editor_entities.emplace_back(instance, EntityType::Quad);
    }
}
void EditorState::SaveScene() {}

// Private functions ----------------------------------------------------
Entity *EditorState::PickTopEntityAt(const gouda::Vec2 &mouse_position)
{
    Entity *top_entity{nullptr};
    f32 max_z{-constants::infinity};

    for (auto &entity : m_editor_entities) {
        if (entity.GetBounds().Contains(mouse_position.x, mouse_position.y)) {
            const f32 z_position{entity.render_data.position.z};

            // Skip NaN just in case
            if (std::isnan(z_position)) {
                continue;
            }

            if (z_position >= max_z) {
                max_z = z_position;
                top_entity = &entity;
            }
        }
    }

    return top_entity;
}

void EditorState::ToggleSelectedEntityPopups() { m_show_entity_popups = !m_show_entity_popups; }

void EditorState::ClearInstances()
{
    m_quad_instances.clear();
    m_text_instances.clear();
    m_particles_instances.clear();
}