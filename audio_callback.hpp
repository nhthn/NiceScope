#pragma once

typedef float* const* InputBuffer;
typedef float** OutputBuffer;

class AudioCallback {
public:
    float* getBuffer();

    void process(float* const* input_buffer, float** output_buffer, int frame_count);

private:
    float m_buffer[64];
};
