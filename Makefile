

all:
	g++ -g src/*.cpp -o scope -lGL -lGLU -lGLEW -lglfw -lportaudio -lfftw3
