#pragma once
#include <vector>
#include <cmath>
#include <mutex>

extern std::mutex g_magnitudeSpectrumMutex;

class Spectrum {
public:
    Spectrum(int fftSize, float descentRate);
    int getFFTSize() { return m_fftSize; };

    void setWindowSize(int windowWidth, int windowHeight);
    std::vector<float>& getPlotX() { return m_plotX; };
    std::vector<float>& getPlotY() { return m_plotY; };
    std::vector<float>& getPlotNormal() { return m_plotNormal; };
    int getNumPlotPoints() { return m_numPlotPoints; }

    void update(std::vector<float>& magnitudeSpectrum);

    float fftBinToFrequency(int fftBin);
    float position(float frequency);
private:
    const int m_fftSize;
    const int m_spectrumSize;

    std::vector<int> m_binToChunk;
    int m_numChunks;
    std::vector<float> m_chunkX;
    std::vector<float> m_chunkY;

    const int m_cubicResolution = 5;
    std::vector<float> m_plotX;
    std::vector<float> m_plotY;
    std::vector<float> m_plotNormal;
    int m_numPlotPoints;

    float m_descentRate;
};