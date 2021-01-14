#include <cmath>
#include <stdexcept>
#include <fstream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fftw3.h>

#include "portaudio_backend.hpp"

extern volatile int g_windowWidth;
extern volatile int g_windowHeight;

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

class Scope {
public:
    Scope(int numPoints);
    ~Scope();

    int getProgram() { return m_program; }
    int getNumTriangles() { return m_numTriangles; }

    void plot(
        std::vector<float>& plotX,
        std::vector<float>& plotY,
        std::vector<float>& plotNormal
    );

    void render();

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
    float m_thicknessInPixels = 10;

    void makeVertexBuffer();
    void makeArrayBuffer();
    void makeElementBuffer();
    void cleanUp();
};

class Spectrum {
public:
    Spectrum(int fftSize, float descentRate);
    int getFFTSize() { return m_fftSize; };

    void setWindowSize(int windowWidth, int windowHeight);
    std::vector<float>& getPlotX() { return m_plotX; };
    std::vector<float>& getPlotY() { return m_plotY; };
    std::vector<float>& getPlotNormal() { return m_plotNormal; };
    int getNumPlotPoints() { return m_numPlotPoints; }

    void update(std::vector<float>& magnitudeSpectrum);

    float fftBinToFrequency(int fftBin);
    float position(float frequency);
private:
    const int m_fftSize;
    const int m_spectrumSize;

    std::vector<int> m_binToChunk;
    int m_numChunks;
    std::vector<float> m_chunkX;
    std::vector<float> m_chunkY;

    const int m_cubicResolution = 5;
    std::vector<float> m_plotX;
    std::vector<float> m_plotY;
    std::vector<float> m_plotNormal;
    int m_numPlotPoints;

    float m_descentRate;
};

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

class VisualizerAudioCallback : public AudioCallback {
public:
    VisualizerAudioCallback(int bufferSize);
    ~VisualizerAudioCallback();
    void process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count) override;
    int getBufferSize() { return m_bufferSize; }
    float* getBuffer() { return m_buffer; }
private:
    float* m_buffer;
    const int m_bufferSize;
    int m_writePos;
};

class FFTAudioCallback : public AudioCallback {
public:
    FFTAudioCallback(int fftSize);
    ~FFTAudioCallback();
    void process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count) override;
    int getBufferSize() { return m_bufferSize; }
    int getSpectrumSize() { return m_spectrumSize; }

    std::vector<float>& getMagnitudeSpectrum() { return m_magnitudeSpectrum; }

private:
    const int m_bufferSize;
    const int m_spectrumSize;
    int m_writePos;
    float m_maxDb = -90.0f;
    double* m_samples;
    fftw_complex *m_complexSpectrum;
    fftw_plan m_fftwPlan;
    void doFFT();
    std::vector<float> m_window;
    std::vector<float> m_magnitudeSpectrum;
};
