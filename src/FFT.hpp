#pragma once
#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

#include <fftw3.h>

#include "pa_ringbuffer.h"

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

    int getBufferSize() { return m_bufferSize; }
    int getSpectrumSize() { return m_spectrumSize; }

    void update(float* buffer);

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

    bool bufferSamples();
    float* getOutputBuffer() { return m_outputBuffer; };

private:
    const int m_numChannels;

    int m_ringBufferSize = 4096;
    int m_outputBufferSize = 2048;
    std::unique_ptr<float[]> m_buffer;
    std::unique_ptr<PaUtilRingBuffer> m_ringBuffer;
    float* m_outputBuffer;
};
