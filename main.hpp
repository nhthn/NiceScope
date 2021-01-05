#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fftw3.h>
#include <cmath>
#include <stdexcept>
#include <fstream>

#include "portaudio_backend.hpp"

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
    FFTAudioCallback(int bufferSize);
    ~FFTAudioCallback();
    void process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count) override;
    int getBufferSize() { return m_bufferSize; }
    int getSpectrumSize() { return m_spectrumSize; }
    float* getBuffer() { return m_buffer; }
private:
    const int m_bufferSize;
    const int m_spectrumSize;
    int m_writePos;
    double* m_samples;
    fftw_complex *m_spectrum;
    fftw_plan m_fftwPlan;
    float* m_buffer;
};
