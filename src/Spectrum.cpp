#include "Spectrum.hpp"

std::mutex g_magnitudeSpectrumMutex;

static float cubicInterpolate(float t, float y0, float y1, float y2, float y3)
{
    return (
        (-y0 + 3 * y1 - 3 * y2 + y3) * t * t * t
        + (2 * y0 - 5 * y1 + 4 * y2 - y3) * t * t
        + (-y0 + y2) * t
        + 2 * y1
    ) * 0.5;
}

static float dCubicInterpolate(float t, float y0, float y1, float y2, float y3)
{
    return (
        3 * (-y0 + 3 * y1 - 3 * y2 + y3) * t * t
        + 2 * (2 * y0 - 5 * y1 + 4 * y2 - y3) * t
        + (-y0 + y2)
    ) * 0.5;
}

Spectrum::Spectrum(
    int fftSize,
    float plotPointPadding,
    float attack,
    float release
)
    : m_spectrumSize(fftSize / 2 + 1),
    m_fftSize(fftSize),
    m_numChunks(0),
    m_numPlotPoints(0),
    m_plotPointPadding(plotPointPadding)
{
    m_kAttack = 1 - std::exp(-attack);
    m_kRelease = 1 - std::exp(-release);

    m_chunkX.reserve(m_spectrumSize);
    m_chunkY.reserve(m_spectrumSize);
    m_lastChunkY.reserve(m_spectrumSize);

    m_plotX.reserve(m_spectrumSize * m_cubicResolution);
    m_plotY.reserve(m_spectrumSize * m_cubicResolution);
    m_plotNormal.reserve(m_spectrumSize * m_cubicResolution);

    m_binToChunk.reserve(m_spectrumSize);
}

float Spectrum::fftBinToFrequency(int fftBin)
{
    return 24000 * static_cast<float>(fftBin) / m_spectrumSize;
}

static float erbs(float frequency) {
    return 21.4 * std::log10(0.00437f * frequency + 1);
}

float Spectrum::position(float frequency)
{
    return (std::log2(frequency) - std::log2(50)) / (std::log2(20e3) - std::log2(50));
}

void Spectrum::setWindowSize(int windowWidth, int windowHeight)
{
    m_chunkX.clear();

    m_binToChunk.clear();
    m_binToChunk.resize(m_spectrumSize);

    bool foundMultiChunk = false;
    int firstMultiChunk;
    int firstMultiChunkOffset;
    float firstMultiChunkPosition;
    int chunkIndex;

    int lastNominalChunk = -1;

    for (int i = 0; i < m_spectrumSize; i++) {
        float frequency = fftBinToFrequency(i);
        float thePosition = position(frequency);
        if (thePosition > 1.0) {
            m_binToChunk[i] = -1;
            continue;
        }
        int nominalChunk = static_cast<int>(std::floor(thePosition * windowWidth / m_plotPointPadding));
        if (foundMultiChunk) {
            chunkIndex = firstMultiChunk + nominalChunk - firstMultiChunkOffset;
            m_binToChunk[i] = chunkIndex;
            if (nominalChunk != lastNominalChunk) {
                m_chunkX.push_back(thePosition);
            }
        } else {
            chunkIndex = i;
            m_binToChunk[i] = i;
            // FIXME may be an off-by-one error in here for the first multichunk
            m_chunkX.push_back(thePosition);
            if (nominalChunk == lastNominalChunk) {
                foundMultiChunk = true;
                firstMultiChunk = i;
                firstMultiChunkOffset = nominalChunk;
                firstMultiChunkPosition = thePosition;
            }
        }
        lastNominalChunk = nominalChunk;
    }

    m_numChunks = m_chunkX.size();
    m_chunkY.resize(m_numChunks);

    m_numPlotPoints = m_numChunks * m_cubicResolution;

    m_plotY.resize(m_numPlotPoints);
    m_plotX.resize(m_numPlotPoints);

    for (int i = 0; i < m_numPlotPoints; i++) {
        int t1 = i / m_cubicResolution;
        int t0 = std::max(t1 - 1, 0);
        int t2 = std::min(t1 + 1, m_numChunks - 1);
        int t3 = std::min(t1 + 2, m_numChunks - 1);
        float x0 = m_chunkX[t0];
        float x1 = m_chunkX[t1];
        float x2 = m_chunkX[t2];
        float x3 = m_chunkX[t3];
        float t = static_cast<float>(i) / m_cubicResolution - t1;
        m_plotX[i] = cubicInterpolate(t, x0, x1, x2, x3);
    }

    m_plotNormal.clear();
    m_plotNormal.resize(m_numPlotPoints);
}

void Spectrum::update(std::vector<float>& magnitudeSpectrum)
{
    for (int i = 0; i < m_numChunks; i++) {
        m_chunkY[i] = -1;
    }

    {
        const std::lock_guard<std::mutex> lock(g_magnitudeSpectrumMutex);
        for (int i = 0; i < m_spectrumSize; i++) {
            int chunk = m_binToChunk[i];
            if (chunk < 0 || chunk >= m_chunkY.size()) {
                break;
            }
            m_chunkY[chunk] = std::max(m_chunkY[chunk], magnitudeSpectrum[i]);
        }
    }

    for (int i = 0; i < m_numChunks; i++) {
        if (m_lastChunkY[i] > m_chunkY[i]) {
            m_lastChunkY[i] = m_lastChunkY[i] * m_kRelease + m_chunkY[i] * (1 - m_kRelease);
        } else {
            m_lastChunkY[i] = m_lastChunkY[i] * m_kAttack + m_chunkY[i] * (1 - m_kAttack);
        }
    }

    for (int i = 0; i < m_numPlotPoints; i++) {
        int t1 = i / m_cubicResolution;
        int t0 = std::max(t1 - 1, 0);
        int t2 = std::min(t1 + 1, m_numChunks - 1);
        int t3 = std::min(t1 + 2, m_numChunks - 1);
        float y0 = m_lastChunkY[t0];
        float y1 = m_lastChunkY[t1];
        float y2 = m_lastChunkY[t2];
        float y3 = m_lastChunkY[t3];
        float t = static_cast<float>(i) / m_cubicResolution - t1;
        m_plotY[i] = cubicInterpolate(t, y0, y1, y2, y3);
        float x0 = m_chunkX[t0];
        float x1 = m_chunkX[t1];
        float x2 = m_chunkX[t2];
        float x3 = m_chunkX[t3];
        m_plotNormal[i] = std::atan2(
            dCubicInterpolate(t, y0, y1, y2, y3),
            dCubicInterpolate(t, x0, x1, x2, x3)
        );
    }
}

