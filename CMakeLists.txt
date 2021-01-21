cmake_minimum_required(VERSION 3.19)

project(NiceScope)

file(GLOB nicescope_files src/*.cpp)

add_executable(NiceScope ${nicescope_files})

target_link_libraries(
    NiceScope
    "GL"
    GLU
    GLEW
    glfw
    portaudio
    fftw3
)
