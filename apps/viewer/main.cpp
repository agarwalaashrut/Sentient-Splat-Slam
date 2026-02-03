// apps/viewer/main.cpp
// Week 1 viewer application for Sentient-Splat
// Sets up a GLFW window with an OpenGL context and runs an event loop.
// Loads synthetic data in grid_gaussians and then renders them
// Controls: WASD move, Q/E down/up, RMB hold for mouse look

#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <nlohmann/json.hpp>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdio>
#include <vector>
#include <stdexcept>

// Gaussian structure to hold position and color temporarily
struct Gaussian {
    glm::vec3 position;
    glm::vec3 color;
};

// Actual Gaussian when stored as a 3d texture
// struct Gaussian3D {
//   glm::vec3 mean;
//   glm::mat3 covariance;
//   float opacity;
//   glm::vec3 color;
// }

// Function to load gaussians from a JSON file
static std::vector<Gaussian> load_gaussians_json(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    nlohmann::json j;
    in >> j;

    std::vector<Gaussian> gaussians;
    if (!j.contains("gaussians") || !j["gaussians"].is_array()) {
      throw std::runtime_error("JSON missing 'gaussians' array: " + filename);
    }

    for (const auto& item : j["gaussians"]) {
        if (!item.contains("position") || !item.contains("color")) {
            throw std::runtime_error("Gaussian missing 'position' or 'color' field");
        }
        Gaussian g;
        g.position = glm::vec3(item["position"][0], item["position"][1], item["position"][2]);
        g.color = glm::vec3(item["color"][0], item["color"][1], item["color"][2]);
        gaussians.push_back(g);
    }

    return gaussians;
}

// Make the camera listen to IO events and update its position and orientation accordingly
struct Camera {
  glm::vec3 position{0.f, 0.5, 4.f};
  float pitch{0.f};
  float yaw{-90.f}; // Facing towards -Z
  float fov{60.f};
  float znear{0.1f};
  float zfar{100.f};

  float speed{2.5f};
  float sensitivity{0.1f};


  //Movement mathematics
  glm::vec3 forward() const {
    const float yaw = glm::radians(this->yaw);
    const float pitch = glm::radians(this->pitch);
    return glm::normalize(glm::vec3{
      cos(yaw) * cos(pitch), 
      sin(pitch),
      sin(yaw) * cos(pitch)
    });
  }
  glm::vec3 right() const {
    return glm::normalize(glm::cross(this->forward(), glm::vec3{0.f, 1.f, 0.f}));
  }

  glm::vec3 up() const {
    return glm::normalize(glm::cross(this->right(), this->forward()));
  }

  glm::mat4 view() const {
    return glm::lookAt(position, position + forward(), up());
  }
  glm::mat4 projection(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, znear, zfar);
  }

  //Actual IO handling
  void on_mouse_move(float dx, float dy) {
    yaw += dx * sensitivity;
    pitch -= dy * sensitivity;
    pitch = glm::clamp(pitch, -89.f, 89.f);
  }

  void on_keyboard(bool W, bool A, bool S, bool D, bool Q, bool E, float dt) {
    float velocity = speed * dt;
    if (W) position += forward() * velocity;
    if (S) position -= forward() * velocity;
    if (A) position -= right() * velocity;
    if (D) position += right() * velocity;
    if (Q) position -= up() * velocity;
    if (E) position += up() * velocity;
  }
};


// GLFW error callback
static void glfw_error_cb(int error, const char* desc) {
  std::fprintf(stderr, "GLFW error %d: %s\n", error, desc ? desc : "(null)");
}

// Compile a shader of given type from source code
static GLuint compile_shader(GLenum type, const char* src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);

  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint len = 0;
    glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
    std::string log((size_t)len, '\0');
    glGetShaderInfoLog(s, len, nullptr, log.data());
    glDeleteShader(s);
    throw std::runtime_error("Shader compile failed: " + log);
  }
  return s;
}

// Link a program from vertex and fragment shader sources
static GLuint make_program(const char* vs_src, const char* fs_src) {
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);

  GLuint p = glCreateProgram();
  glAttachShader(p, vs);
  glAttachShader(p, fs);
  glLinkProgram(p);

  glDeleteShader(vs);
  glDeleteShader(fs);

  GLint ok = 0;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    GLint len = 0;
    glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
    std::string log((size_t)len, '\0');
    glGetProgramInfoLog(p, len, nullptr, log.data());
    glDeleteProgram(p);
    throw std::runtime_error("Program link failed: " + log);
  }
  return p;
}

struct GPUPoint {
  float x, y, z;
  float r, g, b;
};

