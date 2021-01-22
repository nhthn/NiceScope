#pragma once
#include <cmath>
#include <memory>
#include <vector>

#include <fftw3.h>

#include "pa_ringbuffer.h"

#include "portaudio_backend.hpp"

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

    void process(std::shared_ptr<float[]> buffer, int numChannels, int bufferSize, int writePos);

    std::vector<float>& getMagnitudeSpectrum() { return m_magnitudeSpectrum; }

private:
    const int m_channel;
    const int m_bufferSize;
    const int m_spectrumSize;
    float m_maxDb = -90.0f;
    double* m_samples;
    fftw_complex* m_complexSpectrum;
    fftw_plan m_fftwPlan;
    void doFFT();
    std::vector<float> m_window;
    std::vector<float> m_magnitudeSpectrum;
};

class Ingress : public AudioCallback {
public:
    Ingress(int numChannels, int fftSize);
    void process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count) override;

    void bufferSamples();
    std::shared_ptr<float[]> getOutputBuffer() { return m_outputBuffer; };
    int getNumChannels() { return m_numChannels; };
    int getBufferSize() { return m_outputBufferSize; };
    int getWritePos() { return m_writePos; };

private:
    const int m_numChannels;

    int m_writePos = 0;

    int m_ringBufferSize = 4096;
    std::unique_ptr<PaUtilRingBuffer> m_ringBuffer;
    std::unique_ptr<float[]> m_ringBufferData;

    int m_scratchBufferSize = 4096;
    std::unique_ptr<float[]> m_scratchBuffer;

    int m_outputBufferSize = 4096;
    std::shared_ptr<float[]> m_outputBuffer;
};
