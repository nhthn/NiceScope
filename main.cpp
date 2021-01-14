#include "main.hpp"

static std::mutex g_magnitudeSpectrumMutex;

const int k_maxMessageLength = 1024;

static std::array<float, 4> colorFromHex(int string)
{
    return {{
        (string >> (4 * 4) & 0xff) / 255.0f,
        (string >> (2 * 4) & 0xff) / 255.0f,
        (string & 0xff) / 255.0f,
        1.0f
    }};
}

static float cubicInterpolate(float t, float y0, float y1, float y2, float y3)
{
    return (
        (-y0 + 3 * y1 - 3 * y2 + y3) * t * t * t
        + (2 * y0 - 5 * y1 + 4 * y2 - y3) * t * t
        + (-y0 + y2) * t
        + 2 * y1
    ) * 0.5;
}

static float dCubicInterpolate(float t, float y0, float y1, float y2, float y3)
{
    return (
        3 * (-y0 + 3 * y1 - 3 * y2 + y3) * t * t
        + 2 * (2 * y0 - 5 * y1 + 4 * y2 - y3) * t
        + (-y0 + y2)
    ) * 0.5;
}

volatile int g_windowWidth = 640;
volatile int g_windowHeight = 480;

static void resize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    g_windowWidth = width;
    g_windowHeight = height;
}

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

MinimalOpenGLApp::MinimalOpenGLApp(GLFWwindow* window)
{
    m_window = window;
    glfwSetFramebufferSizeCallback(m_window, resize);
}

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

Scope::Scope(int numPoints, std::array<float, 4> color)
    : m_shaderProgram(k_vertexShaderSource, k_fragmentShaderSource),
    m_color(color)
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

FFTAudioCallback::FFTAudioCallback(int fftSize) :
    m_bufferSize(fftSize),
    m_spectrumSize(m_bufferSize / 2 + 1)
{
    m_writePos = 0;

    m_samples = static_cast<double*>(fftw_malloc(sizeof(double) * m_bufferSize));
    for (int i = 0; i < m_bufferSize; i++) {
        m_samples[i] = 0;
    }

    m_window.resize(m_bufferSize);
    for (int i = 0; i < m_bufferSize; i++) {
        float t = static_cast<float>(i) / m_bufferSize;
        m_window[i] = (std::cos(t * 2 * 3.14159265358979) + 1) * 0.5;
    }

    m_complexSpectrum = static_cast<fftw_complex*>(
        fftw_malloc(sizeof(fftw_complex) * m_spectrumSize)
    );

    m_fftwPlan = fftw_plan_dft_r2c_1d(
        m_bufferSize, m_samples, m_complexSpectrum, FFTW_MEASURE
    );

    m_magnitudeSpectrum.resize(m_spectrumSize);
}

FFTAudioCallback::~FFTAudioCallback()
{
    fftw_destroy_plan(m_fftwPlan);
    fftw_free(m_samples);
    fftw_free(m_complexSpectrum);
}

void FFTAudioCallback::doFFT()
{
    fftw_execute(m_fftwPlan);

    {
        const std::lock_guard<std::mutex> lock(g_magnitudeSpectrumMutex);
        float maxDb;
        for (int i = 0; i < m_spectrumSize; i++) {
            float real = m_complexSpectrum[i][0];
            float imag = m_complexSpectrum[i][1];
            float magnitude = std::hypot(real, imag);
            float db = 20 * std::log10(magnitude);
            if (db > maxDb) {
                maxDb = db;
            }
            m_magnitudeSpectrum[i] = db;
        }

        if (maxDb > m_maxDb) {
            m_maxDb = maxDb;
        }

        for (int i = 0; i < m_spectrumSize; i++) {
            m_magnitudeSpectrum[i] = (m_magnitudeSpectrum[i] - m_maxDb) / 60 + 1;
        }
    }
}

void FFTAudioCallback::process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count)
{
    for (int i = 0; i < frame_count; i++) {
        m_samples[m_writePos] = input_buffer[0][i] * m_window[m_writePos];
        m_writePos += 1;
        if (m_writePos == m_bufferSize) {
            doFFT();
            m_writePos = 0;
        }
    }
}


Spectrum::Spectrum(int fftSize, float descentRate)
    : m_spectrumSize(fftSize / 2 + 1),
    m_fftSize(fftSize),
    m_numChunks(0),
    m_numPlotPoints(0),
    m_descentRate(descentRate)
{
    m_chunkX.reserve(m_spectrumSize);
    m_chunkY.reserve(m_spectrumSize);

    m_plotX.reserve(m_spectrumSize * m_cubicResolution);
    m_plotY.reserve(m_spectrumSize * m_cubicResolution);
    m_plotNormal.reserve(m_spectrumSize * m_cubicResolution);

    m_binToChunk.reserve(m_spectrumSize);
}

float Spectrum::fftBinToFrequency(int fftBin)
{
    return 24000 * static_cast<float>(fftBin) / m_spectrumSize;
}

static float erbs(float frequency) {
    return 21.4 * std::log10(0.00437f * frequency + 1);
}

float Spectrum::position(float frequency)
{
    return (std::log2(frequency) - std::log2(50)) / (std::log2(20e3) - std::log2(50));
}

