#pragma once
/**
 * @file states/editor_state.hpp
 * @author GoudaCheeseburgers
 * @date 2025-07-06
 * @brief Application editor state module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <unordered_map>

#include "core/state_stack.hpp"

#include "debug/logger.hpp"

#include "core/constants.hpp"
#include "entities/entity.hpp"
#include "state.hpp"
#include "states_common.hpp"
#include "ui/debug_panel.hpp"
#include "ui/side_panel.hpp"
#include "ui/ui_manager.hpp"

struct SelectionOutline {
    explicit SelectionOutline(const gouda::InstanceData &selected_instance)
    {
        instance.size = selected_instance.size + 4.0f;
        instance.position.x = selected_instance.position.x - 2.0f;
        instance.position.y = selected_instance.position.y - 2.0f;
        instance.position.z = selected_instance.position.z - 0.01f;
        instance.texture_index = 0;
        instance.apply_camera_effects = true;
        instance.colour = {0.3f, 0.9f, 0.3f, 1.0f};
    }

    gouda::InstanceData instance;
};

struct TopMenu {
    gouda::InstanceData instance;
};

struct EntityPopup {
    EntityPopup(const SharedContext &shared_context, Entity *entity) : shared_context(shared_context), entity(entity)
    {
        // TODO: Fix rendering z layering for good!!
        instance.position = {entity->render_data.position.x + 10.0f, entity->render_data.position.y + 10.0f,
                             -0.111111f};
        instance.size = {300.0f, 300.0f};
        instance.texture_index = 0;
        instance.apply_camera_effects = false;
        instance.colour = colours::editor_panel_colour;
    }

    void HandleInput() {}

    void Update(f32 delta_time) {}

    void Render(std::vector<gouda::InstanceData> &quad_instances, std::vector<gouda::TextData> &text_instances)
    {
        quad_instances.emplace_back(instance);

        f32 current_position_y{instance.position.y + instance.size.y};

        if (!entity) {
            APP_LOG_INFO("entity is null");
            return;
        }

        // TODO: Sort text z laying
        APP_LOG_INFO("Positon: {}", entity->render_data.position.ToString());
        gouda::Vector<String> lines = {std::format("POSITION 9 {:.2f}:{:.2f}:{:.2f}", entity->render_data.position.x,
                                                   entity->render_data.position.y, entity->render_data.position.z),
            std::format("TEXTURE {}", entity->render_data.texture_index), "SOME MORE TEXT 9", "SOME MORE TEXT"};

        for (const auto &line : lines) {
            current_position_y -= font_scale + padding.y;
            gouda::Vec3 position{instance.position.x + padding.x, current_position_y - padding.y, -0.1f};
            shared_context.renderer->DrawText(line, position, colours::editor_panel_primary_font_colour, font_scale,
                                              font_id, text_instances);
        }
    }

    gouda::InstanceData instance;
    SharedContext shared_context;
    Entity *entity;

    u32 font_id = 2;
    f32 font_scale = 20.0f;
    gouda::Vec2 padding{10.0f, 10.0f};
};

class EditorState final : public State {
public:
    explicit EditorState(SharedContext &context, StateStack &state_stack);
    ~EditorState() override = default;

    [[nodiscard]] StateID GetID() const override;

    void HandleInput() override;
    void Update(f32 delta_time) override;
    void Render(f32 delta_time) override;
    void OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size) override;
    [[nodiscard]] bool IsOpaque() const override { return false; }

    void OnEnter() override;
    void OnExit() override;

    void LoadScene();
    void SaveScene();

private:
    Entity *PickTopEntityAt(const gouda::Vec2 &mouse_position);
    void ToggleSelectedEntityPopups();
    void ClearInstances();

private:
    std::vector<gouda::InstanceData> m_quad_instances;
    std::vector<gouda::TextData> m_text_instances;
    std::vector<gouda::ParticleData> m_particles_instances;

    gouda::Vector<Entity> m_editor_entities;
    Entity *p_selected_entity;
    bool m_show_entity_popups;

    TopMenu m_top_menu;
    SidePanel m_side_panel;
    DebugPanel m_debug_panel;

    UIManager m_ui_manager;
};
