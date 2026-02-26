#include <GLFW/glfw3.h>
#include <TGUI/TGUI.hpp>
#include <array>
#include <filesystem>
#include <algorithm>
#include <print>
import master_window;

auto main (int argc, char** argv) -> int
{
    std::println("Initializing Rostam...");
    glfwSetErrorCallback([](const auto err_code,const auto desc)
    {
        std::println("\033[31m[ROSTAM GLFW ERROR]\033[0m glfw error detected: {} - {}", err_code, desc);
    });
    glfwInit();
    auto* window = glfwCreateWindow(1000, 500, "Rostam Media", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    //if(const auto theme = std::getenv("ROSTAM_THEME");theme != nullptr)
    
    const auto possible_theme_paths = std::to_array({(std::filesystem::canonical("/proc/self/exe").remove_filename()/"dark.txt"),std::filesystem::path("/usr/share/rostam_theme/dark.txt")});
    if(const auto found = std::ranges::find_if(possible_theme_paths,[](const auto& p){return std::filesystem::exists(p);});
    found != possible_theme_paths.cend())
        tgui::Theme::setDefault(found->c_str());
    else if(const auto appdir = std::getenv("APPDIR");appdir)
    {
        tgui::Theme::setDefault((appdir/std::filesystem::path("share/rostam_theme/dark.txt")).c_str());
    }
    
    {// scope for gui to be destroyed before calling terminate
        MainWindow gui(window);
        gui.mainLoop("#211f1f");
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}