#pragma once

#include <stdexcept>
#include <thread>
#include <chrono>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "FFT.hpp"
#include "Scope.hpp"
#include "ShaderProgram.hpp"
#include "Spectrum.hpp"
#include "portaudio_backend.hpp"

extern volatile int g_windowWidth;
extern volatile int g_windowHeight;

class MinimalOpenGLApp {
public:
    MinimalOpenGLApp(GLFWwindow* window);

    void setProgram(GLuint program) { m_program = program; }
    void setNumTriangles(int numTriangles) { m_numTriangles = numTriangles; }

private:
    const char* m_windowTitle;
    int m_numTriangles;
    GLFWwindow* m_window;
    GLuint m_program;
};
