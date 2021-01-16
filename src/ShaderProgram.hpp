#pragma once
#include <stdexcept>
#include <string>

#include <GL/glew.h>

class ShaderProgram {
public:
    ShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource);
    ~ShaderProgram();
    GLuint getProgram() { return m_program; }
    GLuint getAttribLocation(const char* key) { return glGetAttribLocation(m_program, key); }

private:
    const char* m_vertexShaderSource;
    const char* m_fragmentShaderSource;
    GLuint m_vertexShader;
    GLuint m_fragmentShader;
    GLuint m_program;

    void makeVertexShader();
    void makeFragmentShader();
    void makeProgram();
    void cleanUp();
};
