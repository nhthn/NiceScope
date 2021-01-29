#include "main.hpp"

static std::array<float, 4> colorFromHex(int string, float alpha)
{
    return { { (string >> (4 * 4) & 0xff) / 255.0f,
        (string >> (2 * 4) & 0xff) / 255.0f,
        (string & 0xff) / 255.0f,
        alpha } };
}

static std::array<float, 4> colorFromHex(int string)
{
    return colorFromHex(string, 1.0f);
}

volatile int g_windowWidth = 640;
volatile int g_windowHeight = 480;

static void resize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    g_windowWidth = width;
    g_windowHeight = height;
}

GLFWwindow* setUpWindowAndOpenGL(const char* windowTitle)
{
    if (!glfwInit()) {
        throw std::runtime_error("GLFW initialization failed.");
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);

#if (__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    GLFWwindow* window = glfwCreateWindow(g_windowWidth, g_windowHeight, windowTitle, NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("GLFW initialization failed.");
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    if (!glCreateShader) {
        throw std::runtime_error("Unsuccessful GL initialization.");
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    return window;
}

MinimalOpenGLApp::MinimalOpenGLApp(GLFWwindow* window)
{
    m_window = window;
    glfwSetFramebufferSizeCallback(m_window, resize);
}

int main(int argc, char** argv)
{
    std::string device = "system";

    int i = 1;
    while (i < argc) {
        std::string arg = argv[i];
        if (arg == "--device") {
            i++;
            if (i >= argc) {
                throw std::runtime_error("Unexpected end of arguments");
            }
            device = argv[i];
        } else {
            throw std::runtime_error("Unrecognized argument");
        }
        i++;
    }

    auto window = setUpWindowAndOpenGL("Scope");
    MinimalOpenGLApp app(window);

    int fftSize = 2048;

    Spectrum spectrumLeft(fftSize, 2, 0.1, 1.5);
    spectrumLeft.setWindowSize(g_windowWidth, g_windowHeight);
    Scope scopeLeft(spectrumLeft.getNumPlotPoints(), colorFromHex(0xf0c674, 0.8), 8.0);

    Spectrum spectrumRight(fftSize, 2, 0.1, 1.5);
    spectrumRight.setWindowSize(g_windowWidth, g_windowHeight);
    Scope scopeRight(spectrumRight.getNumPlotPoints(), colorFromHex(0x8abeb7, 0.8), 8.0);

    Spectrum spectrum2(fftSize, 2, 3, 5);
    spectrum2.setWindowSize(g_windowWidth, g_windowHeight);
    Scope scope2(spectrum2.getNumPlotPoints(), colorFromHex(0x3c3d3b), 8.0);

    FFT fftLeft(fftSize, 0);
    FFT fftRight(fftSize, 1);
    SpectralMaximum spectralMaximum(fftLeft.getSpectrumSize());

    RangeComputer rangeComputer;

    Ingress callback(2, fftSize);

    PortAudioBackend audioBackend(&callback, device, 2);
    audioBackend.run();

    std::array<float, 4> color = colorFromHex(0x1d1f21);

    glfwSwapInterval(1);
    glClearColor(color[0], color[1], color[2], color[3]);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        callback.bufferSamples();

        fftLeft.process(callback);
        fftRight.process(callback);
        spectralMaximum.set(fftLeft.getMagnitudeSpectrum());
        spectralMaximum.computeMaximumWith(fftRight.getMagnitudeSpectrum());
        rangeComputer.process(spectralMaximum.getMaximum());

        spectrum2.update(spectralMaximum.getMagnitudeSpectrum());
        scope2.plotFilled(rangeComputer, spectrum2.getPlotX(), spectrum2.getPlotY());
        scope2.render();

        spectrumLeft.update(fftLeft.getMagnitudeSpectrum());
        scopeLeft.plot(rangeComputer, spectrumLeft.getPlotX(), spectrumLeft.getPlotY(), spectrumLeft.getPlotNormal());
        scopeLeft.render();

        spectrumRight.update(fftRight.getMagnitudeSpectrum());
        scopeRight.plot(rangeComputer, spectrumRight.getPlotX(), spectrumRight.getPlotY(), spectrumRight.getPlotNormal());
        scopeRight.render();

        glfwSwapBuffers(window);
        glfwPollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
    }

    glfwTerminate();
    return 0;
}
