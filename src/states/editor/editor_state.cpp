/**
 * @file editor_state.cpp
 * @author GoudaCheeseburgers
 * @date 2025-07-06
 * @brief Application editor state module implementation
 */
#include "states/editor_state.hpp"

#include <nlohmann/json.hpp>

#include "debug/logger.hpp"
#include "ui/editor_popups.hpp"

// TODO: Sort constructor ordering for panels/menus to be similar as possible.
EditorState::EditorState(SharedContext &context, StateStack &state_stack)
    : State(context, state_stack, "EditorState"),
    p_current_scene{nullptr},
    m_top_menu{context,
                 28.0f,
                 colours::editor_panel_colour,
                 {5.0f, 5.0f},
                 1,
                 20.0f,
                 colours::editor_panel_primary_font_colour},
      m_side_panel{context,
                   {0.2f, 1.0f},
                   {10.0f, 0.0f},
                   colours::editor_panel_colour,
                   "PANEL",
                   1,
                   colours::editor_panel_primary_font_colour,
                   50.0F},
      m_debug_panel{context, {250.0f, 250.0f}, colours::debug_panel_colour, 1, 20.0f},
      m_ui_manager{context},
      m_auto_save{false},
      m_scene_modified{true},
      m_exit_requested{false},
      m_show_entity_popups{true}
{
    m_quad_instances.reserve(app_constants::max_quads);
    m_text_instances.reserve(app_constants::max_glyphs);

    // TODO: Load Editor settings

    LoadScene("new_scene.json");

    // TODO: DO this better
    m_debug_panel.instance.position.y -= m_top_menu.GetSize().y;
}

void EditorState::HandleInput()
{

    // TODO: Move this into buttons. But do we keep keys too?
    if (m_exit_requested) {
        if (m_context.input_handler->IsKeyPressed(gouda::Key::Y)) {
            // Confirm exit
            m_context.window->Close();
        }
        else if (m_context.input_handler->IsKeyPressed(gouda::Key::N)) {
            // Cancel exit
            m_exit_requested = false;
        }
        return; // Exit early to prevent other input from being processed
    }

    static std::unordered_map<gouda::Key, bool> key_was_pressed;

    auto handle_toggle = [&](const gouda::Key key, auto &&action) {
        const bool is_pressed{m_context.input_handler->IsKeyPressed(key)};
        bool &was_pressed{key_was_pressed[key]};

        if (is_pressed && !was_pressed) {
            action(); // Perform action on key press
        }

        was_pressed = is_pressed;
    };

    handle_toggle(gouda::Key::P, [&] { m_side_panel.ToggleVisibility(); });
    handle_toggle(gouda::Key::L, [&] { ToggleSelectedEntityPopups(); });
    handle_toggle(gouda::Key::F3, [&] { m_debug_panel.ToggleVisibility(); });
}

void EditorState::Update(const f32 delta_time)
{
    // TODO: Handle multi-select
    m_top_menu.Update(delta_time);

    // Check if entities are hovered
    if (!m_exit_requested) {
        m_side_panel.Update(delta_time);
        const gouda::Vec2 mouse_position{m_context.input_handler->GetMousePositionFloat()};
        p_current_scene->selected_entity = PickTopEntityAt(mouse_position);
    }
}

void EditorState::Render(const f32 delta_time)
{
    // TODO: Figure out a document laying once and for all

    // Render the scene visible in the scene camera frustum.
    const auto &frustum = m_context.scene_camera->GetFrustumData();

    for (const auto &entity : p_current_scene->editor_entities) {
        if (IsInFrustum(entity.render_data.position, entity.render_data.size, frustum)) {
            m_quad_instances.push_back(entity.render_data);
        }
    }

    // Draw UI
    m_top_menu.Draw(m_quad_instances, m_text_instances);
    m_debug_panel.Draw(m_quad_instances, m_text_instances);

    // Handle exit confirmation or side panel
    if (m_scene_modified && m_exit_requested) {
        DrawExitConfirmationPopup();
    }
    else {
        m_side_panel.Draw(m_quad_instances, m_text_instances);

        // Handle selected entity outline and popup
        if (p_current_scene->selected_entity != nullptr) {
            m_quad_instances.emplace_back(SelectionOutline{p_current_scene->selected_entity->render_data}.instance);

            if (m_show_entity_popups) {
                DrawEntityPopup();
            }
        }
    }

    // Perform the final render
    m_context.renderer->Render(delta_time, *m_context.uniform_data, m_quad_instances, m_text_instances,
                               m_particles_instances);

    ClearInstances();
}

