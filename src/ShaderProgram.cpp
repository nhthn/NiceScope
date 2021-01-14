#include "ShaderProgram.hpp"

static const int k_maxMessageLength = 1024;

ShaderProgram::ShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
    : m_vertexShaderSource(vertexShaderSource)
    , m_fragmentShaderSource(fragmentShaderSource)
{
    makeVertexShader();
    makeFragmentShader();
    makeProgram();
}

ShaderProgram::~ShaderProgram()
{
    cleanUp();
}

void ShaderProgram::makeVertexShader()
{
    m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(m_vertexShader, 1, &m_vertexShaderSource, NULL);
    glCompileShader(m_vertexShader);

    GLint success;
    glGetShaderiv(m_vertexShader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        GLsizei logLength = 0;
        GLchar c_message[k_maxMessageLength];
        glGetShaderInfoLog(m_vertexShader, k_maxMessageLength, &logLength, c_message);
        std::string message = c_message;
        std::string full_message = "Error compiling vertex shader: " + message;
        throw std::runtime_error(full_message);
    }
}

void ShaderProgram::makeFragmentShader()
{
    m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(m_fragmentShader, 1, &m_fragmentShaderSource, NULL);
    glCompileShader(m_fragmentShader);

    GLint success;
    glGetShaderiv(m_fragmentShader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        GLsizei logLength = 0;
        GLchar c_message[k_maxMessageLength];
        glGetShaderInfoLog(m_fragmentShader, k_maxMessageLength, &logLength, c_message);
        std::string message = c_message;
        std::string full_message = "Error compiling fragment shader: " + message;
        throw std::runtime_error(full_message);
    }
}

void ShaderProgram::makeProgram()
{
    m_program = glCreateProgram();
    glAttachShader(m_program, m_vertexShader);
    glAttachShader(m_program, m_fragmentShader);
    glLinkProgram(m_program);
}

void ShaderProgram::cleanUp()
{
    glDetachShader(m_program, m_vertexShader);
    glDetachShader(m_program, m_fragmentShader);
    glDeleteProgram(m_program);
    glDeleteShader(m_vertexShader);
    glDeleteShader(m_fragmentShader);
}

