#pragma once

#include <glad/glad.h>
#include <vector>

namespace sss {

/**
 * Wrapper for managing OpenGL Vertex Array Objects (VAO).
 */
class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    void bind() const;
    void unbind() const;

    GLuint handle() const { return m_vao; }

private:
    GLuint m_vao = 0;
};

/**
 * Wrapper for managing OpenGL Buffer Objects (VBO, IBO, etc.).
 */
class Buffer {
public:
    Buffer();
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    void bind(GLenum target) const;
    void unbind(GLenum target) const;

    template <typename T>
    void set_data(GLenum target, const std::vector<T>& data, GLenum usage = GL_STATIC_DRAW) {
        bind(target);
        glBufferData(target, data.size() * sizeof(T), data.data(), usage);
    }

    template <typename T>
    void set_data(GLenum target, const T* data, size_t count, GLenum usage = GL_STATIC_DRAW) {
        bind(target);
        glBufferData(target, count * sizeof(T), data, usage);
    }

    GLuint handle() const { return m_vbo; }

private:
    GLuint m_vbo = 0;
};

/**
 * Quad mesh for rendering gaussians (2 triangles, 6 vertices).
 */
class QuadMesh {
public:
    QuadMesh();
    ~QuadMesh();

    QuadMesh(const QuadMesh&) = delete;
    QuadMesh& operator=(const QuadMesh&) = delete;

    void render() const;

    GLuint vao() const { return m_vao->handle(); }
    GLuint vbo() const { return m_vbo->handle(); }

private:
    std::unique_ptr<VertexArray> m_vao;
    std::unique_ptr<Buffer> m_vbo;
};

}  // namespace sss