void EditorState::OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size)
{
    m_framebuffer_size = new_framebuffer_size;
    m_side_panel.OnFramebufferResize(new_framebuffer_size);
}

void EditorState::OnEnter()
{
    const std::vector<gouda::InputHandler::ActionBinding> editor_bindings{
        {gouda::Key::Escape, gouda::ActionState::Pressed, [this] { RequestExit(); }},
        {gouda::Key::A, gouda::ActionState::Pressed,
         [this] { m_context.scene_camera->SetMovementFlag(gouda::CameraMovement::MoveLeft); }},
        {gouda::Key::A, gouda::ActionState::Released,
         [this] { m_context.scene_camera->ClearMovementFlag(gouda::CameraMovement::MoveLeft); }},
        {gouda::Key::D, gouda::ActionState::Pressed,
         [this] { m_context.scene_camera->SetMovementFlag(gouda::CameraMovement::MoveRight); }},
        {gouda::Key::D, gouda::ActionState::Released,
         [this] { m_context.scene_camera->ClearMovementFlag(gouda::CameraMovement::MoveRight); }},
        {gouda::Key::W, gouda::ActionState::Pressed,
         [this] { m_context.scene_camera->SetMovementFlag(gouda::CameraMovement::MoveUp); }},
        {gouda::Key::W, gouda::ActionState::Released,
         [this] { m_context.scene_camera->ClearMovementFlag(gouda::CameraMovement::MoveUp); }},
        {gouda::Key::S, gouda::ActionState::Pressed,
         [this] { m_context.scene_camera->SetMovementFlag(gouda::CameraMovement::MoveDown); }},
        {gouda::Key::S, gouda::ActionState::Released,
         [this] { m_context.scene_camera->ClearMovementFlag(gouda::CameraMovement::MoveDown); }},
        {gouda::Key::Q, gouda::ActionState::Pressed,
         [this] { m_context.scene_camera->SetMovementFlag(gouda::CameraMovement::ZoomIn); }},
        {gouda::Key::Q, gouda::ActionState::Released,
         [this] { m_context.scene_camera->ClearMovementFlag(gouda::CameraMovement::ZoomIn); }},
        {gouda::Key::E, gouda::ActionState::Pressed,
         [this] { m_context.scene_camera->SetMovementFlag(gouda::CameraMovement::ZoomOut); }},
        {gouda::Key::E, gouda::ActionState::Released,
         [this] { m_context.scene_camera->ClearMovementFlag(gouda::CameraMovement::ZoomOut); }},
        {gouda::Key::Space, gouda::ActionState::Pressed, [this] { m_context.scene_camera->Shake(10.0f, 0.5f); }},
        {gouda::Key::C, gouda::ActionState::Pressed, [this] { m_context.renderer->ToggleComputeParticles(); }},
    };

    m_context.input_handler->LoadStateBindings(m_state_id, editor_bindings);
    m_context.input_handler->SetActiveState(m_state_id);
}

void EditorState::OnExit()
{
    // TODO: Figure out how we are going to set and get the scene name/filename
    if (m_auto_save && m_scene_modified) {
        //SaveScene("new_scene.json");
    }

    m_context.input_handler->UnloadStateBindings(m_state_id);
}

