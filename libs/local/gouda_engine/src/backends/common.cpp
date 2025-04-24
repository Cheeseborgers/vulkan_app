#include "backends/common.hpp"

namespace gouda {

std::string platform_to_string(Platform platform)
{
    switch (platform) {
        case Platform::X11:
            return internal::constants::X11;
        case Platform::Wayland:
            return internal::constants::Wayland;
        case Platform::Windows:
            return internal::constants::Windows;
        case Platform::Headless:
            return internal::constants::Headless;
        case Platform::SystemDefault:
            return internal::constants::SystemDefault;
        default:
            return internal::constants::Unknown; // We should never reach this
    }
}

} // namespace gouda end