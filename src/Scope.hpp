#pragma once
#include <GL/glew.h>
#include <array>
#include <cmath>
#include <vector>

#include "ShaderProgram.hpp"
#include "FFT.hpp"

extern volatile int g_windowWidth;
extern volatile int g_windowHeight;

class Scope {
public:
    Scope(
        int numPoints,
        std::array<float, 4> color,
        int thicknessInPixels);
    ~Scope();

    int getProgram() { return m_program; }
    int getNumTriangles() { return m_numTriangles; }

    void plot(
        RangeComputer& rangeComputer,
        std::vector<float>& plotX,
        std::vector<float>& plotY,
        std::vector<float>& plotNormal);
    void plotFilled(
        RangeComputer& rangeComputer,
        std::vector<float>& plotX,
        std::vector<float>& plotY);

    void render();

private:
    ShaderProgram m_shaderProgram;
    std::array<float, 4> m_color;
    int m_numSegments = 64;
    int m_numTriangles;
    GLuint m_program;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    GLfloat* m_coordinates;
    int m_coordinatesLength;
    GLuint* m_elements;
    int m_elementsLength;
    float m_thicknessInPixels;

    void makeVertexBuffer();
    void makeArrayBuffer();
    void makeElementBuffer();
    void cleanUp();
};
