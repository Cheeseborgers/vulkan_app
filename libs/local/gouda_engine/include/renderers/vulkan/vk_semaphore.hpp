#pragma once
/**
 * @file vk_semaphore.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-24
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <vulkan/vulkan.h>

namespace gouda::vk {

class Device;

class Semaphore {
public:
    explicit Semaphore(Device *device);

    ~Semaphore();

    bool Create();
    void Destroy();

    [[nodiscard]] VkSemaphore Get() const noexcept { return p_semaphore; }

private:
    VkSemaphore p_semaphore;
    Device *p_device;
};

} // namespace gouda::vk