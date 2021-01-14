#include "main.hpp"

static std::mutex g_magnitudeSpectrumMutex;

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
