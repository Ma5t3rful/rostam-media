#include <GLFW/glfw3.h>
#include <TGUI/TGUI.hpp>
#include <filesystem>
#include <optional>
#include <print>
import master_window;
import cross_platform;

auto main (int argc, char** argv) -> int
{
    std::println("Initializing Rostam...");
    glfwSetErrorCallback([](const auto err_code,const auto desc)
    {
        std::println("\033[31m[ROSTAM GLFW ERROR]\033[0m glfw error detected: {} - {}", err_code, desc);
    });
    glfwInit();
    auto* window = glfwCreateWindow(1000, 500, "Rostam Media", nullptr, nullptr);
    glfwSetWindowSizeLimits(window, 280, 250, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    
    if(const auto dark_theme = cross_platform::find_file("dark.txt");dark_theme)tgui::Theme::setDefault(dark_theme->c_str());
    else std::println("Falied to find the theme file. looks might be off!");

    {// scope for gui to be destroyed before calling terminate
        MainWindow gui(window);
        gui.mainLoop("#211f1f");
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}