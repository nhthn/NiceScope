#include "portaudio_backend.hpp"

PortAudioBackend::PortAudioBackend(AudioCallback* callback, std::string device, int numChannels)
    : m_callback(callback)
    , m_device(device)
    , m_numChannels(numChannels)
    , sample_format(paFloat32)
{
}

void PortAudioBackend::run()
{
    handle_error(Pa_Initialize());

    int device = find_device();
    auto device_info = Pa_GetDeviceInfo(device);

    input_parameters.device = device;
    input_parameters.channelCount = m_numChannels;
    input_parameters.sampleFormat = sample_format;
    input_parameters.suggestedLatency = device_info->defaultLowInputLatency;
    input_parameters.hostApiSpecificStreamInfo = nullptr;

    PaStreamFlags stream_flags = paNoFlag;
    void* user_data = this;

    handle_error(
        Pa_OpenStream(
            &m_stream,
            &input_parameters,
            nullptr,
            m_sample_rate,
            m_block_size,
            stream_flags,
            stream_callback,
            user_data));

    handle_error(Pa_StartStream(m_stream));
}

void PortAudioBackend::end()
{
    handle_error(Pa_StopStream(m_stream));
    handle_error(Pa_CloseStream(m_stream));
    handle_error(Pa_Terminate());
}

void PortAudioBackend::process(InputBuffer input_buffer, OutputBuffer output_buffer, int numFrames)
{
    m_callback->process(input_buffer, output_buffer, numFrames);
}

void PortAudioBackend::handle_error(PaError error)
{
    if (error == paNoError) {
        return;
    }
    std::cerr << "PortAudio error: " << Pa_GetErrorText(error) << ", exiting :(" << std::endl;
    throw std::runtime_error("PortAudio error");
}

int PortAudioBackend::find_device()
{
    PaHostApiIndex host_api_index = Pa_HostApiTypeIdToHostApiIndex(paJACK);
    if (host_api_index < 0) {
#if (__APPLE__)
        std::cerr << "JACK not found, trying CoreAudio" << std::endl;
        host_api_index = Pa_HostApiTypeIdToHostApiIndex(paCoreAudio);
        if (host_api_index < 0) {
            return Pa_GetDefaultInputDevice();
        }
#else
        std::cerr << "JACK not found, using default device" << std::endl;
        return Pa_GetDefaultInputDevice();
#endif
    }


    const PaHostApiInfo* host_api_info = Pa_GetHostApiInfo(host_api_index);

    int device_count = host_api_info->deviceCount;
    for (int i = 0; i < device_count; i++) {
        int device_index = Pa_HostApiDeviceIndexToDeviceIndex(host_api_index, i);
        const PaDeviceInfo* info = Pa_GetDeviceInfo(device_index);
        std::string name = info->name;
        if (name == m_device) {
            return device_index;
        }
    }

    std::cerr << "device named " << m_device << " not found, trying default device." << std::endl;
    auto device_index = host_api_info->defaultInputDevice;
    if (device_index != paNoDevice) {
        return device_index;
    }

    throw std::runtime_error("Couldn't find device.");
}

int PortAudioBackend::stream_callback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    PortAudioBackend* backend = static_cast<PortAudioBackend*>(userData);
    backend->process(
        static_cast<InputBuffer>(inputBuffer),
        static_cast<OutputBuffer>(outputBuffer),
        frameCount);
    return 0;
}
