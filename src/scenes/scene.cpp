#include "scenes/scene.hpp"

#include <fstream>
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "containers/small_vector.hpp"
#include "core/types.hpp"
#include "debug/logger.hpp"
#include "math/collision.hpp"
#include "math/vector.hpp"

struct GridRange {
    int min_x;
    int max_x;
    int min_y;
    int max_y;
};

GridRange GetGridRange(const Rect<f32> &bounds, const f32 cell_size)
{
    return {(gouda::math::floor(bounds.left / cell_size)), (gouda::math::floor(bounds.right / cell_size)),
            (gouda::math::floor(bounds.bottom / cell_size)), (gouda::math::floor(bounds.top / cell_size))};
}

bool IsInFrustum(const gouda::Vec3 &position, const gouda::Vec2 &size,
                 const gouda::OrthographicCamera::FrustumData &frustum)
{
    const gouda::math::AABB2D frustum_bounds{{frustum.left + frustum.position.x, frustum.top + frustum.position.y},
                                             {frustum.right + frustum.position.x, frustum.bottom + frustum.position.y}};
    const gouda::math::AABB2D object_bounds{{position.x, position.y}, {position.x + size.x, position.y + size.y}};

    return object_bounds.Intersects(frustum_bounds);
}
// Scene ---------------------------------------------------------------------------------------
Scene::Scene(gouda::OrthographicCamera *scene_camera, gouda::OrthographicCamera *ui_camera, gouda::vk::TextureManager *texture_manager)
    : p_scene_camera{scene_camera},
      p_ui_camera{ui_camera},
      p_texture_manager{texture_manager},
      m_player{gouda::InstanceData{}, {0.0f}, 0.0f},
      m_instances_dirty{true},
      m_font_id{1}
{
    ////*
    const gouda::Vector<gouda::InstanceData> instances = {
    {{2945.3f, 3048.8f, -0.522f}, {81.9f, 453.95f}, 0.0f, 4},
    {{200.3f, 200.8f, -0.587f}, {281.9f, 453.95f}, 0.0f, 4},
    {{1182.08f, 69.08f, -0.507f}, {188.0f, 101.51f}, 0.0f, 2},
    {{176.96f, 45.58f, -0.503f}, {246.26f, 227.67f}, 0.0f, 4},
    {{1167.22f, 727.54f, -0.552f}, {197.87f, 315.93f}, 0.0f, 2},
    {{-640.7f, -591.73f, -0.524f}, {145.63f, 238.34f}, 0.0f, 4},
    {{1087.48f, -242.59f, -0.536f}, {237.0f, 388.18f}, 0.0f, 2},
    {{213.46f, 900.51f, -0.585f}, {129.06f, 259.01f}, 0.0f, 1},
    {{-715.56f, -46.76f, -0.556f}, {257.86f, 227.7f}, 0.0f, 1},
    {{-218.47f, 777.39f, -0.543f}, {269.42f, 481.1f}, 0.0f, 1},
    {{630.22f, 517.14f, -0.509f}, {61.04f, 472.29f}, 0.0f, 2},
    {{919.43f, 1134.58f, -0.577f}, {243.88f, 165.4f}, 0.0f, 4},
    {{-262.03f, -736.05f, -0.519f}, {195.07f, 189.2f}, 0.0f, 3},
    {{392.01f, -153.14f, -0.581f}, {133.27f, 125.28f}, 0.0f, 2},
    {{-589.58f, 970.76f, -0.584f}, {155.69f, 340.97f}, 0.0f, 4},
    {{709.94f, 1156.22f, -0.526f}, {55.83f, 166.83f}, 0.0f, 1},
    {{1043.79f, -608.35f, -0.567f}, {130.38f, 148.01f}, 0.0f, 4},
    {{-768.78f, 502.0f, -0.581f}, {227.02f, 378.66f}, 0.0f, 0},
    {{955.42f, 821.25f, -0.517f}, {276.74f, 197.19f}, 0.0f, 1},
    {{-477.7f, -189.79f, -0.544f}, {261.06f, 299.23f}, 0.0f, 2},
    {{-527.9f, 332.5f, -0.532f}, {221.28f, 176.09f}, 0.0f, 4},
    {{393.4f, 76.7f, -0.514f}, {264.59f, 417.81f}, 0.0f, 3},
    {{-345.7f, 83.45f, -0.589f}, {116.81f, 109.75f}, 0.0f, 1},
    {{-133.66f, 235.38f, -0.521f}, {267.79f, 363.2f}, 0.0f, 2},
    {{149.24f, 692.7f, -0.513f}, {229.17f, 114.26f}, 0.0f, 1},
    {{912.76f, 318.42f, -0.585f}, {153.9f, 398.73f}, 0.0f, 4},
    {{571.88f, 959.26f, -0.536f}, {249.71f, 245.45f}, 0.0f, 3},
    {{-118.05f, -96.84f, -0.537f}, {161.1f, 249.32f}, 0.0f, 1},
    {{221.67f, -547.5f, -0.588f}, {154.59f, 326.03f}, 0.0f, 4},
    {{655.83f, -687.51f, -0.515f}, {94.62f, 227.87f}, 0.0f, 4},
    {{524.87f, 697.39f, -0.525f}, {227.79f, 404.37f}, 0.0f, 3},
    {{-791.78f, -253.62f, -0.573f}, {242.24f, 382.73f}, 0.0f, 1},
    {{607.72f, 137.36f, -0.517f}, {119.1f, 460.24f}, 0.0f, 3},
    {{-687.95f, 179.47f, -0.518f}, {176.91f, 149.15f}, 0.0f, 1},
    {{1059.01f, 499.87f, -0.582f}, {182.49f, 467.9f}, 0.0f, 4},
    {{-221.81f, -451.19f, -0.509f}, {63.57f, 330.27f}, 0.0f, 4},
    {{851.02f, -114.47f, -0.577f}, {156.75f, 302.73f}, 0.0f, 0},
    {{155.89f, 1095.48f, -0.588f}, {153.22f, 242.06f}, 0.0f, 4},
    {{-491.75f, 684.49f, -0.511f}, {76.05f, 115.27f}, 0.0f, 4},
    {{-334.72f, 985.35f, -0.569f}, {57.97f, 190.37f}, 0.0f, 4},
    {{-45.29f, 888.62f, -0.589f}, {203.9f, 229.43f}, 0.0f, 2},
    {{403.64f, -736.12f, -0.537f}, {255.69f, 431.18f}, 0.0f, 3},
    {{-796.82f, 1008.09f, -0.555f}, {218.18f, 311.46f}, 0.0f, 1},
    {{-458.31f, -401.66f, -0.528f}, {51.26f, 153.7f}, 0.0f, 0},
    {{-53.42f, -759.58f, -0.552f}, {228.56f, 340.13f}, 0.0f, 4},
    {{921.27f, -436.21f, -0.529f}, {278.43f, 334.79f}, 0.0f, 3},
    {{-310.8f, 542.54f, -0.516f}, {63.66f, 233.54f}, 0.0f, 4},
    {{775.4f, 719.19f, -0.558f}, {184.31f, 101.12f}, 0.0f, 4},
    {{52.4f, -407.07f, -0.582f}, {147.11f, 427.64f}, 0.0f, 4},
    {{593.53f, -123.78f, -0.503f}, {181.65f, 345.05f}, 0.0f, 4},
    {{488.41f, 1187.64f, -0.569f}, {142.01f, 339.15f}, 0.0f, 4},
    {{710.36f, -330.24f, -0.504f}, {88.82f, 401.39f}, 0.0f, 2},
};


    m_entities.reserve(instances.size());
    for (const auto &instance : instances) {
        m_entities.emplace_back(instance, EntityType::Quad);
    }

    ///*/

    //const gouda::Vector<gouda::InstanceData> instances;

    SetupPlayer();
    SetupUI();

    m_visible_quad_instances.reserve(instances.size() + 1);
    BuildSpatialGrid();

    m_particles_instances.reserve(1024); // Reserve space for particles
}

