#pragma once

#include "cameras/orthographic_camera.hpp"
#include "renderers/vulkan/vk_renderer.hpp"

#include "core/player.hpp"

// Simple 2D grid position
struct GridPos {
    int x, y;
    bool operator==(const GridPos &other) const { return x == other.x && y == other.y; }
    struct Hash {
        size_t operator()(const GridPos &pos) const { return std::hash<int>()(pos.x) ^ (std::hash<int>()(pos.y) << 1); }
    };
};

class Scene {
public:
    Scene(gouda::OrthographicCamera *camera_ptr);
    ~Scene() = default;

    void Update(f32 delta_time);
    void Render(f32 delta_time, gouda::vk::Renderer &renderer, gouda::vk::UniformData &uniform_data);

    void DrawUI(gouda::vk::Renderer &renderer);

    void LoadFromJSON(std::string_view filepath);
    void SaveToJSON(std::string_view filepath);

    // Particle methods
    void SpawnParticle(const gouda::Vec3 &position, const gouda::Vec2 &size, const gouda::Vec3 &velocity, f32 lifetime,
                       u32 texture_index = 0, const gouda::Vec4 &colour = {1.0f});

    Player &GetPlayer() { return m_player; }

    void SetFontID(u32 id) { m_font_id = id; }

private:
    void BuildSpatialGrid();
    void UpdateVisibleInstances();
    void UpdateParticles(f32 delta_time);

private:
    gouda::OrthographicCamera *p_camera;

    Player m_player;
    std::vector<Entity> m_entities;

    std::vector<gouda::vk::InstanceData> m_visible_quad_instances;
    std::vector<gouda::vk::TextData> m_text_instances;
    std::vector<gouda::vk::ParticleData> m_particles_instances;
    bool m_instances_dirty;

    u32 m_font_id;

    std::unordered_map<GridPos, std::vector<size_t>, GridPos::Hash> m_spatial_grid;
};