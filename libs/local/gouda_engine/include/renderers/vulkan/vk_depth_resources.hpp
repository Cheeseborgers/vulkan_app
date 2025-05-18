#pragma once
/**
 * @file vk_depth_resources.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-19
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "renderers/vulkan/vk_texture.hpp"
#include "containers/small_vector.hpp"

namespace gouda::vk {

class Swapchain;
class Device;
class Instance;
class BufferManager;

class DepthResources {
public:
    DepthResources(Device *device, Instance *instance, BufferManager *buffer_manager, Swapchain *swapchain);
    ~DepthResources();

    void Create();
    void Recreate();
    void Destroy();

    [[nodiscard]] const Vector<Texture> &GetDepthImages() const { return m_depth_images; }

private:
    Swapchain *p_swapchain;
    Device *p_device;
    Instance *p_instance;
    BufferManager *p_buffer_manager;

    Vector<Texture> m_depth_images;
};

} // namespace gouda::vk