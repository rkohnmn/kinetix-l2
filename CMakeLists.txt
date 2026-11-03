cmake_minimum_required(VERSION 3.20)
project(kinetix-l2
    VERSION 3.0.0
    DESCRIPTION "Kinetix L2 — Native C++20 4-Modality SLA Focus Studio (librsvg + Frosted-Glass Shader + Photon + Dear ImGui + PangoCairo)"
    LANGUAGES CXX
)

# ────────────────────────────────────────────────────────────────
# Global Settings (`buildcpp.md` Modern CMake Patterns)
# ────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(KINETIX_BUILD_TESTS "Build Kinetix L2 automated test suite" ON)
option(KINETIX_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(KINETIX_ENABLE_SANITIZERS "Enable ASan/UBSan in Debug builds" OFF)

# ────────────────────────────────────────────────────────────────
# Compiler Warnings & Sanitizers Interface Libraries (`buildcpp.md`)
# ────────────────────────────────────────────────────────────────
add_library(project_warnings INTERFACE)
if(KINETIX_ENABLE_WARNINGS)
    target_compile_options(project_warnings INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang>:
            -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wcast-align
        >
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4 /permissive-
        >
    )
endif()

add_library(project_sanitizers INTERFACE)
if(KINETIX_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(project_sanitizers INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(project_sanitizers INTERFACE -fsanitize=address,undefined)
endif()

# ────────────────────────────────────────────────────────────────
# Target-Based Core Library
# ────────────────────────────────────────────────────────────────
set(KINETIX_CORE_SOURCES
    src/domain/fsrs_engine.cpp
    src/domain/knowledge_graph.cpp
    src/domain/pedagogical_loop.cpp
    src/domain/habit_engine.cpp
    src/persistence/database_manager.cpp
    src/platform/platform_adapter.cpp
    src/ui/vector_canvas.cpp
    src/ui/main_window.cpp
)

add_library(kinetix_core STATIC ${KINETIX_CORE_SOURCES})
target_include_directories(kinetix_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(kinetix_core
    PRIVATE
        project_warnings
        project_sanitizers
)

if(WIN32)
    target_compile_definitions(kinetix_core PUBLIC PLATFORM_WINDOWS SDL_MAIN_HANDLED)
    if(MSVC)
        message(FATAL_ERROR "Windows builds require MSYS2 UCRT64 and its native SDL2/Cairo/Pango/librsvg packages; see README.md")
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NATIVE REQUIRED IMPORTED_TARGET sqlite3 cairo pangocairo librsvg-2.0)
    # SDL's pkg-config flags add SDL2main and -mwindows. We own our console
    # entry points and explicitly call SDL_SetMainReady instead.
    find_package(SDL2 CONFIG REQUIRED)
    target_link_libraries(kinetix_core PUBLIC PkgConfig::NATIVE SDL2::SDL2)
elseif(APPLE)
    target_compile_definitions(kinetix_core PUBLIC PLATFORM_MACOS)
    target_link_libraries(kinetix_core PUBLIC "-framework Cocoa" "-framework Metal" "-framework CoreAudio" "-framework AudioToolbox")
elseif(ANDROID)
    target_compile_definitions(kinetix_core PUBLIC PLATFORM_ANDROID)
    target_link_libraries(kinetix_core PUBLIC android log GLESv2 OpenSLES)
else()
    target_compile_definitions(kinetix_core PUBLIC PLATFORM_LINUX)
endif()

if(APPLE OR (UNIX AND NOT ANDROID))
    find_library(SQLITE3_LIB NAMES sqlite3 libsqlite3.so.0 PATHS /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu /opt/homebrew/lib /usr/local/lib /usr/lib)
    find_library(CAIRO_LIB NAMES cairo libcairo.so.2 PATHS /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu /opt/homebrew/lib /usr/local/lib /usr/lib)
    find_library(RSVG_LIB NAMES rsvg-2 librsvg-2.so.2 PATHS /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu /opt/homebrew/lib /usr/local/lib /usr/lib)
    find_library(PANGO_LIB NAMES pango-1.0 libpango-1.0.so.0 PATHS /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu /opt/homebrew/lib /usr/local/lib /usr/lib)
    find_library(PANGOCAIRO_LIB NAMES pangocairo-1.0 libpangocairo-1.0.so.0 PATHS /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu /opt/homebrew/lib /usr/local/lib /usr/lib)
    find_library(GOBJECT_LIB NAMES gobject-2.0 libgobject-2.0.so.0 PATHS /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu /opt/homebrew/lib /usr/local/lib /usr/lib)
    find_library(SDL2_LIB NAMES SDL2 SDL2-2.0 libSDL2-2.0.so.0 PATHS /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu /opt/homebrew/lib /usr/local/lib /usr/lib)

    target_link_libraries(kinetix_core
        PUBLIC
            ${SQLITE3_LIB}
            ${CAIRO_LIB}
            ${RSVG_LIB}
            ${PANGO_LIB}
            ${PANGOCAIRO_LIB}
            ${GOBJECT_LIB}
            ${SDL2_LIB}
    )
endif()

add_executable(kinetix-l2 src/main.cpp)
target_link_libraries(kinetix-l2 PRIVATE kinetix_core project_warnings project_sanitizers)

if(KINETIX_BUILD_TESTS)
    enable_testing()
    add_executable(kinetix-tests tests/test_main.cpp)
    target_link_libraries(kinetix-tests PRIVATE kinetix_core project_warnings project_sanitizers)
    add_executable(kinetix-rendering-tests tests/test_rendering.cpp)
    target_link_libraries(kinetix-rendering-tests PRIVATE kinetix_core project_warnings project_sanitizers)
    add_test(NAME NativeRenderingContract COMMAND kinetix-rendering-tests)
    set_tests_properties(NativeRenderingContract PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy" TIMEOUT 30)
    add_test(NAME KinetixUnitAndIntegrationSuite COMMAND kinetix-tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

# Dedicated fixed-fixture samples, separate from correctness smoke limits.
option(BUILD_PERFORMANCE_SAMPLES "Build repeated performance sampling tool" OFF)
if(BUILD_PERFORMANCE_SAMPLES)
    add_executable(kinetix-perf-samples benchmarks/perf_samples.cpp)
    target_link_libraries(kinetix-perf-samples PRIVATE kinetix_core)
    target_compile_definitions(kinetix-perf-samples PRIVATE PERF_BUILD_TYPE="$<CONFIG>" PERF_COMPILER="${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
endif()
