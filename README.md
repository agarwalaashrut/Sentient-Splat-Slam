# Sentient Splat SLAM

A real-time 3D Gaussian Splatting SLAM (Simultaneous Localization and Mapping) system with an interactive OpenGL viewer. This project explores efficient 3D scene representation using Gaussian splatting techniques for fast rendering and localization.

## Features

- **Gaussian Point Rendering**: Real-time rendering of 3D Gaussian point clouds using OpenGL
- **Interactive 3D Viewer**: Explore scenes with intuitive camera controls (WASD, mouse look)
- **Scene Loading**: Load JSON-based scene definitions with Gaussian position, scale, rotation, and color
- **Synthetic Scene Generators**: Python scripts to generate diverse test scenes (grid, galaxy, procedural clusters)

## Project Structure

```
Sentient-Splat-Slam/
├── apps/
│   └── viewer/                 # Interactive viewer application
│       ├── main.cpp            # Main viewer entry point
│       └── CMakeLists.txt
├── src/
│   └── core/                   # Core library components
│       ├── camera.cpp/h        # Camera management
│       ├── config.cpp/h        # Configuration handling
│       ├── logging.h           # Logging utilities
│       └── types.h             # Type definitions
├── scripts/                    # Build and run scripts
│   ├── build.ps1               # Build script (PowerShell)
│   ├── clean.ps1               # Clean build artifacts
│   ├── run_viewer.ps1          # Run the viewer application
│   ├── gen_grid_scene.py       # Generate grid-based test scene
│   ├── gen_galaxy_scene.py     # Generate spiral galaxy scene
│   └── generate_gaussians_scene.py  # Generate mixed procedural scene
├── assets/
│   └── test_scenes/            # Test scene definitions
│       ├── grid_gaussians.json
│       ├── grid_gaussians_large.json
│       ├── galaxy.json
│       └── scene_gaussians.json
├── CMakeLists.txt              # Root CMake configuration
└── README.md                   # This file
```

## Dependencies

The project uses the following external libraries (automatically fetched via CMake):

- **GLFW 3.4**: Window management and input handling
- **GLAD**: OpenGL loader for modern OpenGL
- **GLM 1.0.1**: Linear algebra and matrix math
- **ImGui**: Immediate-mode GUI framework
- **spdlog 1.14.1**: Structured logging
- **fmt 10.2.1**: String formatting
- **nlohmann/json 3.11.3**: JSON parsing and scene loading
- **Visual Studio 2022** (Windows): C++20 compiler

## Building

### Prerequisites

- Visual Studio 2022 (with C++20 support)
- CMake 3.20 or later
- Windows 10/11

### Build Instructions

1. **Using the build script** (recommended on Windows):
   ```powershell
   .\scripts\build.ps1
   ```

2. **Manual CMake build**:
   ```bash
   cmake -S . -B build -G "Visual Studio 18 2026" -A x64
   cmake --build build --config Debug
   ```

The build will:
- Fetch and compile all dependencies
- Generate compile commands for tooling (VS Code, clangd)
- Create executables in `build/apps/viewer/{Debug,Release}/`

## Running

### Quick Start

Run the viewer with a test scene:
```powershell
.\scripts\run_viewer.ps1
```

### Manual Execution

```powershell
.\build\apps\viewer\Debug\viewer.exe assets\test_scenes\grid_gaussians.json
```

### Camera Controls

- **WASD**: Move forward/backward and strafe left/right
- **Q/E**: Move down/up
- **Right Mouse Button**: Hold and move mouse to rotate view

## Scene Format

Scenes are defined in JSON format with the following Gaussian properties:
```json
{
  "format": "gaussian3d_v1",
  "gaussians": [
    {
      "mean": [x, y, z],
      "scale": [sx, sy, sz],
      "rotation": [qx, qy, qz, qw],
      "opacity": a,
      "color": [r, g, b]
    }
  ]
}
```

Generate synthetic test scenes:
```bash
python scripts/gen_grid_scene.py --out assets/test_scenes/grid_gaussians.json
python scripts/gen_galaxy_scene.py --out assets/test_scenes/galaxy.json
python scripts/generate_gaussians_scene.py --out assets/test_scenes/scene_gaussians.json
```

## Development

### Code Style

- **C++20** standard with strict compiler warnings enabled
- CMake configuration uses modern practices
- Compiler-specific GNU extensions disabled

### Logging

The project uses **spdlog** for structured logging. Check logs for debugging information during execution.

### Project Configuration

Edit `CMakeLists.txt` to toggle options:
- `SSS_ENABLE_WARNINGS`: Enable/disable strict compiler warnings (default: ON)

## Clean Build

Remove all build artifacts:
```powershell
.\scripts\clean.ps1
```

## Future Development

Planned improvements include:
- SLAM algorithm integration for camera localization and mapping
- Real-time Gaussian parameter optimization
- Point cloud import/export (PLY, PCD formats)
- Advanced rendering techniques (depth sorting, level-of-detail)
- GPU-accelerated Gaussian splatting with compute shaders
- Gaussian parameter visualization and editing tools

## License

[Add license information if applicable]

## Author

[Add author information if applicable]
