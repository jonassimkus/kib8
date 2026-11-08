cmake_minimum_required(VERSION 3.20)

project(kib8 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)

add_subdirectory(vendor/SDL)

add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/backends/imgui_impl_sdl3.cpp
    vendor/imgui/backends/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(imgui PUBLIC vendor/imgui vendor/imgui/backends)

target_link_libraries(imgui PUBLIC SDL3::SDL3)


file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS src/core/*.cpp)

add_library(chip8core STATIC ${CORE_SOURCES})

target_include_directories(chip8core PUBLIC src)


file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS src/frontend/*.cpp)

add_executable(kib8 ${APP_SOURCES})

target_link_libraries(kib8 PRIVATE chip8core imgui SDL3::SDL3)


# ===== Warnings (only on your code) =====
if(MSVC)
    target_compile_options(chip8core PRIVATE /W4)
    target_compile_options(kib8  PRIVATE /W4)
else()
    target_compile_options(chip8core PRIVATE -Wall -Wextra)
    target_compile_options(kib8  PRIVATE -Wall -Wextra)

endif()