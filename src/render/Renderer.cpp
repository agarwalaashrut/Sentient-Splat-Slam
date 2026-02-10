#include "Renderer.hpp"
#include <vector>
#include <memory>
#include <glm/gtc/type_ptr.hpp>

namespace sss {

Renderer::Renderer() = default;

Renderer::~Renderer() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
    }
}

void Renderer::init(int width, int height) {
    m_viewport_width = width;
    m_viewport_height = height;

    // Create shader program
    create_shader_program();

    // Create quad mesh
    m_quad_mesh = std::make_unique<QuadMesh>();

    // Create instance buffer
    m_instance_buffer = std::make_unique<InstanceBuffer>();
}

void Renderer::begin_frame() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, m_viewport_width, m_viewport_height);
}

void Renderer::end_frame() {
    // Frame finalization happens in App
}

float Renderer::aspect_ratio() const {
    return (m_viewport_height > 0) ? (float)m_viewport_width / (float)m_viewport_height : 1.f;
}

void Renderer::render_scene(const Scene& scene, const glm::mat4& view, const glm::mat4& projection) {
    const auto& gaussians = scene.get_gaussians();
    if (gaussians.empty()) {
        return;
    }

    // Convert gaussians to GPU format
    std::vector<GaussianInstanceGPU> instanceData;
    instanceData.reserve(gaussians.size());
    for (const auto& g : gaussians) {
        GaussianInstanceGPU inst;
        inst.mean_opacity = glm::vec4(g.mean, g.opacity);
        inst.quat = glm::vec4(g.rotation.x, g.rotation.y, g.rotation.z, g.rotation.w);
        inst.scale_colorx = glm::vec4(g.scale, g.color.r);
        inst.coloryz_pad = glm::vec4(g.color.g, g.color.b, 0.f, 0.f);
        instanceData.push_back(inst);
    }

    // Update instance buffer
    m_instance_buffer->vbo = std::make_unique<Buffer>();
    m_instance_buffer->vbo->set_data(GL_ARRAY_BUFFER, instanceData, GL_DYNAMIC_DRAW);
    m_instance_buffer->instance_count = instanceData.size();

    // Setup rendering state
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Bind program and set uniforms
    glUseProgram(m_program);
    glUniformMatrix4fv(Shader::get_uniform_location(m_program, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(Shader::get_uniform_location(m_program, "uProj"), 1, GL_FALSE, glm::value_ptr(projection));

    // Setup VAO and render
    glBindVertexArray(m_quad_mesh->vao());

    // Bind instance buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_instance_buffer->vbo->handle());

    // Define instanced attributes
    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(i + 1);
        glVertexAttribPointer(i + 1, 4, GL_FLOAT, GL_FALSE, sizeof(GaussianInstanceGPU), 
                            (void*)(i * sizeof(glm::vec4)));
        glVertexAttribDivisor(i + 1, 1);
    }

    // Draw
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)m_instance_buffer->instance_count);

    // Cleanup state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "[Renderer] GL Error: 0x%x\n", err);
    }
}

void Renderer::create_shader_program() {
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

    float eps = 0.03;
    float op  = max(vOpacity, eps);
    float r2  = 2.0 * log(op / eps);
    float r   = sqrt(max(r2, 0.0));
    r = min(r, 1.5);

    float z = max(-(uView * vec4(iMeanOpacity.xyz, 1.0)).z, 0.1);
    float maxWorld = 0.01 * z;
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

    FragColor = vec4(vColor * alpha, alpha);
}
    )";

    try {
        m_program = Shader::create_program(vs_src, fs_src);
    } catch (const std::exception& e) {
        fprintf(stderr, "[Renderer] Shader creation failed: %s\n", e.what());
        throw;
    }
}

}  // namespace sss