void EditorState::LoadScene(StringView scene_file_path)
{
    /*// Ensure the scene directory exists
    FilePath scene_path{scene_file_path};
    FilePath scenes_directory{scene_path.parent_path()};
    std::error_code error_code;
    if (!std::filesystem::exists(scenes_directory, error_code)) {
        if (!std::filesystem::create_directories(scenes_directory, error_code)) {
            APP_LOG_ERROR("Failed to create scenes directory '{}'. Error: {}", scenes_directory.string(),
    error_code.message()); return;
        }
    }

    // Check if the scene file exists
    if (!std::filesystem::exists(scene_path, error_code)) {
        APP_LOG_WARNING("Scene file '{}' does not exist, creating with default settings.", scenes_directory.string());
        return;
    }

    // Check if the scene file is empty
    if (std::filesystem::file_size(scene_path, error_code) == 0) {
        APP_LOG_WARNING("Scene file '{}' is empty, using default settings.", scene_path.string());
        return;
    }

    std::ifstream file(scene_path);
    if (!file.is_open()) {
        APP_LOG_ERROR("Failed to open settings file '{}'.", scene_path.string());
        return;
    }

    try {
        nlohmann::json json_data;
        file >> json_data;

        if (!json_data.is_object()) {
            throw std::runtime_error("Invalid JSON format (not an object).");
        }

        // json_data.get_to(m_settings);
    }
    catch (const std::exception &error) {
        APP_LOG_WARNING("Failed to parse settings file '{}'. Error: {}, using default settings.", scene_path.string(),
                        error.what());
    }*/

    EditorScene scene(scene_file_path);
    m_editor_scenes.emplace_back(scene);
    p_current_scene = &m_editor_scenes.back();

    // TODO: Load scene from file
    const gouda::Vector<gouda::InstanceData> instances{
        {{100.0f, 304.8f, -0.522f}, {81.9f, 453.95f}, 0.0f, 4},
        {{200.0f, 200.8f, -0.587f}, {281.9f, 453.95f}, 0.0f, 4},
        {{567.08f, 69.08f, -0.507f}, {188.0f, 101.51f}, 0.0f, 2},
        {{634.96f, 45.58f, -0.503f}, {246.26f, 227.67f}, 0.0f, 4},
    };

    p_current_scene->editor_entities.reserve(instances.size());
    for (const auto &instance : instances) {
        p_current_scene->editor_entities.emplace_back(instance, EntityType::Quad);
    }

    m_scene_modified = true;
}
void EditorState::SaveScene(StringView scene_file_path)
{
    // Return early for no scene changes
    if (!m_scene_modified) {
        return;
    }

    /*if (std::ofstream file(scene_file_path.data()); !file.is_open()) {
        APP_LOG_ERROR("Could not open scene file '{}' for saving.", scene_file_path.data());
        return;
    }

    try {
        //const nlohmann::json json_data = m_settings; // Should be a JSON object
        //file << json_data.dump(4);             // Pretty print with indentation
    }
    catch (const std::exception &e) {
        APP_LOG_ERROR("Failed to save scene file '{}'. Reason: {}.", scene_file_path.data(), e.what());
        return;
    }

    APP_LOG_DEBUG("Scene saved to '{}", scene_file_path.data());*/

    // Unflag the scene as changed
    m_scene_modified = false;
}

// Private functions ----------------------------------------------------
void EditorState::DrawEntityPopup()
{
    EntityPopup popup{m_context,
                      p_current_scene->selected_entity,
                      {300.0f, 300.0f},
                      colours::editor_panel_colour,
                      0,
                      {10.0f, 10.0f},
                      2,
                      20.0f,
                      colours::editor_panel_primary_font_colour};

    popup.Draw(m_quad_instances, m_text_instances);
}

void EditorState::DrawExitConfirmationPopup()
{
    ExitConfirmationPopup{m_context,
                          {100.0f, 100.0f, -0.1111f},
                          {500.0f, 500.0f},
                          colours::editor_panel_colour,
                          0,
                          {10.0f, 10.0f},
                          1,
                          20.0f,
                          colours::editor_panel_primary_font_colour}
        .Draw(m_quad_instances, m_text_instances);
}

Entity *EditorState::PickTopEntityAt(const gouda::Vec2 &mouse_position) const
{
    Entity *top_entity{nullptr};
    f32 max_z{-constants::infinity};

    for (auto &entity : p_current_scene->editor_entities) {
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
void EditorState::AddEntity(const Entity &entity)
{
    p_current_scene->editor_entities.emplace_back(entity);
    m_scene_modified = true;
}

void EditorState::ToggleSelectedEntityPopups() { m_show_entity_popups = !m_show_entity_popups; }
void EditorState::RequestExit()
{
    if (m_scene_modified) {
        m_exit_requested = true;
    }
    else {
        m_context.window->Close();
    }
}
void EditorState::ToggleExitRequested() { m_exit_requested = !m_exit_requested; }

void EditorState::ClearInstances()
{
    m_quad_instances.clear();
    m_text_instances.clear();
    m_particles_instances.clear();
}