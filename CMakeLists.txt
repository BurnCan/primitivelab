cmake_minimum_required(VERSION 3.5)
project(LearnOpenGLGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# -------------------
# Fetch dependencies
# -------------------
# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# stb_image
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# ImGui (for editor only) — docking branch
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# -------------------
# Common include directories
# -------------------
set(COMMON_INCLUDE
    src
    ${CMAKE_SOURCE_DIR}/include
    ${glfw_SOURCE_DIR}/include
    ${glm_SOURCE_DIR}
    ${stb_SOURCE_DIR}
)

# -------------------
# Common sources (shared by both Game and Editor)
# -------------------
set(COMMON_SOURCES
    src/external/glad.c
    src/Primitives.cpp
    src/SceneManager.cpp
    src/Terrain.cpp
    src/Camera.cpp
    src/stb_image_impl.cpp
)

# -------------------
# ImGui sources (editor only)
# -------------------
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# -------------------
# Game executable
# -------------------
add_executable(Game
    src/main.cpp
    src/ShaderUtils.cpp
    ${COMMON_SOURCES}
)

target_include_directories(Game PRIVATE ${COMMON_INCLUDE})
target_link_libraries(Game PRIVATE glfw)

# -------------------
# Editor executable
# -------------------
add_executable(Editor
    src/editor.cpp
    src/ShaderUtils.cpp
    src/external/tinyfiledialogs.c
    ${COMMON_SOURCES}
    ${IMGUI_SOURCES}
)

target_include_directories(Editor PRIVATE ${COMMON_INCLUDE} ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(Editor PRIVATE glfw)

# -------------------
# Platform-specific libraries
# -------------------

# Linux/Unix dependencies (excluding Apple)
if(UNIX AND NOT APPLE)
    target_link_libraries(Game PRIVATE dl pthread X11 Xrandr Xi Xxf86vm Xinerama Xcursor)
    target_link_libraries(Editor PRIVATE dl pthread X11 Xrandr Xi Xxf86vm Xinerama Xcursor)
endif()

# macOS/Apple dependencies
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(Game PRIVATE ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
    target_link_libraries(Editor PRIVATE ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
endif()

# Windows dependencies
if(WIN32)
    # Required libraries for OpenGL/window management on Windows
    target_link_libraries(Game PRIVATE kernel32 user32 gdi32 shell32 opengl32)
    target_link_libraries(Editor PRIVATE kernel32 user32 gdi32 shell32 opengl32)
endif()