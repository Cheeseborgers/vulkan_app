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
#include "core/state_stack.hpp"

#include "debug/logger.hpp"

#include "core/constants.hpp"
#include "entities/entity.hpp"
#include "state.hpp"
#include "states_common.hpp"
#include "ui/debug_panel.hpp"
#include "ui/side_panel.hpp"
#include "ui/top_panel.hpp"
#include "ui/ui_manager.hpp"
#include "utils/filesystem.hpp"

struct SelectionOutline {
    explicit SelectionOutline(const gouda::InstanceData &selected_instance, const f32 outline_size = 2.0f)
    {
        // TODO: Fix layering here.
        instance.size = selected_instance.size + outline_size * 2.0f;
        instance.position.x = selected_instance.position.x - outline_size;
        instance.position.y = selected_instance.position.y - outline_size;
        instance.position.z = selected_instance.position.z - 0.01f;
        instance.texture_index = 0;
        instance.apply_camera_effects = true;
        instance.colour = colours::editor_entity_selection_colour;
    }

    gouda::InstanceData instance;
};

struct EditorSceneTab {
    // TODO: Add close button
    explicit EditorSceneTab(StringView scene_name) : scene_name(scene_name)
    {
        // TODO: set instance data.
    }

    void Update(const f32 delta_time)
    {
        // TODO: Update based on mouse position.
    }

    void Draw(gouda::vk::Renderer &renderer, std::vector<gouda::InstanceData> &quad_instances, std::vector<gouda::TextData> &text_instances)
    {
        quad_instances.emplace_back(instance);
        // TODO: Calc position with padding etc...
        renderer.DrawText(scene_name, {}, colours::editor_panel_primary_font_colour, 20.0f, 1, text_instances);
    }

    gouda::InstanceData instance;
    String scene_name;
};

struct EditorSceneTabBar {
    explicit EditorSceneTabBar(SharedContext &shared_context) : shared_context{shared_context}, selected_tab{nullptr} {}

    void Update(const f32 delta_time)
    {
        for (auto &tab : tabs) {
            tab.Update(delta_time);
        }
    }

    void Draw(std::vector<gouda::InstanceData> &quad_instances, std::vector<gouda::TextData> &text_instances)
    {
        // Draw the bar/panel itself
        quad_instances.emplace_back(instance);

        // Draw each tab and its text
        for (auto &tab : tabs) {
            tab.Draw(*shared_context.renderer, quad_instances, text_instances);
        }
    }

    void AddTab(const EditorSceneTab &new_tab) { tabs.push_back(new_tab); }

    void RemoveTab()
    {

    }

    void ClearTabs()
    {
        tabs.clear();
    }

    SharedContext &shared_context;
    gouda::InstanceData instance;

    gouda::Vector<EditorSceneTab> tabs;
    EditorSceneTab *selected_tab;
};

class EditorState final : public State {
public:
    explicit EditorState(SharedContext &context, StateStack &state_stack);

    void HandleInput() override;
    void Update(f32 delta_time) override;
    void Render(f32 delta_time) override;
    void OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size) override;
    [[nodiscard]] bool IsOpaque() const override { return false; }

    void OnEnter() override;
    void OnExit() override;

    void LoadScene(StringView scene_file_path);
    void SaveScene(StringView scene_file_path);

private:
    struct EditorScene {
        explicit EditorScene(StringView scene_file_path)
            : scene_file_path{scene_file_path}, selected_entity{nullptr}, scene_changed{false}
        {
        }

        void AddEntity(const Entity &entity)
        {
            editor_entities.emplace_back(entity);
            scene_changed = true;
        }

        [[nodiscard]] String GetSceneName() const { return gouda::fs::GetFileName(scene_file_path); }

        String scene_file_path;
        gouda::Vector<Entity> editor_entities;
        Entity *selected_entity; // TODO: Change this to a size_t index into the editor entities.
        bool scene_changed;
    };

private:
    void DrawEntityPopup();
    void DrawExitConfirmationPopup();

    [[nodiscard]] Entity *PickTopEntityAt(const gouda::Vec2 &mouse_position) const;
    void AddEntity(const Entity &entity);
    void ToggleSelectedEntityPopups();
    void RequestExit();
    void ToggleExitRequested();
    void ClearInstances();

private:
    std::vector<gouda::InstanceData> m_quad_instances;
    std::vector<gouda::TextData> m_text_instances;
    std::vector<gouda::ParticleData> m_particles_instances;

    EditorScene *p_current_scene;
    gouda::Vector<EditorScene> m_editor_scenes;

    TopPanel m_top_menu;
    SidePanel m_side_panel;
    DebugPanel m_debug_panel;

    UIManager m_ui_manager;

    bool m_auto_save;
    bool m_scene_modified; // TODO: Move scene modified to EditorScene struct.
    bool m_exit_requested;
    bool m_show_entity_popups;
};
