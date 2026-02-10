#include "App.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstdio>

#include "../core/Camera.hpp"
#include "../core/Time.hpp"
#include "../core/Log.hpp"
#include "../scene/Scene.hpp"
#include "../scene/SceneIO.hpp"
#include "../render/Renderer.hpp"
#include "../ui/DebugUI.hpp"

namespace sss {

// GLFW error callback
static void glfw_error_cb(int error, const char* desc) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, desc ? desc : "(null)");
}

App::App() = default;

App::~App() {
    shutdown();
}

bool App::init(int width, int height, const char* title) {
    // Initialize GLFW
    glfwSetErrorCallback(glfw_error_cb);
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    // Configure OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Create window
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // vsync

    // Load OpenGL functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "Failed to load OpenGL via GLAD\n");
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }

    // Initialize subsystems
    Time::init();

    m_camera = std::make_unique<Camera>();
    m_scene = std::make_unique<Scene>();
    m_renderer = std::make_unique<Renderer>();
    m_renderer->init(width, height);
    m_debug_ui = std::make_unique<DebugUI>();
    m_debug_ui->init(m_window);

    std::fprintf(stdout, "[App] Initialization complete, GL version: %s\n", glGetString(GL_VERSION));

    return true;
}

bool App::load_scene(const std::string& filepath) {
    try {
        *m_scene = SceneIO::load_scene_json(filepath);
        m_scene_path = filepath;
        std::fprintf(stdout, "[App] Loaded %zu gaussians from %s\n", m_scene->gaussian_count(), filepath.c_str());
        return true;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[App] Scene load error: %s\n", e.what());
        return false;
    }
}

void App::run() {
    std::fprintf(stdout, "[App] Entering render loop...\n");

    while (!should_close()) {
        glfwPollEvents();

        Time::tick();
        float dt = Time::delta_time();

        // Update viewport
        int fbw = 1, fbh = 1;
        glfwGetFramebufferSize(m_window, &fbw, &fbh);
        m_renderer->init(fbw, fbh);

        handle_input();
        update(dt);
        render();

        glfwSwapBuffers(m_window);
    }

    std::fprintf(stdout, "[App] Exiting render loop\n");
}

void App::shutdown() {
    if (m_debug_ui) {
        m_debug_ui->shutdown();
    }

    m_renderer.reset();
    m_scene.reset();
    m_camera.reset();
    m_debug_ui.reset();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}

bool App::should_close() const {
    return m_window && glfwWindowShouldClose(m_window);
}

void App::handle_input() {
    // RMB mouse capture
    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (!m_mouse_captured) {
            m_mouse_captured = true;
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwGetCursorPos(m_window, &m_last_mouse_x, &m_last_mouse_y);
        }
    } else {
        if (m_mouse_captured) {
            m_mouse_captured = false;
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    // Mouse look
    if (m_mouse_captured) {
        double mx, my;
        glfwGetCursorPos(m_window, &mx, &my);
        float dx = (float)(mx - m_last_mouse_x);
        float dy = (float)(my - m_last_mouse_y);
        m_last_mouse_x = mx;
        m_last_mouse_y = my;
        m_camera->on_mouse_move(dx, dy);
    }

    // Keyboard movement
    bool W = glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS;
    bool A = glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS;
    bool S = glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS;
    bool D = glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS;
    bool Q = glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS;
    bool E = glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS;

    m_camera->on_keyboard(W, A, S, D, Q, E, Time::delta_time());
}

void App::update(float dt) {
    // Game logic updates go here
    (void)dt; // unused for now
}

void App::render() {
    m_renderer->begin_frame();
    m_renderer->render_scene(*m_scene, m_camera->view(), m_camera->projection(m_renderer->aspect_ratio()));

    // UI overlay
    m_debug_ui->begin_frame();
    m_debug_ui->render_debug_overlay(*m_camera, *m_scene, *m_renderer, m_scene_path);
    m_debug_ui->end_frame();
}

}  // namespace sss
