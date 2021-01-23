#pragma once
#include <cmath>
#include <memory>
#include <vector>

#include <fftw3.h>

#include "pa_ringbuffer.h"

#include "portaudio_backend.hpp"

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
    const int m_ringBufferSize;
    const int m_scratchBufferSize;
    const int m_outputBufferSize;

    int m_writePos = 0;

    std::unique_ptr<PaUtilRingBuffer> m_ringBuffer;
    std::unique_ptr<float[]> m_ringBufferData;

    std::unique_ptr<float[]> m_scratchBuffer;

    std::shared_ptr<float[]> m_outputBuffer;
};

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

    void process(Ingress& ingress);

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

class SpectralMaximum {
public:
    SpectralMaximum(int spectrumSize);

    std::vector<float>& getMagnitudeSpectrum() { return m_magnitudeSpectrum; };
    void set(std::vector<float> spectrum);
    void computeMaximumWith(std::vector<float> spectrum);
    float getMaximum();

private:
    const int m_spectrumSize;
    std::vector<float> m_magnitudeSpectrum;
};

class RangeComputer {
public:
    float getTop() { return 15; };
    float getBottom() { return getTop() - 60; };

    float convertValueToScreenY(float value);
};