void Scene::Update(const f32 delta_time)
{
    p_scene_camera->Update(delta_time);
    p_ui_camera->Update(delta_time);

    UpdateParticles(delta_time);

    // Early exit for no movement
    if (m_player.velocity.x == 0 && m_player.velocity.y == 0) {
        UpdateVisibleInstances();
        UpdateUI(delta_time);
        return;
    }

    // Calculate new position
    gouda::Vec3 new_position{};
    new_position.x = m_player.render_data.position.x + m_player.velocity.x * delta_time;
    new_position.y = m_player.render_data.position.y + m_player.velocity.y * delta_time,
    new_position.z = m_player.render_data.position.z;

    // Player collision bounds
    const Rect<f32> player_bounds{new_position.x, new_position.x + m_player.render_data.size.x, new_position.y,
                                  new_position.y + m_player.render_data.size.y};

    // Spatial grid query for collision
    static constexpr f32 cell_size{500.0f};
    auto [min_x, max_x, min_y, max_y] = GetGridRange(player_bounds, cell_size);

    static std::unordered_set<size_t> nearby_entities;
    nearby_entities.clear();
    for (int x = min_x; x <= max_x; ++x) {
        for (int y = min_y; y <= max_y; ++y) {
            if (auto it = m_spatial_grid.find(GridPos{x, y}); it != m_spatial_grid.end()) {
                nearby_entities.insert(it->second.begin(), it->second.end());
            }
        }
    }

    // Collision detection and resolution
    for (const size_t entity_idx : nearby_entities) {
        auto &entity = m_entities[entity_idx];
        if (const gouda::Vec2 & entity_size{entity.render_data.size};
            check_collision(new_position, m_player.render_data.size, entity.render_data.position, entity_size)) {

            const Rect<f32> entity_bounds{entity.render_data.position.x, entity.render_data.position.x + entity_size.x,
                                          entity.render_data.position.y, entity.render_data.position.y + entity_size.y};

            const Rect<f32> penetration_bounds{
                player_bounds.right - entity_bounds.left, entity_bounds.right - player_bounds.left,
                player_bounds.top - entity_bounds.bottom, entity_bounds.top - player_bounds.bottom};

            f32 min_penetration{constants::f32_max};
            enum class Direction : u8 { None, Left, Right, Bottom, Top } resolve_dir = Direction::None;

            if (penetration_bounds.left > 0 && m_player.velocity.x > 0) {
                if (penetration_bounds.left < min_penetration) {
                    min_penetration = penetration_bounds.left;
                    resolve_dir = Direction::Left;
                }
            }
            if (penetration_bounds.right > 0 && m_player.velocity.x < 0) {
                if (penetration_bounds.right < min_penetration) {
                    min_penetration = penetration_bounds.right;
                    resolve_dir = Direction::Right;
                }
            }
            if (penetration_bounds.bottom > 0 && m_player.velocity.y > 0) {
                if (penetration_bounds.bottom < min_penetration) {
                    min_penetration = penetration_bounds.bottom;
                    resolve_dir = Direction::Bottom;
                }
            }
            if (penetration_bounds.top > 0 && m_player.velocity.y < 0) {
                if (penetration_bounds.top < min_penetration) {
                    min_penetration = penetration_bounds.top;
                    resolve_dir = Direction::Top;
                }
            }

            switch (resolve_dir) {
                case Direction::Left:
                    new_position.x = entity_bounds.left - m_player.render_data.size.x - 0.001f;
                    m_player.velocity.x = 0;
                    break;
                case Direction::Right:
                    new_position.x = entity_bounds.right + 0.001f;
                    m_player.velocity.x = 0;
                    break;
                case Direction::Bottom:
                    new_position.y = entity_bounds.bottom - m_player.render_data.size.y - 0.001f;
                    m_player.velocity.y = 0;
                    break;
                case Direction::Top:
                    new_position.y = entity_bounds.top + 0.001f;
                    m_player.velocity.y = 0;
                    break;
                case Direction::None:
                    break;
            }
            break; // Early exit after first collision
        }
    }

    m_player.render_data.position = new_position;

    // Update visible instances without spatial grid
    UpdateVisibleInstances();
    UpdateUI(delta_time);
}

