#pragma once

#include <string>
#include <glm/glm.hpp>

namespace sss {

class Scene;
class Camera;
class Renderer;

class DebugUI {
public:
    DebugUI();
    ~DebugUI();

    /**
     * Initialize ImGui context and backends.
     * Call this once at application startup.
     */
    void init(void* window_ptr);

    /**
     * Begin a new frame for UI rendering.
     * Call this once per frame before rendering UI.
     */
    void begin_frame();

    /**
     * Render the debug overlay.
     * Display camera info, scene stats, etc.
     */
    void render_debug_overlay(const Camera& camera, const Scene& scene, 
                             const Renderer& renderer, const std::string& scene_path);

    /**
     * End the frame and submit UI to renderer.
     * Call this once per frame after all UI code.
     */
    void end_frame();

    /**
     * Shutdown ImGui context.
     * Call this at application shutdown.
     */
    void shutdown();

private:
};

}  // namespace sss
