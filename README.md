## Scope

**This project is in an early stage of development. Use at your own risk.**

This is a WIP attempt to create a minimal, beautiful, and configurable scope for the open source pro audio ecosystem.

### Building and running

Obtain dependencies.

    mkdir build
    cd build
    cmake ..
    ./NiceScope

### Dependencies

- CMake
- OpenGL
- GLU
- GLEW
- GLFW
- PortAudio
- FFTW

Debian:

    sudo apt install cmake libglu-dev libglew-dev libglfw3-dev libportaudio2 libfftw3-dev

Arch:

    # Replace glfw-x11 with glfw-wayland if on Wayland
    sudo pacman -S cmake glu glew glfw-x11 portaudio fftw

