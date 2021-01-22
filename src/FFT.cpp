#include "FFT.hpp"

Ingress::Ingress(int numChannels, int fftSize)
    : m_numChannels(numChannels)
    , m_ringBuffer(new PaUtilRingBuffer)
    , m_ringBufferData(new float[m_ringBufferSize * m_numChannels])
    , m_scratchBuffer(new float[m_scratchBufferSize * m_numChannels])
    , m_outputBuffer(new float[m_outputBufferSize * m_numChannels])
{
    ring_buffer_size_t size = PaUtil_InitializeRingBuffer(
        m_ringBuffer.get(), sizeof(float) * m_numChannels, m_ringBufferSize, m_ringBufferData.get()
    );
    if (size < 0) {
        throw std::runtime_error("Ring buffer initialization failed.");
    }

    for (int i = 0; i < m_ringBufferSize * m_numChannels; i++) {
        m_ringBufferData[i] = 0;
    }
    for (int i = 0; i < m_scratchBufferSize * m_numChannels; i++) {
        m_scratchBuffer[i] = 0;
    }
    for (int i = 0; i < m_outputBufferSize * m_numChannels; i++) {
        m_outputBuffer[i] = 0;
    }
}

void Ingress::process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count)
{
    auto writeAvailable = PaUtil_GetRingBufferWriteAvailable(m_ringBuffer.get());
    if (frame_count > writeAvailable) {
        PaUtil_AdvanceRingBufferReadIndex(m_ringBuffer.get(), frame_count);
    }
    PaUtil_WriteRingBuffer(m_ringBuffer.get(), input_buffer, frame_count);
}

void Ingress::bufferSamples()
{
    int availableFrames = PaUtil_GetRingBufferReadAvailable(m_ringBuffer.get());
    int readSamples = std::min(availableFrames, m_scratchBufferSize);
    int frameCount = PaUtil_ReadRingBuffer(m_ringBuffer.get(), m_scratchBuffer.get(), readSamples);

    for (int i = 0; i < frameCount * m_numChannels; i++) {
        m_outputBuffer.get()[m_writePos] = m_scratchBuffer.get()[i];

        m_writePos += 1;
        if (m_writePos == m_outputBufferSize) {
            m_writePos = 0;
        }
    }
}

FFT::FFT(int fftSize, int channel)
    : m_channel(channel)
    , m_bufferSize(fftSize)
    , m_spectrumSize(m_bufferSize / 2 + 1)
{

    m_samples = static_cast<double*>(fftw_malloc(sizeof(double) * m_bufferSize));
    for (int i = 0; i < m_bufferSize; i++) {
        m_samples[i] = 0;
    }

    m_window.resize(m_bufferSize);
    for (int i = 0; i < m_bufferSize; i++) {
        float t = static_cast<float>(i) / m_bufferSize;
        m_window[i] = (-std::cos(t * 2 * 3.14159265358979) + 1) * 0.5;
    }

    m_complexSpectrum = static_cast<fftw_complex*>(
        fftw_malloc(sizeof(fftw_complex) * m_spectrumSize));

    m_fftwPlan = fftw_plan_dft_r2c_1d(
        m_bufferSize, m_samples, m_complexSpectrum, FFTW_MEASURE);

    m_magnitudeSpectrum.resize(m_spectrumSize);
}

FFT::~FFT()
{
    fftw_destroy_plan(m_fftwPlan);
    fftw_free(m_samples);
    fftw_free(m_complexSpectrum);
}

void FFT::doFFT()
{
    fftw_execute(m_fftwPlan);

    float maxDb = 0;
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

void FFT::process(Ingress& ingress)
{
    for (int i = 0; i < m_bufferSize; i++) {
        int index = ingress.getWritePos() + (-m_bufferSize + i) * ingress.getNumChannels() + m_channel;
        if (index < 0) {
            index += ingress.getBufferSize();
        }
        m_samples[i] = ingress.getOutputBuffer().get()[index] * m_window[i];
    }
    doFFT();
}
