#include "DebugUI.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glfw/glfw3.h>
#include "../core/Camera.hpp"
#include "../scene/Scene.hpp"
#include "../render/Renderer.hpp"

namespace sss {

DebugUI::DebugUI() = default;

DebugUI::~DebugUI() = default;

void DebugUI::init(void* window_ptr) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)window_ptr, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

void DebugUI::begin_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DebugUI::render_debug_overlay(const Camera& camera, const Scene& scene,
                                   const Renderer& renderer, const std::string& scene_path) {
    ImGui::Begin("Debug Overlay");
    
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Separator();

    // Camera info
    ImGui::Text("Camera pos: (%.2f, %.2f, %.2f)", camera.position.x, camera.position.y, camera.position.z);
    ImGui::Text("Yaw/Pitch:  (%.1f, %.1f)", camera.yaw, camera.pitch);
    
    glm::vec3 fwd = camera.forward();
    ImGui::Text("Forward:    (%.2f, %.2f, %.2f)", fwd.x, fwd.y, fwd.z);

    ImGui::Separator();

    // Scene info
    ImGui::Text("Gaussians:  %zu", scene.gaussian_count());
    ImGui::Text("Scene:      %s", scene_path.c_str());

    ImGui::Separator();

    // Renderer info
    ImGui::Text("Viewport:   %d x %d", renderer.viewport_width(), renderer.viewport_height());
    ImGui::Text("Controls:   WASD, Q/E, hold RMB to look");

    ImGui::End();
}

void DebugUI::end_frame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DebugUI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

}  // namespace sss