void Scene::Render(const f32 delta_time, gouda::vk::Renderer &renderer, gouda::UniformData &uniform_data)
{
    DrawUI(renderer);

    uniform_data.wvp = p_scene_camera->GetViewProjectionMatrix();
    uniform_data.wvp_static = p_ui_camera->GetViewProjectionMatrix();
    renderer.Render(delta_time, uniform_data, m_visible_quad_instances, m_text_instances, m_particles_instances);
}
void Scene::UpdateUI([[maybe_unused]]const f32 delta_time)
{
    for (auto element : m_ui_elements) {
        m_visible_quad_instances.push_back(element);
    }
}

void Scene::DrawUI(gouda::vk::Renderer &renderer)
{
    m_text_instances.clear();

    renderer.DrawText("GOUDA RENDERER", {100.0f, 100.0f, -0.8}, {0.0f, 1.0f, 0.0f, 1.0f}, 20.0f, m_font_id,
                      m_text_instances, gouda::TextAlign::Center, true);


    renderer.DrawText("GOUDA RENDERER", {200.0f, 200.0f, -0.8}, {0.0f, 1.0f, 0.0f, 1.0f}, 50.0f, 2,
                  m_text_instances);
}

void Scene::LoadFromJSON(std::string_view filepath)
{
    m_instances_dirty = true; // Trigger grid rebuild
    BuildSpatialGrid();
}

