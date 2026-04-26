#CMake and C++ version
cmake_minimum_required(VERSION 3.31)
project(rostam-media VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 26)

# setup tgui
set(TGUI_BUILD_GUI_BUILDER OFF)
set(TGUI_BACKEND GLFW_OPENGL3)
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare( #TODO: Switch from nightly to stable
    TGUI
    URL "https://github.com/texus/TGUI/archive/refs/tags/v1.12.0.tar.gz"
)
FetchContent_MakeAvailable(TGUI)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resource/dark.txt  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(src/version.cppm.in ${CMAKE_CURRENT_BINARY_DIR}/version.cppm)
# setup portable file dialogs (single-header)
include_directories(deps/portable_file_dialogs)

# setup GUI and core modules, C++26 hardened standard and -Wall.
message("🐦‍🔥 Setting up Rostam.")
file(GLOB src_files src/*.cppm)
file(GLOB costom_widgets src/costom_widgets/*.cppm)

if(WIN32)
add_definitions(-D_WIN32_WINNT=0x0601) # Windows 7 compatibility
endif()
add_executable(${PROJECT_NAME} WIN32)
target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)
target_sources(${PROJECT_NAME} PRIVATE FILE_SET rostam_modules TYPE CXX_MODULES FILES ${src_files})
target_sources(${PROJECT_NAME} PRIVATE FILE_SET rostam_core_module TYPE CXX_MODULES FILES src/core/rostam.cpp)
target_sources(${PROJECT_NAME} PRIVATE FILE_SET costom_widgets TYPE CXX_MODULES FILES ${costom_widgets})
target_sources(${PROJECT_NAME} PRIVATE FILE_SET version_module TYPE CXX_MODULES FILES ${CMAKE_CURRENT_BINARY_DIR}/version.cppm)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -fhardened -fmodules)

# TODO: Maybe update the compiler and remove the workarounds
if(WIN32) # compiler workarounds for windows
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -fmodules)
target_link_options(rostam-media PRIVATE -Wl,--allow-multiple-definition) # workaround for weired std::format redefinition errors on windows
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/resource/rostam_windows_icon.o") # icon for windows 
target_link_libraries(${PROJECT_NAME} PRIVATE -lstdc++exp "${CMAKE_CURRENT_SOURCE_DIR}/resource/rostam_windows_icon.o") # workaround for undefined reference for std::write_to_terminal...
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE TGUI::TGUI)
# target_link_options(${PROJECT_NAME} PUBLIC -static-libstdc++)

###### INSTALLATION PROCESS ######
include (GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/dark.txt DESTINATION ${CMAKE_INSTALL_BINDIR})
# TODO: loop for installing icons
if(UNIX)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/icons/256x256/apps/rostam-media.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/desktop/rostam-media.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/)
endif()
###### PACKAGING ######
set(CPACK_PACKAGE_EXECUTABLES rostam-media;RostamMedia)
set(CPACK_PACKAGE_VENDOR "-")
set(CPACK_NSIS_IGNORE_LICENSE_PAGE TRUE)
include (CPack)
