// This module has the facillity
module;
#if __unix
#include <format>
#endif
#include <string>
#include <algorithm>
#include <string_view>
#include <stdexcept>
#include <filesystem>
#include <optional>
#ifdef _WIN32
#include <windows.h>
#endif
export module cross_platform;

export 
namespace cross_platform
{
    auto open_link (const std::string_view link) -> void
    {
        #if __unix__
        const auto xdg_open = std::format("xdg-open \"{}\"",link);
        const auto result   = std::system (xdg_open.c_str());
        if(result != 0) throw std::runtime_error(std::format("xdg-open returned non-zero: {}",result));
        #elif _WIN32 //FIXME: Does not work with utf-8 strings.
        // const auto start_command = std::format("start \"\" \"{}\"",link);
        // const auto _ = std::system(start_command.c_str());
        #endif
    }

    auto exe_path () -> std::filesystem::path
    {
        // This is one of the things that the standard library should add.
        #if __unix__ // DONE
        return std::filesystem::canonical("/proc/self/exe").remove_filename();
        #elif _WIN32 
        wchar_t path_buffer [MAX_PATH * 2];
        GetModuleFileNameW(nullptr, path_buffer, MAX_PATH * 2);
        return std::filesystem::path(path_buffer).remove_filename();
        #endif
    }
    
    //TODO: Needs testing
    auto find_file (const std::string_view filename) -> std::optional<std::filesystem::path>
    {
        if(const auto appdir_env = std::getenv("APPDIR"); appdir_env)
            if(const auto addr = std::filesystem::path(appdir_env)/"share/rostam_theme"/filename; 
            std::filesystem::exists(addr))
                return addr;
        
        const auto possible_paths = std::to_array<std::filesystem::path>({cross_platform::exe_path(),"/usr/share/rostam/","/usr/share/rostam_theme/","/usr/bin",std::filesystem::current_path()});
        if(const auto found_path = std::ranges::find_if(possible_paths,[filename](const auto& p){return std::filesystem::exists(p/filename);}); found_path != possible_paths.end())
            return *found_path/filename;
        
        return {};
    }
}