void Spectrum::setWindowSize(int windowWidth, int windowHeight)
{
    int chunkN = 2;

    m_chunkX.clear();

    m_binToChunk.clear();
    m_binToChunk.resize(m_spectrumSize);

    bool foundMultiChunk = false;
    int firstMultiChunk;
    int firstMultiChunkOffset;
    float firstMultiChunkPosition;
    int chunkIndex;

    int lastNominalChunk = -1;

    for (int i = 0; i < m_spectrumSize; i++) {
        float frequency = fftBinToFrequency(i);
        float thePosition = position(frequency);
        if (thePosition > 1.0) {
            m_binToChunk[i] = -1;
            continue;
        }
        int nominalChunk = static_cast<int>(std::floor(thePosition * windowWidth / chunkN));
        if (foundMultiChunk) {
            chunkIndex = firstMultiChunk + nominalChunk - firstMultiChunkOffset;
            m_binToChunk[i] = chunkIndex;
            if (nominalChunk != lastNominalChunk) {
                m_chunkX.push_back(thePosition);
            }
        } else {
            chunkIndex = i;
            m_binToChunk[i] = i;
            // FIXME may be an off-by-one error in here for the first multichunk
            m_chunkX.push_back(thePosition);
            if (nominalChunk == lastNominalChunk) {
                foundMultiChunk = true;
                firstMultiChunk = i;
                firstMultiChunkOffset = nominalChunk;
                firstMultiChunkPosition = thePosition;
            }
        }
        lastNominalChunk = nominalChunk;
    }

    m_numChunks = m_chunkX.size();
    m_chunkY.resize(m_numChunks);

    m_numPlotPoints = m_numChunks * m_cubicResolution;

    m_plotY.resize(m_numPlotPoints);
    m_plotX.resize(m_numPlotPoints);

    for (int i = 0; i < m_numPlotPoints; i++) {
        int t1 = i / m_cubicResolution;
        int t0 = std::max(t1 - 1, 0);
        int t2 = std::min(t1 + 1, m_numChunks - 1);
        int t3 = std::min(t1 + 2, m_numChunks - 1);
        float x0 = m_chunkX[t0];
        float x1 = m_chunkX[t1];
        float x2 = m_chunkX[t2];
        float x3 = m_chunkX[t3];
        float t = static_cast<float>(i) / m_cubicResolution - t1;
        m_plotX[i] = cubicInterpolate(t, x0, x1, x2, x3);
    }

    m_plotNormal.clear();
    m_plotNormal.resize(m_numPlotPoints);
}

void Spectrum::update(std::vector<float>& magnitudeSpectrum)
{
    for (int i = 0; i < m_numChunks; i++) {
        m_chunkY[i] -= m_descentRate;
    }

    {
        const std::lock_guard<std::mutex> lock(g_magnitudeSpectrumMutex);
        for (int i = 0; i < m_spectrumSize; i++) {
            int chunk = m_binToChunk[i];
            if (chunk < 0 || chunk >= m_chunkY.size()) {
                break;
            }
            m_chunkY[chunk] = std::max(m_chunkY[chunk], magnitudeSpectrum[i]);
        }
    }

    for (int i = 0; i < m_numPlotPoints; i++) {
        int t1 = i / m_cubicResolution;
        int t0 = std::max(t1 - 1, 0);
        int t2 = std::min(t1 + 1, m_numChunks - 1);
        int t3 = std::min(t1 + 2, m_numChunks - 1);
        float y0 = m_chunkY[t0];
        float y1 = m_chunkY[t1];
        float y2 = m_chunkY[t2];
        float y3 = m_chunkY[t3];
        float t = static_cast<float>(i) / m_cubicResolution - t1;
        m_plotY[i] = cubicInterpolate(t, y0, y1, y2, y3);
        float x0 = m_chunkX[t0];
        float x1 = m_chunkX[t1];
        float x2 = m_chunkX[t2];
        float x3 = m_chunkX[t3];
        m_plotNormal[i] = std::atan2(
            dCubicInterpolate(t, y0, y1, y2, y3),
            dCubicInterpolate(t, x0, x1, x2, x3)
        );
    }
}


int main(int argc, char** argv)
{
    auto window = setUpWindowAndOpenGL("Scope");
    MinimalOpenGLApp app(window);

    int fftSize = 2048;

    Spectrum spectrum(fftSize, 1e-2);
    spectrum.setWindowSize(g_windowWidth, g_windowHeight);
    Scope scope(spectrum.getNumPlotPoints(), colorFromHex(0xc5c8c6));

    Spectrum spectrum2(fftSize, 1e-3);
    spectrum2.setWindowSize(g_windowWidth, g_windowHeight);
    Scope scope2(spectrum2.getNumPlotPoints(), colorFromHex(0x2c2d2b));

    FFTAudioCallback callback(fftSize);

    PortAudioBackend audioBackend(&callback);
    audioBackend.run();

    std::array<float, 4> color = colorFromHex(0x1d1f21);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(color[0], color[1], color[2], color[3]);
        glClear(GL_COLOR_BUFFER_BIT);

        spectrum2.update(callback.getMagnitudeSpectrum());
        scope2.plotFilled(spectrum2.getPlotX(), spectrum2.getPlotY());
        scope2.render();

        spectrum.update(callback.getMagnitudeSpectrum());
        scope.plot(spectrum.getPlotX(), spectrum.getPlotY(), spectrum.getPlotNormal());
        scope.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