int main(int argc, char** argv) {
  glfwSetErrorCallback(glfw_error_cb);
  if (!glfwInit()) {
    std::fprintf(stderr, "Failed to initialize GLFW\n");
    return 1;
  }

  // OpenGL 3.3 core
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  GLFWwindow* window = glfwCreateWindow(1280, 720, "Sentient-Splat SLAM — Week 1 Viewer", nullptr, nullptr);
  if (!window) {
    std::fprintf(stderr, "Failed to create GLFW window\n");
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // vsync for stability


  // Load OpenGL functions (GLAD)
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::fprintf(stderr, "Failed to load OpenGL via GLAD\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  // Load gaussians from JSON
  std::string scene_path = "assets/test_scenes/grid_gaussians.json";
  if (argc >= 2) scene_path = argv[1];

  std::vector<Gaussian> gaussians;
  try {
    gaussians = load_gaussians_json(scene_path);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "Scene load error: %s\n", e.what());
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  // Build GPU buffer
  std::vector<GPUPoint> points;
  points.reserve(gaussians.size());
  for (const auto& g : gaussians) {
    points.push_back({g.position.x, g.position.y, g.position.z, g.color.r, g.color.g, g.color.b});
  }

  // Shaders: render points
  const char* vs_src = R"(
    #version 330 core
    layout(location=0) in vec3 aPos;
    layout(location=1) in vec3 aCol;
    uniform mat4 uView;
    uniform mat4 uProj;
    out vec3 vCol;
    void main() {
      vCol = aCol;
      gl_Position = uProj * uView * vec4(aPos, 1.0);
      gl_PointSize = 6.0;
    }
  )";
  const char* fs_src = R"(
    #version 330 core
    in vec3 vCol;
    out vec4 FragColor;
    void main() { FragColor = vec4(vCol, 1.0); }
  )";

  GLuint prog = 0;
  try {
    prog = make_program(vs_src, fs_src);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "%s\n", e.what());
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  GLuint vao = 0, vbo = 0;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(points.size() * sizeof(GPUPoint)), points.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GPUPoint), (void*)0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GPUPoint), (void*)(3 * sizeof(float)));

  glBindVertexArray(0);

  // ImGui setup
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  Camera cam;

  // Mouse capture (RMB)
  bool mouse_captured = false;
  double last_mx = 0.0, last_my = 0.0;

  // FPS timing
  using clock = std::chrono::high_resolution_clock;
  auto last_t = clock::now();
  float fps_smooth = 0.f;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // framebuffer + aspect
    int fbw = 1, fbh = 1;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);
    const float aspect = (fbh > 0) ? (float)fbw / (float)fbh : 1.f;

    // delta time
    auto now = clock::now();
    const float dt = std::chrono::duration<float>(now - last_t).count();
    last_t = now;
    const float fps = (dt > 0.f) ? (1.f / dt) : 0.f;
    fps_smooth = (fps_smooth == 0.f) ? fps : (0.9f * fps_smooth + 0.1f * fps);

    // RMB capture
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
      if (!mouse_captured) {
        mouse_captured = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetCursorPos(window, &last_mx, &last_my);
      }
    } else {
      if (mouse_captured) {
        mouse_captured = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }
    }

    // mouse look
    if (mouse_captured) {
      double mx, my;
      glfwGetCursorPos(window, &mx, &my);
      float dx = (float)(mx - last_mx);
      float dy = (float)(my - last_my);
      last_mx = mx; last_my = my;
      cam.on_mouse_move(dx, dy);
    }

    // keyboard movement
    const bool W = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    const bool A = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    const bool S = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    const bool D = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    const bool Q = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
    const bool E = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    const bool boost = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
                       (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    cam.on_keyboard(W, A, S, D, Q, E, dt);

    // Render
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.06f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(prog);
    const glm::mat4 V = cam.view();
    const glm::mat4 P = cam.projection(aspect);

    glUniformMatrix4fv(glGetUniformLocation(prog, "uView"), 1, GL_FALSE, &V[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(prog, "uProj"), 1, GL_FALSE, &P[0][0]);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, (GLsizei)points.size());
    glBindVertexArray(0);

    // ImGui overlay
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug Overlay");
    ImGui::Text("FPS (smooth): %.1f", fps_smooth);
    ImGui::Separator();

    const glm::vec3 fwd = cam.forward();
    ImGui::Text("Camera pos: (%.2f, %.2f, %.2f)", cam.position.x, cam.position.y, cam.position.z);
    ImGui::Text("Yaw/Pitch:  (%.1f, %.1f)", cam.yaw, cam.pitch);
    ImGui::Text("Forward:    (%.2f, %.2f, %.2f)", fwd.x, fwd.y, fwd.z);

    ImGui::Separator();
    ImGui::Text("#Gaussians: %d", (int)gaussians.size());
    ImGui::Text("Scene: %s", scene_path.c_str());
    ImGui::Text("Controls: WASD, Q/E, Shift boost, hold RMB to look");
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  glDeleteProgram(prog);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

