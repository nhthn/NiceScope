cmake_minimum_required(VERSION 3.19)

project(NiceScope)

add_subdirectory(third_party/portaudio)
include_directories(
    third_party/portaudio/include/
    third_party/portaudio/src/common/
)

if(APPLE)
    SET(CMAKE_CXX_STANDARD 11)
    include_directories(/usr/local/include)
endif()

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

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wpedantic")
endif()
