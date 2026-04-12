// This module has the facillity
module;
#include <format>
#include <string>
#include <string_view>
#include <stdexcept>
#include <filesystem>
export module cross_platform;

export 
namespace cross_platform
{
    auto open_link (const std::string_view link) -> void
    {
        #ifdef __unix__
        const auto xdg_open = std::format("xdg-open \"{}\"",link);
        const auto result =  std::system (xdg_open.c_str());
        if(result != 0) throw std::runtime_error(std::format("xdg-open returned non-zero: {}",result));
        #elif WIN32 //FIXME: Does not work with utf-8 strings.
        const auto start_command = std::format("start \"\" \"{}\"",link);
        const auto _ = std::system(start_command.c_str());
        #endif
    }

    auto exe_path () -> std::filesystem::path
    {
        // This is one of the things that the standard library should add.
        #ifdef __unix__ //DONE
        return std::filesystem::canonical("/proc/self/exe").remove_filename();
        #elif WIN32     // TODO: implement this
        return "C:/stub";
        #endif
    }
}