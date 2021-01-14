#pragma once
#include <vector>
#include <cmath>
#include <mutex>

#include <fftw3.h>

#include "portaudio_backend.hpp"

extern std::mutex g_magnitudeSpectrumMutex;

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
