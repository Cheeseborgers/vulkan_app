#include "application.hpp"

int main()
{
    Gouda::WindowConfig window_config{};
    window_config.size = {800, 800};
    window_config.title = "Gouda Renderer";
    window_config.resizable = true;
    window_config.fullscreen = false;
    window_config.vsync = false;
    window_config.refresh_rate = 0;
    window_config.renderer = Gouda::Renderer::Vulkan;
    window_config.platform = Gouda::Platform::X11;

    Application app;
    app.Init(window_config);
    app.Execute();
}
