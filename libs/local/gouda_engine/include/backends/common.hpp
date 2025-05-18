#pragma once

#include <string>

#include "core/types.hpp"

namespace gouda {
namespace internal::constants {

static constexpr auto X11 = "X11";
static constexpr auto Wayland = "Wayland";
static constexpr auto Windows = "Windows";
static constexpr auto MacOS = "MacOS";
static constexpr auto Headless = "Headless";
static constexpr auto SystemDefault = "System default";
static constexpr auto Unknown = "Unknown";

}

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
        : resizable{true},
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