#include "audio_callback.hpp"

float* AudioCallback::getBuffer() {
    return m_buffer;
}

void AudioCallback::process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count) {
    for (int i = 0; i < 16; i++) {
        m_buffer[i] = input_buffer[0][i];
    }
}
