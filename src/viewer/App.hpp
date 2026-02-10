#pragma once

#include <memory>
#include <string>

struct GLFWwindow;

namespace sss {

class Camera;
class Scene;
class Renderer;
class DebugUI;

class App {
public:
    App();
    ~App();

    /**
     * Initialize the application.
     * Creates GLFW window, loads OpenGL, initializes systems.
     * 
     * @param width Window width in pixels
     * @param height Window height in pixels
     * @param title Window title
     * @return true if initialization successful
     */
    bool init(int width, int height, const char* title);

    /**
     * Load a scene from a JSON file.
     * 
     * @param filepath Path to the scene JSON file
     * @return true if load successful
     */
    bool load_scene(const std::string& filepath);

    /**
     * Run the main event loop.
     * Returns when the window is closed.
     */
    void run();

    /**
     * Shutdown the application.
     * Cleans up all resources.
     */
    void shutdown();

    /**
     * Check if application should close.
     */
    bool should_close() const;

private:
    GLFWwindow* m_window = nullptr;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<DebugUI> m_debug_ui;
    std::string m_scene_path;

    bool m_mouse_captured = false;
    double m_last_mouse_x = 0.0;
    double m_last_mouse_y = 0.0;

    void handle_input();
    void update(float dt);
    void render();
};

}  // namespace sss
