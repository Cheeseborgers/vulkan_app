#include "application.hpp"

const WindowSize window_size{800, 800};
constexpr std::string_view application_title{"Gouda Renderer"};

int main()
{
    Application app{window_size};
    app.Init(application_title);
    app.Execute();
}
