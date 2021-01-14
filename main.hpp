#pragma once

#include <cmath>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <mutex>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fftw3.h>

#include "portaudio_backend.hpp"
#include "ShaderProgram.hpp"
#include "Scope.hpp"
#include "Spectrum.hpp"

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
