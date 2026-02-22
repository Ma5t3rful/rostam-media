#CMake and c++ version
cmake_minimum_required(VERSION 3.31)
project(rostam-media VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 26)

# setup tgui
set(TGUI_BUILD_GUI_BUILDER OFF)
set(TGUI_BACKEND GLFW_OPENGL3)
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare( #TODO: Switch from nightly to stable
    TGUI
    URL "https://github.com/texus/TGUI/archive/refs/tags/nightly_build.tar.gz"
)
FetchContent_MakeAvailable(TGUI)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resource/dark.txt  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

#setup portable file dialogs (single-header)
include_directories(deps/portable_file_dialogs)

# setup GUI and core modules, C++26 hardened standard and -Wall.
message("🐦‍🔥 Setting up Rostam.")
file(GLOB src_files src/*.cppm)
file(GLOB costom_widgets src/costom_widgets/*.cppm)
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)
target_sources(${PROJECT_NAME} PRIVATE FILE_SET rostam_modules TYPE CXX_MODULES FILES ${src_files})
target_sources(${PROJECT_NAME} PRIVATE FILE_SET rostam_core_module TYPE CXX_MODULES FILES src/core/rostam.cpp)
target_sources(${PROJECT_NAME} PRIVATE FILE_SET costom_widgets TYPE CXX_MODULES FILES ${costom_widgets})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -fhardened -fmodules)
target_link_libraries(${PROJECT_NAME} TGUI::TGUI)
target_link_options(${PROJECT_NAME} PUBLIC -static-libstdc++)

###### INSTALLATION PROCESS ######
include (GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
# TODO loop for installing icons
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/icons/256x256/apps/rostam-media.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/desktop/rostam-media.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/dark.txt DESTINATION ${CMAKE_INSTALL_BINDIR})