void Scene::SaveToJSON(std::string_view filepath) {}

void Scene::SpawnParticle(const gouda::Vec3 &position, const gouda::Vec2 &size, const gouda::Vec3 &velocity,
                          const f32 lifetime, const u32 texture_index, const gouda::Vec4 &colour)
{
    if (size.x <= 0.0f || size.y <= 0.0f || lifetime <= 0.0f) {
        APP_LOG_WARNING("Invalid particle parameters: size=[{}, {}], lifetime={}", size.x, size.y, lifetime);
        return;
    }

    m_particles_instances.emplace_back(position, size, lifetime, velocity, colour, texture_index);
}

// Private ---------------------------------------------------------------------------------
void Scene::SetupEntities() {}

void Scene::SetupPlayer()
{
    // TODO: Fix the player position in regards to the camera viewport and check collision is not affected by the z axis
    // yet?????
    m_player.render_data.position = {500.0f, 500.0f, -0.9f};
    m_player.render_data.size = {32.0f, 32.0f};
    m_player.render_data.texture_index = 5;
    m_player.render_data.colour = {1.0f, 1.0f, 1.0f, 0.0f};
    m_player.velocity = {0.0f};
    m_player.speed = 200.0f;
    m_player.render_data.is_atlas = true;

    const auto sprite = p_texture_manager->GetSprite(5, "player.walk");
    const auto frame = sprite->frames.at(1);

    // Currently hardcoded
    m_player.render_data.sprite_rect =
        UVRect{frame.uv_rect.u_min, frame.uv_rect.v_min, frame.uv_rect.u_max, frame.uv_rect.v_max};

    // Add animation component to player
    m_player.animation_component = AnimationComponent{};
    m_player.animation_component->current_animation = "idle";
}
void Scene::SetupUI()
{
    gouda::InstanceData debug{};
    debug.position = {10.f, 800.f, -1.0f};
    debug.size = {100.0f, 100.0f};
    debug.texture_index = 0;
    debug.colour = {0.3f, 0.6f, 0.4f, 1.0f};
    debug.apply_camera_effects = 0;
    m_ui_elements.emplace_back(debug);

    gouda::InstanceData menu{};
    menu.position = {100.f, 100.f, -1.0f};
    menu.size = {500.0f, 500.0f};
    menu.texture_index = 0;
    menu.colour = {0.3f, 0.3f, 0.3f, 1.0f};
    menu.apply_camera_effects = 0;
    m_ui_elements.push_back(menu);
}

void Scene::BuildSpatialGrid()
{
    m_spatial_grid.clear();
    static constexpr f32 cell_size{500.0f};

    for (size_t i = 0; i < m_entities.size(); ++i) {
        const auto &entity = m_entities[i];
        Rect bounds{entity.render_data.position.x, entity.render_data.position.x + entity.render_data.size.x,
                    entity.render_data.position.y, entity.render_data.position.y + entity.render_data.size.y};

        auto [min_x, max_x, min_y, max_y] = GetGridRange(bounds, cell_size);
        for (int x = min_x; x <= max_x; ++x) {
            for (int y = min_y; y <= max_y; ++y) {
                m_spatial_grid[GridPos{x, y}].push_back(i);
            }
        }
    }
}

void Scene::UpdateVisibleInstances()
{
    m_visible_quad_instances.clear();
    const auto &frustum = p_scene_camera->GetFrustumData();
    for (const auto &entity : m_entities) {
        if (IsInFrustum(entity.render_data.position, entity.render_data.size, frustum)) {
            m_visible_quad_instances.push_back(entity.render_data);
        }
    }
    if (IsInFrustum(m_player.render_data.position, m_player.render_data.size, frustum)) {
        m_visible_quad_instances.push_back(m_player.render_data);
    }
}

void Scene::UpdateParticles(const f32 delta_time)
{
    for (auto it = m_particles_instances.begin(); it != m_particles_instances.end();) {
        it->velocity += gouda::Vec3{0.0f, constants::gravity, 0.0f} * delta_time;
        it->position += it->velocity * delta_time;
        it->lifetime -= delta_time;
        if (it->lifetime <= 0.0f) {
            it = m_particles_instances.erase(it);
        }
        else {
            it->colour.w = it->lifetime / 5.0f; // Fade over 5s
            ++it;
        }
    }
}
