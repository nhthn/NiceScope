#include <cmath>
#include <stdexcept>
#include <fstream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fftw3.h>

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

    float fftBinToFrequency(int fftBin);
    float position(float frequency);

    void setWindowSize(int windowWidth, int windowHeight);
    std::vector<float>& getPlotX() { return m_plotX; };
    std::vector<float>& getPlotY() { return m_plotY; };
    int getNumPlotPoints() { return m_numPlotPoints; }

private:
    const int m_bufferSize;
    const int m_spectrumSize;
    int m_numChunks;
    int m_writePos;
    float m_maxDb = -90.0f;
    double* m_samples;
    fftw_complex *m_spectrum;
    fftw_plan m_fftwPlan;
    std::vector<float> m_magnitudeSpectrum;

    std::vector<float> m_chunkX;
    std::vector<float> m_chunkY;

    const int m_cubicResolution = 5;
    std::vector<float> m_plotX;
    std::vector<float> m_plotY;
    int m_numPlotPoints;

    std::vector<int> m_binToChunk;
    void doFFT();
};
