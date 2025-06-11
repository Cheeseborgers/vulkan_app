#pragma once
/**
 * @file scenes/scene.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-06
 * @brief Application scene module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "cameras/orthographic_camera.hpp"
#include "renderers/vulkan/vk_renderer.hpp"
#include "renderers/vulkan/vk_texture_manager.hpp"

#include "entities/player.hpp"

struct GridPos {
    s32 x;
    s32 y;

    bool operator==(const GridPos &other) const { return x == other.x && y == other.y; }
    struct Hash {
        size_t operator()(const GridPos &pos) const { return std::hash<int>()(pos.x) ^ std::hash<int>()(pos.y) << 1; }
    };
};

class Scene {
public:
    explicit Scene(gouda::OrthographicCamera *scene_camera, gouda::OrthographicCamera *ui_camera, gouda::vk::TextureManager *texture_manager);
    ~Scene() = default;

    void Update(f32 delta_time);
    void Render(f32 delta_time, gouda::vk::Renderer &renderer, gouda::UniformData &uniform_data);

    void UpdateUI(f32 delta_time);
    void DrawUI(gouda::vk::Renderer &renderer);

    void LoadFromJSON(StringView filepath);
    void SaveToJSON(StringView filepath);

    // Particle methods
    void SpawnParticle(const gouda::Vec3 &position, const gouda::Vec2 &size, const gouda::Vec3 &velocity, f32 lifetime,
                       u32 texture_index = 0, const gouda::Vec4 &colour = {1.0f});

    Player &GetPlayer() { return m_player; }

    void SetFontID(const u32 id) { m_font_id = id; }

private:
    void SetupEntities();
    void SetupPlayer();
    void SetupUI();
    void BuildSpatialGrid();
    void UpdateVisibleInstances();
    void UpdateParticles(f32 delta_time);

private:
    gouda::OrthographicCamera *p_scene_camera;
    gouda::OrthographicCamera *p_ui_camera;
    gouda::vk::TextureManager *p_texture_manager;

    Player m_player;
    gouda::Vector<Entity> m_entities;

    std::vector<gouda::InstanceData> m_visible_quad_instances;
    std::vector<gouda::TextData> m_text_instances;
    std::vector<gouda::ParticleData> m_particles_instances;
    bool m_instances_dirty;

    u32 m_font_id;

    std::unordered_map<GridPos, gouda::Vector<size_t>, GridPos::Hash> m_spatial_grid;

    std::vector<gouda::InstanceData> m_ui_elements;
};