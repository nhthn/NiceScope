#pragma once
#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

#include <fftw3.h>

#include "portaudio_backend.hpp"

extern std::mutex g_magnitudeSpectrumMutex;

class FFT {
public:
    FFT(int fftSize, int m_channel);
    ~FFT();

    FFT(const FFT& other) = delete;
    FFT& operator=(const FFT& other) = delete;
    FFT(const FFT&& other) = delete;
    FFT& operator=(const FFT&& other) = delete;

    void process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count);
    int getBufferSize() { return m_bufferSize; }
    int getSpectrumSize() { return m_spectrumSize; }

    std::vector<float>& getMagnitudeSpectrum() { return m_magnitudeSpectrum; }

private:
    const int m_channel;
    const int m_bufferSize;
    const int m_spectrumSize;
    int m_writePos;
    float m_maxDb = -90.0f;
    double* m_samples;
    fftw_complex* m_complexSpectrum;
    fftw_plan m_fftwPlan;
    void doFFT();
    std::vector<float> m_window;
    std::vector<float> m_magnitudeSpectrum;
};

class FFTAudioCallback : public AudioCallback {
public:
    FFTAudioCallback(int numChannels, int fftSize);
    void process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count) override;

    void addFFT(FFT* fft);

private:
    const int m_numChannels;
    std::vector<FFT*> m_ffts;
};
