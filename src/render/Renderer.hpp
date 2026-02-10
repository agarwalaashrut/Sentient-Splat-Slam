#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include "../scene/Scene.hpp"
#include "Buffers.hpp"
#include "Shader.hpp"

namespace sss {

class Renderer {
public:
    Renderer();
    ~Renderer();

    /**
     * Initialize renderer with window dimensions.
     */
    void init(int width, int height);

    /**
     * Prepare for a new frame (clear buffers, etc).
     */
    void begin_frame();

    /**
     * Finalize frame and swap buffers.
     */
    void end_frame();

    /**
     * Render a scene of gaussians.
     * 
     * @param scene The scene to render
     * @param view View matrix from camera
     * @param projection Projection matrix from camera
     */
    void render_scene(const Scene& scene, const glm::mat4& view, const glm::mat4& projection);

    /**
     * Get current viewport width.
     */
    int viewport_width() const { return m_viewport_width; }

    /**
     * Get current viewport height.
     */
    int viewport_height() const { return m_viewport_height; }

    /**
     * Get current aspect ratio.
     */
    float aspect_ratio() const;

private:
    struct InstanceBuffer {
        std::unique_ptr<Buffer> vbo;
        size_t instance_count = 0;
    };

    int m_viewport_width = 0;
    int m_viewport_height = 0;

    GLuint m_program = 0;
    std::unique_ptr<QuadMesh> m_quad_mesh;
    std::unique_ptr<InstanceBuffer> m_instance_buffer;

    void create_shader_program();
};

}  // namespace sss
