#include "backends/common.hpp"

namespace Gouda {

std::string platform_to_string(Platform platform)
{
    switch (platform) {
        case Platform::X11:
            return Internal::Constants::X11;
        case Platform::Wayland:
            return Internal::Constants::Wayland;
        case Platform::Windows:
            return Internal::Constants::Windows;
        case Platform::Headless:
            return Internal::Constants::Headless;
        case Platform::SystemDefault:
            return Internal::Constants::SystemDefault;
        default:
            return Internal::Constants::Unknown; // We should never reach this
    }
}

} // namespace Gouda end