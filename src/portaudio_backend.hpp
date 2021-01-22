#pragma once
#include "portaudio.h"
#include <iostream>
#include <string>

typedef float* const* InputBuffer;
typedef float** OutputBuffer;

class AudioCallback {
public:
    virtual void process(float* const* input_buffer, float** output_buffer, int frame_count) = 0;
};

class PortAudioBackend {
public:
    PortAudioBackend(AudioCallback* callback, std::string device, int numChannels);

    void run();
    void end();
    void process(
        InputBuffer input_buffer,
        OutputBuffer output_buffer,
        int frame_count);

private:
    AudioCallback* m_callback;
    std::string m_device;
    const int m_numChannels;
    PaSampleFormat sample_format;
    PaStream* m_stream;
    const float m_sample_rate = 48000.0f;
    const int m_block_size = 256;
    PaStreamParameters input_parameters;
    PaStreamParameters output_parameters;


    void handle_error(PaError error);
    int find_device();
    static int stream_callback(
        const void* inputBuffer,
        void* outputBuffer,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);
};
