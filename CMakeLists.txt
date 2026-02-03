# sentient-splat-slam/CMakeLists.txt
# Purpose: Project-wide “contract” — standard, options, and which subprojects exist.
cmake_minimum_required(VERSION 3.20)

# Name + languages (keeps CMake/IDEs/tooling sane)
project(sentient_splat_slam LANGUAGES C CXX)

# Keep all targets on the same C++ dialect (Week 1: viewer + core)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # avoid compiler-specific GNU extensions

# Helpful for clangd/VSCode tooling; produces compile_commands.json in build/
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optional toggles you can change without editing code
option(SSS_ENABLE_WARNINGS "Enable strict compiler warnings" ON)

# Build the viewer app (apps define executables; root stays minimal)
add_subdirectory(apps/viewer)
