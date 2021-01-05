#include "main.hpp"

const int k_maxMessageLength = 1024;

volatile static int g_windowWidth = 640;
volatile static int g_windowHeight = 480;

GLFWwindow* setUpWindowAndOpenGL(const char* windowTitle) {
    if (!glfwInit()) {
        throw std::runtime_error("GLFW initialization failed.");
    }
    GLFWwindow* window = glfwCreateWindow(g_windowWidth, g_windowHeight, windowTitle, NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("GLFW initialization failed.");
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();
    if (!glCreateShader) {
        throw std::runtime_error("Unsuccessful GLEW initialization.");
    }

    return window;
}

class MinimalOpenGLApp {
public:
    MinimalOpenGLApp(GLFWwindow* window)
    {
        m_window = window;
        glfwSetFramebufferSizeCallback(m_window, resize);
    }

    void setProgram(GLuint program)
    {
        m_program = program;
    }

    void setNumTriangles(int numTriangles)
    {
        m_numTriangles = numTriangles;
    }

private:
    const char* m_windowTitle;
    int m_numTriangles;
    GLFWwindow* m_window;
    GLuint m_program;

    void render()
    {
    }

    static void resize(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
        g_windowWidth = width;
        g_windowHeight = height;
    }
};

class ShaderProgram {
public:
    ShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
        : m_vertexShaderSource(vertexShaderSource)
        , m_fragmentShaderSource(fragmentShaderSource)
    {
        makeVertexShader();
        makeFragmentShader();
        makeProgram();
    }

    ~ShaderProgram()
    {
        cleanUp();
    }

    GLuint getProgram()
    {
        return m_program;
    }

    GLuint getAttribLocation(const char* key)
    {
        return glGetAttribLocation(m_program, key);
    }

private:
    const char* m_vertexShaderSource;
    const char* m_fragmentShaderSource;
    GLuint m_vertexShader;
    GLuint m_fragmentShader;
    GLuint m_program;

    void makeVertexShader()
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

    void makeFragmentShader()
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

    void makeProgram()
    {
        m_program = glCreateProgram();
        glAttachShader(m_program, m_vertexShader);
        glAttachShader(m_program, m_fragmentShader);
        glLinkProgram(m_program);
    }

    void cleanUp()
    {
        glDetachShader(m_program, m_vertexShader);
        glDetachShader(m_program, m_fragmentShader);
        glDeleteProgram(m_program);
        glDeleteShader(m_vertexShader);
        glDeleteShader(m_fragmentShader);
    }
};

class Scope {
public:
    Scope(int numPoints)
    {
        const char* vertexShaderSource = ("#version 130\n"
                                          "in vec2 pos;\n"
                                          "void main()\n"
                                          "{\n"
                                          "    gl_Position = vec4(pos, 1, 1);\n"
                                          "}\n");

        const char* fragmentShaderSource = (
            "#version 130\n"
            "uniform vec2 windowSize;\n"
            "out vec3 fragColor;\n"
            "void main()\n"
            "{\n"
            "fragColor = vec3(1.0);\n"
            "}\n"
        );

        ShaderProgram shaderProgram(vertexShaderSource, fragmentShaderSource);
        m_program = shaderProgram.getProgram();

        m_numSegments = numPoints - 1;

        // Each segment has two points, plus an additional two points at the end
        // of the strip. Each point has two coordinates.
        m_coordinatesLength = 4 * (m_numSegments + 1);
        m_coordinates = new GLfloat[m_coordinatesLength];
        for (int i = 0; i < m_numSegments + 1; i++) {
            float x = -1 + (float)2 * i / m_numSegments;
            m_coordinates[4 * i + 0] = x;
            m_coordinates[4 * i + 1] = -1;
            m_coordinates[4 * i + 2] = x;
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

    ~Scope()
    {
        cleanUp();
    }

    int getProgram()
    {
        return m_program;
    }

    int getNumTriangles()
    {
        return m_numTriangles;
    }

    void render(float* buffer) {
        float thicknessInWindowCoordinates = m_thicknessInPixels / g_windowHeight;
        for (int i = 0; i < m_numSegments + 1; i++) {
            m_coordinates[4 * i + 1] = buffer[i] + thicknessInWindowCoordinates;
            m_coordinates[4 * i + 3] = buffer[i] - thicknessInWindowCoordinates;
        }

        glBufferData(GL_ARRAY_BUFFER, m_coordinatesLength * sizeof(GLfloat), m_coordinates, GL_STREAM_DRAW);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(m_program);

        GLuint color = glGetUniformLocation(m_program, "windowSize");
        glUniform2f(color, g_windowWidth, g_windowHeight);

        glDrawElements(GL_TRIANGLES, 3 * m_numTriangles, GL_UNSIGNED_INT, (void*)0);
    }

private:
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
    float m_thicknessInPixels = 3;

    void makeVertexBuffer()
    {
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    }

    void makeArrayBuffer()
    {
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        glVertexAttribPointer(glGetAttribLocation(m_program, "pos"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    void makeElementBuffer()
    {
        glGenBuffers(1, &m_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_elementsLength * sizeof(GLuint), m_elements, GL_STATIC_DRAW);
    }

    void cleanUp()
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
};

VisualizerAudioCallback::VisualizerAudioCallback(int bufferSize)
    : m_bufferSize(bufferSize)
{
    m_writePos = 0;
    m_buffer = static_cast<float*>(malloc(sizeof(float) * m_bufferSize));
    for (int i = 0; i < m_bufferSize; i++) {
        m_buffer[i] = 0;
    }
}

VisualizerAudioCallback::~VisualizerAudioCallback()
{
    free(m_buffer);
}

void VisualizerAudioCallback::process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count)
{
    for (int i = 0; i < frame_count; i++) {
        m_buffer[m_writePos] = input_buffer[0][i];
        m_writePos += 1;
        if (m_writePos == m_bufferSize) {
            m_writePos = 0;
        }
    }
}

FFTAudioCallback::FFTAudioCallback(int bufferSize)
    : m_bufferSize(bufferSize),
    m_spectrumSize(bufferSize / 2 + 1)
{
    m_writePos = 0;

    m_samples = static_cast<double*>(fftw_malloc(sizeof(double) * m_bufferSize));
    for (int i = 0; i < m_bufferSize; i++) {
        m_samples[i] = 0;
    }

    m_spectrum = static_cast<fftw_complex*>(
        fftw_malloc(sizeof(fftw_complex) * m_spectrumSize)
    );

    m_buffer = static_cast<float*>(malloc(sizeof(float) * m_spectrumSize));
    for (int i = 0; i < m_spectrumSize; i++) {
        m_buffer[i] = 0;
    }

    m_fftwPlan = fftw_plan_dft_r2c_1d(m_bufferSize, m_samples, m_spectrum, FFTW_MEASURE);
}

FFTAudioCallback::~FFTAudioCallback()
{
    fftw_destroy_plan(m_fftwPlan);
    fftw_free(m_samples);
    fftw_free(m_spectrum);
}

void FFTAudioCallback::process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count)
{
    for (int i = 0; i < frame_count; i++) {
        m_samples[m_writePos] = input_buffer[0][i];
        m_writePos += 1;
        if (m_writePos == m_bufferSize) {
            fftw_execute(m_fftwPlan);

            for (int j = 0; j < m_spectrumSize; j++) {
                float real = m_spectrum[j][0];
                float imag = m_spectrum[j][1];
                float magnitude = std::hypot(real, imag);
                m_buffer[j] = magnitude;
            }

            m_writePos = 0;
        }
    }
}


int main(int argc, char** argv)
{
    auto window = setUpWindowAndOpenGL("Scope");
    MinimalOpenGLApp app(window);

    FFTAudioCallback callback(1024);

    PortAudioBackend audioBackend(&callback);
    audioBackend.run();

    Scope scope(callback.getSpectrumSize());

    while (!glfwWindowShouldClose(window)) {
        scope.render(callback.getBuffer());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
