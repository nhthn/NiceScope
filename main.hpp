#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <stdexcept>
#include <fstream>

#include "portaudio_backend.hpp"

class VisualizerAudioCallback : public AudioCallback {
public:
    VisualizerAudioCallback();
    ~VisualizerAudioCallback();
    void process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count) override;
    int getBufferSize() { return m_bufferSize; }
    float* getBuffer() { return m_buffer; }
private:
    float* m_buffer;
    const int m_bufferSize;
    int m_writePos;
};
