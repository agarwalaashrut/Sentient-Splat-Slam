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
#include <glm/gtc/quaternion.hpp>

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
struct Gaussian3D {
  glm::vec3 mean;
  glm::vec3 scale;
  glm::quat rotation;
  float opacity;
  glm::vec3 color;
};

struct GaussianInstanceGPU {
  glm::vec4 mean_opacity;   // xyz, opacity
  glm::vec4 quat;           // xyzw
  glm::vec4 scale_colorx;   // scale.xyz, color.x
  glm::vec4 coloryz_pad;    // color.y, color.z, 0, 0
};

static_assert(sizeof(GaussianInstanceGPU) == 64, "Instance struct must be 64 bytes");

// Function to load gaussians from a JSON file
static std::vector<Gaussian3D> load_gaussians_json(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    nlohmann::json j;
    in >> j;

    std::vector<Gaussian3D> gaussians;
    if (!j.contains("gaussians") || !j["gaussians"].is_array()) {
      throw std::runtime_error("JSON missing 'gaussians' array: " + filename);
    }

    for (const auto& item : j["gaussians"]) {
        if (!item.contains("color")) {
            throw std::runtime_error("Gaussian missing 'color' field");
        }
        Gaussian3D g;
        
        // Default values for all fields
        g.mean = glm::vec3(0.0f, 0.0f, 0.0f);
        g.scale = glm::vec3(1.0f, 1.0f, 1.0f);
        g.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        g.opacity = 1.0f;
        g.color = glm::vec3(item["color"][0], item["color"][1], item["color"][2]);
        
        // Parse optional fields if present
        if (item.contains("mean") && item["mean"].is_array() && item["mean"].size() >= 3) {
            g.mean = glm::vec3(item["mean"][0], item["mean"][1], item["mean"][2]);
        } else if (item.contains("position") && item["position"].is_array() && item["position"].size() >= 3) {
            g.mean = glm::vec3(item["position"][0], item["position"][1], item["position"][2]);
        }
        if (item.contains("scale") && item["scale"].is_array() && item["scale"].size() >= 3) {
            g.scale = glm::vec3(item["scale"][0], item["scale"][1], item["scale"][2]);
        }
        if (item.contains("rotation") && item["rotation"].is_array() && item["rotation"].size() >= 4) {
            g.rotation = glm::quat(item["rotation"][0], item["rotation"][1], item["rotation"][2], item["rotation"][3]);
        }
        if (item.contains("opacity") && item["opacity"].is_number()) {
            g.opacity = item["opacity"].get<float>();
        }
        
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

  std::vector<Gaussian3D> gaussians;
  try {
    gaussians = load_gaussians_json(scene_path);
    std::fprintf(stdout, "[DEBUG] Loaded %zu gaussians from %s\n", gaussians.size(), scene_path.c_str());
    if (!gaussians.empty()) {
      const auto& g0 = gaussians[0];
      std::fprintf(stdout, "  First gaussian: mean=(%.2f, %.2f, %.2f), scale=(%.2f, %.2f, %.2f), color=(%.2f, %.2f, %.2f), opacity=%.2f\n",
        g0.mean.x, g0.mean.y, g0.mean.z,
        g0.scale.x, g0.scale.y, g0.scale.z,
        g0.color.r, g0.color.g, g0.color.b, g0.opacity);
    }
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
    points.push_back({g.mean.x, g.mean.y, g.mean.z, g.color.r, g.color.g, g.color.b});
  }

  // Shaders: render points
  const char* vs_src = R"(
  #version 330 core
layout(location=0) in vec2 aQuadPos;
layout(location=1) in vec4 iMeanOpacity;
layout(location=2) in vec4 iQuat;
layout(location=3) in vec4 iScaleColorX;
layout(location=4) in vec4 iColorYZPad;

uniform mat4 uView;
uniform mat4 uProj;

out vec2 vLocalPos;
out vec3 vColor;
out float vOpacity;

void main() {
    vLocalPos = aQuadPos;
    vColor = vec3(iScaleColorX.w, iColorYZPad.x, iColorYZPad.y);
    vOpacity = iMeanOpacity.w;

    mat3 camR = transpose(mat3(uView));
    vec3 right = camR[0];
    vec3 up    = camR[1];

    float eps = 0.03;                 // match discard
    float op  = max(vOpacity, eps);
    float r2  = 2.0 * log(op / eps);
    float r   = sqrt(max(r2, 0.0));
    r = min(r, 1.5);                  // tighter cap

    float z = max(-(uView * vec4(iMeanOpacity.xyz, 1.0)).z, 0.1);
    float maxWorld = 0.01 * z;        // tune: 0.005..0.02
    vec2 s = min(iScaleColorX.xy, vec2(maxWorld));

    vec3 worldPos = iMeanOpacity.xyz +
                    right * (aQuadPos.x * s.x * r) +
                    up    * (aQuadPos.y * s.y * r);

    gl_Position = uProj * uView * vec4(worldPos, 1.0);
}

  )";

  const char* fs_src = R"(
    #version 330 core
in vec2 vLocalPos;
in vec3 vColor;
in float vOpacity;
out vec4 FragColor;

void main() {
    float r2 = dot(vLocalPos, vLocalPos);
    float t = 1.0 - r2;
    t = clamp(t, 0.0, 1.0);

    float alpha = vOpacity * t * t;

    if (alpha < 0.03) discard;

    FragColor = vec4(vColor * alpha, alpha); // premultiplied
}

  )";

  GLuint prog = 0;
  try {
    prog = make_program(vs_src, fs_src);
    std::fprintf(stdout, "[DEBUG] Shader program compiled successfully, ID=%u\n", prog);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "%s\n", e.what());
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  float quadVerts[] = { -1,-1,  1,-1,  1,1, -1,-1,  1,1, -1,1 };

  GLuint quadVAO, quadVBO;
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
  std::fprintf(stdout, "[DEBUG] Quad VBO setup complete, quadVBO=%u\n", quadVBO);

  std::vector<GaussianInstanceGPU> instanceData;
  for(const auto& g : gaussians) {
      GaussianInstanceGPU inst;
      inst.mean_opacity = glm::vec4(g.mean, g.opacity);
      inst.quat = glm::vec4(g.rotation.x, g.rotation.y, g.rotation.z, g.rotation.w);
      inst.scale_colorx = glm::vec4(g.scale, g.color.r);
      inst.coloryz_pad = glm::vec4(g.color.g, g.color.b, 0.f, 0.f);
      instanceData.push_back(inst);
  }
  std::fprintf(stdout, "[DEBUG] Built GPU instance data for %zu gaussians (size: %zu bytes)\n", instanceData.size(), instanceData.size() * sizeof(GaussianInstanceGPU));

  GLuint instanceVBO;
  glGenBuffers(1, &instanceVBO);
  glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
  glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(GaussianInstanceGPU), instanceData.data(), GL_STATIC_DRAW);
  std::fprintf(stdout, "[DEBUG] Instance VBO setup, instanceVBO=%u, data size=%zu bytes\n", instanceVBO, instanceData.size() * sizeof(GaussianInstanceGPU));

  // Define Instanced Attributes (locations 1-4)
  for (int i = 0; i < 4; i++) {
      glEnableVertexAttribArray(i + 1);
      glVertexAttribPointer(i + 1, 4, GL_FLOAT, GL_FALSE, sizeof(GaussianInstanceGPU), (void*)(i * sizeof(glm::vec4)));
      glVertexAttribDivisor(i + 1, 1); // Tell GL this is per-instance
      std::fprintf(stdout, "[DEBUG] Instance attribute %d setup (offset=%zu bytes)\n", i + 1, i * sizeof(glm::vec4));
  }
  std::fprintf(stdout, "[DEBUG] VAO setup complete, quadVAO=%u, instanceVBO=%u\n", quadVAO, instanceVBO);

  // ImGui setup
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");
  std::fprintf(stdout, "[DEBUG] Initialization complete, GL version: %s\n", glGetString(GL_VERSION));
  std::fprintf(stdout, "[DEBUG] Entering render loop...\n");

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

    cam.on_keyboard(W, A, S, D, Q, E, dt);

    // Render
    // --- Clear screen ---
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- Render Splats ---
    glEnable(GL_BLEND);
    // Premultiplied alpha blending is standard for Splatting
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); 
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // Disable depth writes so splats can overlap smoothly

    glUseProgram(prog);
    const glm::mat4 V = cam.view();
    const glm::mat4 P = cam.projection(aspect);

    glUniformMatrix4fv(glGetUniformLocation(prog, "uView"), 1, GL_FALSE, &V[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(prog, "uProj"), 1, GL_FALSE, &P[0][0]);
    
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);  // Ensure instanceVBO is bound
    
    static bool debug_once = true;
    if (debug_once) {
      std::fprintf(stdout, "[DEBUG] Rendering %zu instances (6 vertices each)\n", instanceData.size());
      std::fprintf(stdout, "[DEBUG] Camera pos=(%.2f, %.2f, %.2f), aspect=%.2f\n", cam.position.x, cam.position.y, cam.position.z, aspect);
      
      // Check for GL errors
      GLenum err = glGetError();
      if (err != GL_NO_ERROR) {
        std::fprintf(stderr, "[DEBUG] GL Error before draw: 0x%x\n", err);
      }
      debug_once = false;
    }
    
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)instanceData.size());
    
    // Check for GL errors after draw
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
      std::fprintf(stderr, "[DEBUG] GL Error after draw: 0x%x\n", err);
    }

    glDepthMask(GL_TRUE); // Re-enable for UI/Gizmos
    glDisable(GL_BLEND);

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
    ImGui::Text("Controls: WASD, Q/E, hold RMB to look");
    ImGui::Separator();
    ImGui::Text("Viewport: %d x %d", fbw, fbh);
    ImGui::Text("Prog: %u | VAO: %u", prog, quadVAO);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glDeleteBuffers(1, &quadVBO);
  glDeleteVertexArrays(1, &quadVAO);

  // Delete the dynamic instance data
  glDeleteBuffers(1, &instanceVBO);

  // Delete the shader program
  glDeleteProgram(prog);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

