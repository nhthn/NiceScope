

all:
	g++ -std=c++14 -g src/*.cpp -o scope -lGL -lGLU -lGLEW -lglfw -lportaudio -lfftw3
