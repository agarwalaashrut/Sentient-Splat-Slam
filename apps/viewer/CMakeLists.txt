# sentient-splat-slam/apps/viewer/CMakeLists.txt
# Purpose: Build the Week 1 viewer executable and fetch/link required deps.

include(FetchContent)

# -------------------------
# Dependencies (FetchContent)
# -------------------------

# GLFW: windowing + input
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM: math (vectors/matrices)
FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# spdlog: logging
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)

# fmt: formatting
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

# nlohmann/json: JSON scene loading
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# GLAD: OpenGL Loader (using a generator or a specific repo)
FetchContent_Declare(glad
  GIT_REPOSITORY https://github.com/premake-libs/glad
  GIT_TAG master
)
FetchContent_MakeAvailable(glad)

# ImGui: UI overlay (source-based)
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.0
)
FetchContent_MakeAvailable(imgui)

# Build GLAD as a static library (find the generated C source)
file(GLOB GLAD_C_SOURCES
  "${glad_SOURCE_DIR}/src/*.c"
)

if (GLAD_C_SOURCES STREQUAL "")
  message(FATAL_ERROR "GLAD C source not found in ${glad_SOURCE_DIR}/src. Expected a .c file (e.g., glad.c).")
endif()

add_library(glad_lib STATIC
  ${GLAD_C_SOURCES}
)

target_include_directories(glad_lib PUBLIC
  ${glad_SOURCE_DIR}/include
)



# -------------------------
# ImGui library target
# -------------------------
# ImGui is not a prebuilt library; we compile its sources + required backends.
add_library(imgui_lib STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp

  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_lib PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

# ImGui backend uses GLFW types/functions.
target_link_libraries(imgui_lib PUBLIC glfw)

# -------------------------
# Viewer executable
# -------------------------
add_executable(viewer
  main.cpp
  ../../src/viewer/App.cpp
  ../../src/viewer/App.hpp
  ../../src/core/Camera.cpp
  ../../src/core/Camera.hpp
  ../../src/core/Time.cpp
  ../../src/core/Time.hpp
  ../../src/core/Log.cpp
  ../../src/core/Log.hpp
  ../../src/core/config.cpp
  ../../src/render/Renderer.cpp
  ../../src/render/Renderer.hpp
  ../../src/render/Shader.cpp
  ../../src/render/Shader.hpp
  ../../src/render/Buffers.cpp
  ../../src/render/Buffers.hpp
  ../../src/scene/Scene.cpp
  ../../src/scene/Scene.hpp
  ../../src/scene/SceneIO.cpp
  ../../src/scene/SceneIO.hpp
  ../../src/ui/DebugUI.cpp
  ../../src/ui/DebugUI.hpp
)

# Allow includes like: #include "core/camera.h"
target_include_directories(viewer PRIVATE
  ../../src
)

# Link everything the viewer needs
target_link_libraries(viewer PRIVATE
  glfw
  glad_lib
  imgui_lib
  glm::glm
  spdlog::spdlog
  fmt::fmt
  nlohmann_json::nlohmann_json
)

# -------------------------
# Platform-specific linking
# -------------------------
if(APPLE)
  target_link_libraries(viewer PRIVATE
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework OpenGL"
  )
endif()

# -------------------------
# Warnings (optional but recommended)
# -------------------------
if(SSS_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(viewer PRIVATE /W4)
  else()
    target_compile_options(viewer PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()
