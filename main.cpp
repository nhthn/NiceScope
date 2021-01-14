#include "main.hpp"

static std::array<float, 4> colorFromHex(int string)
{
    return {{
        (string >> (4 * 4) & 0xff) / 255.0f,
        (string >> (2 * 4) & 0xff) / 255.0f,
        (string & 0xff) / 255.0f,
        1.0f
    }};
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
