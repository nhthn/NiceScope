cmake_minimum_required(VERSION 3.19)

project(NiceScope)

add_subdirectory(third_party/portaudio)
include_directories(
    third_party/portaudio/include/
    third_party/portaudio/src/common/
)

file(GLOB nicescope_files src/*.cpp)
add_executable(NiceScope ${nicescope_files})


target_link_libraries(
    NiceScope
    "GL"
    GLU
    GLEW
    glfw
    portaudio_static
    fftw3
)
