#include "FFT.hpp"

FFTAudioCallback::FFTAudioCallback(int fftSize) :
    m_bufferSize(fftSize),
    m_spectrumSize(m_bufferSize / 2 + 1)
{
    m_writePos = 0;

    m_samples = static_cast<double*>(fftw_malloc(sizeof(double) * m_bufferSize));
    for (int i = 0; i < m_bufferSize; i++) {
        m_samples[i] = 0;
    }

    m_window.resize(m_bufferSize);
    for (int i = 0; i < m_bufferSize; i++) {
        float t = static_cast<float>(i) / m_bufferSize;
        m_window[i] = (std::cos(t * 2 * 3.14159265358979) + 1) * 0.5;
    }

    m_complexSpectrum = static_cast<fftw_complex*>(
        fftw_malloc(sizeof(fftw_complex) * m_spectrumSize)
    );

    m_fftwPlan = fftw_plan_dft_r2c_1d(
        m_bufferSize, m_samples, m_complexSpectrum, FFTW_MEASURE
    );

    m_magnitudeSpectrum.resize(m_spectrumSize);
}

FFTAudioCallback::~FFTAudioCallback()
{
    fftw_destroy_plan(m_fftwPlan);
    fftw_free(m_samples);
    fftw_free(m_complexSpectrum);
}

void FFTAudioCallback::doFFT()
{
    fftw_execute(m_fftwPlan);

    {
        const std::lock_guard<std::mutex> lock(g_magnitudeSpectrumMutex);
        float maxDb;
        for (int i = 0; i < m_spectrumSize; i++) {
            float real = m_complexSpectrum[i][0];
            float imag = m_complexSpectrum[i][1];
            float magnitude = std::hypot(real, imag);
            float db = 20 * std::log10(magnitude);
            if (db > maxDb) {
                maxDb = db;
            }
            m_magnitudeSpectrum[i] = db;
        }

        if (maxDb > m_maxDb) {
            m_maxDb = maxDb;
        }

        for (int i = 0; i < m_spectrumSize; i++) {
            m_magnitudeSpectrum[i] = (m_magnitudeSpectrum[i] - m_maxDb) / 60 + 1;
        }
    }
}

void FFTAudioCallback::process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count)
{
    for (int i = 0; i < frame_count; i++) {
        m_samples[m_writePos] = input_buffer[0][i] * m_window[m_writePos];
        m_writePos += 1;
        if (m_writePos == m_bufferSize) {
            doFFT();
            m_writePos = 0;
        }
    }
}

