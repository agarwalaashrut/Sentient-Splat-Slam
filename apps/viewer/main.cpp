// apps/viewer/main.cpp
// Week 1 viewer application for Sentient-Splat SLAM
// Entry point that initializes and runs the application

#include <src/viewer/App.hpp>
#include <cstdio>

int main(int argc, char** argv) {
    // Determine scene path from command line
    std::string scene_path = "assets/test_scenes/grid_gaussians.json";
    if (argc >= 2) {
        scene_path = argv[1];
    }

    // Create and initialize application
    sss::App app;
    if (!app.init(1280, 720, "Sentient-Splat SLAM — Week 1 Viewer")) {
        std::fprintf(stderr, "Failed to initialize application\n");
        return 1;
    }

    // Load the scene
    if (!app.load_scene(scene_path)) {
        std::fprintf(stderr, "Failed to load scene: %s\n", scene_path.c_str());
        app.shutdown();
        return 1;
    }

    // Run the main loop
    app.run();

    // Cleanup
    app.shutdown();
    return 0;
}

