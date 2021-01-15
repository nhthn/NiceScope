#include "Scope.hpp"

const char* k_vertexShaderSource = ("#version 130\n"
                                  "in vec2 pos;\n"
                                  "void main()\n"
                                  "{\n"
                                  "    gl_Position = vec4(pos, 1, 1);\n"
                                  "}\n");

const char* k_fragmentShaderSource = (
    "#version 130\n"
    "uniform vec2 windowSize;\n"
    "uniform vec4 color;\n"
    "out vec4 fragColor;\n"
    "void main()\n"
    "{\n"
    "fragColor = color;\n"
    "}\n"
);

Scope::Scope(
    int numPoints,
    std::array<float, 4> color,
    int thicknessInPixels
)
    : m_shaderProgram(k_vertexShaderSource, k_fragmentShaderSource),
    m_color(color),
    m_thicknessInPixels(thicknessInPixels)
{
    m_program = m_shaderProgram.getProgram();

    m_numSegments = numPoints - 1;

    // Each segment has two points, plus an additional two points at the end
    // of the strip. Each point has two coordinates.
    m_coordinatesLength = 4 * (m_numSegments + 1);
    m_coordinates = new GLfloat[m_coordinatesLength];
    for (int i = 0; i < m_numSegments + 1; i++) {
        m_coordinates[4 * i + 0] = 0;
        m_coordinates[4 * i + 1] = -1;
        m_coordinates[4 * i + 2] = 0;
        m_coordinates[4 * i + 3] = 1;
    }

    m_numTriangles = 2 * m_numSegments;
    m_elementsLength = 3 * m_numTriangles;
    m_elements = new GLuint[m_elementsLength];
    for (int i = 0; i < m_numSegments; i++) {
        m_elements[6 * i + 0] = 2 * i + 0;
        m_elements[6 * i + 1] = 2 * i + 1;
        m_elements[6 * i + 2] = 2 * i + 2;
        m_elements[6 * i + 3] = 2 * i + 1;
        m_elements[6 * i + 4] = 2 * i + 2;
        m_elements[6 * i + 5] = 2 * i + 3;
    }

    makeVertexBuffer();
    makeArrayBuffer();
    makeElementBuffer();
}

Scope::~Scope()
{
    cleanUp();
}

void Scope::plot(
    std::vector<float>& plotX,
    std::vector<float>& plotY,
    std::vector<float>& plotNormal
)
{
    for (int i = 0; i < plotY.size(); i++) {
        float thicknessX = std::sin(plotNormal[i]) * m_thicknessInPixels / g_windowWidth * 0.5;
        float thicknessY = std::cos(plotNormal[i]) * m_thicknessInPixels / g_windowHeight * 0.5;
        m_coordinates[4 * i + 0] = 2 * plotX[i] - 1 - thicknessX;
        m_coordinates[4 * i + 1] = 2 * plotY[i] - 1 + thicknessY;
        m_coordinates[4 * i + 2] = 2 * plotX[i] - 1 + thicknessX;
        m_coordinates[4 * i + 3] = 2 * plotY[i] - 1 - thicknessY;
    }
}

void Scope::plotFilled(
    std::vector<float>& plotX,
    std::vector<float>& plotY
)
{
    for (int i = 0; i < plotY.size(); i++) {
        m_coordinates[4 * i + 0] = 2 * plotX[i] - 1;
        m_coordinates[4 * i + 1] = 2 * plotY[i] - 1;
        m_coordinates[4 * i + 2] = 2 * plotX[i] - 1;
        m_coordinates[4 * i + 3] = -1;
    }
}


void Scope::render()
{
    glBufferData(GL_ARRAY_BUFFER, m_coordinatesLength * sizeof(GLfloat), m_coordinates, GL_STREAM_DRAW);

    glUseProgram(m_program);

    GLuint windowSize = glGetUniformLocation(m_program, "windowSize");
    glUniform2f(windowSize, g_windowWidth, g_windowHeight);

    GLuint color = glGetUniformLocation(m_program, "color");
    glUniform4f(color, m_color[0], m_color[1], m_color[2], m_color[3]);

    glDrawElements(GL_TRIANGLES, 3 * m_numTriangles, GL_UNSIGNED_INT, (void*)0);
}

void Scope::makeVertexBuffer()
{
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
}

void Scope::makeArrayBuffer()
{
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glVertexAttribPointer(glGetAttribLocation(m_program, "pos"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void Scope::makeElementBuffer()
{
    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_elementsLength * sizeof(GLuint), m_elements, GL_STATIC_DRAW);
}

void Scope::cleanUp()
{
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &m_ebo);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &m_vbo);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &m_vao);
}

