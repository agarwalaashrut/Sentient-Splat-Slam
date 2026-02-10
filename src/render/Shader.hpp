#pragma once

#include <glad/glad.h>
#include <string>

namespace sss {

class Shader {
public:
    /**
     * Compile a shader from source code.
     * 
     * @param type GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.
     * @param src GLSL source code
     * @return OpenGL shader object handle
     */
    static GLuint compile(GLenum type, const char* src);

    /**
     * Create a shader program from vertex and fragment shaders.
     * 
     * @param vs_src Vertex shader source
     * @param fs_src Fragment shader source
     * @return OpenGL program handle
     */
    static GLuint create_program(const char* vs_src, const char* fs_src);

    /**
     * Get the uniform location for a program and variable name.
     */
    static GLint get_uniform_location(GLuint program, const char* name);
};

}  // namespace sss
