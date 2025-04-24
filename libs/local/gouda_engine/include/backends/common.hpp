#pragma once

#include <string>

#include "core/types.hpp"

namespace gouda {
namespace internal {
namespace constants {

static constexpr const char *X11 = "X11";
static constexpr const char *Wayland = "Wayland";
static constexpr const char *Windows = "Windows";
static constexpr const char *MacOS = "MacOS";
static constexpr const char *Headless = "Headless";
static constexpr const char *SystemDefault = "System default";
static constexpr const char *Unknown = "Unknown";

} // namespace constants end
} // namespace internal end

enum class Platform : u8 { X11, Wayland, Windows, Headless, SystemDefault };
enum class Renderer : u8 { Vulkan, OpenGL };

struct WindowConfig {
    WindowSize size;
    std::string_view title;
    bool resizable;
    bool fullscreen;
    bool vsync;
    int refresh_rate;
    Renderer renderer;
    Platform platform;

    WindowConfig()
        : title{""},
          resizable{true},
          fullscreen{false},
          vsync{false},
          refresh_rate{60},
          renderer{Renderer::Vulkan},
          platform{Platform::X11}
    {
    }
};

std::string platform_to_string(Platform platform);

} // namespace gouda end