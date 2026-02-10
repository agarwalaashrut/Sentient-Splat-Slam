#include "Buffers.hpp"
#include <memory>

namespace sss {

// ============ VertexArray ============

VertexArray::VertexArray() {
    glGenVertexArrays(1, &m_vao);
}

VertexArray::~VertexArray() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
}

void VertexArray::bind() const {
    glBindVertexArray(m_vao);
}

void VertexArray::unbind() const {
    glBindVertexArray(0);
}

// ============ Buffer ============

Buffer::Buffer() {
    glGenBuffers(1, &m_vbo);
}

Buffer::~Buffer() {
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }
}

void Buffer::bind(GLenum target) const {
    glBindBuffer(target, m_vbo);
}

void Buffer::unbind(GLenum target) const {
    glBindBuffer(target, 0);
}

// ============ QuadMesh ============

QuadMesh::QuadMesh() 
    : m_vao(std::make_unique<VertexArray>()),
      m_vbo(std::make_unique<Buffer>()) {
    
    m_vao->bind();
    
    // Simple quad: 2 triangles, 6 vertices
    // Positions range from -1 to 1 (will be scaled in vertex shader)
    float quadVerts[] = {
        -1, -1,  // bottom-left
         1, -1,  // bottom-right
         1,  1,  // top-right
        -1, -1,  // bottom-left (second triangle)
         1,  1,  // top-right
        -1,  1   // top-left
    };
    
    m_vbo->set_data(GL_ARRAY_BUFFER, quadVerts, 12, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    
    m_vao->unbind();
}

QuadMesh::~QuadMesh() = default;

void QuadMesh::render() const {
    m_vao->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

}  // namespace sss
