all:
	g++ -g portaudio_backend.cpp main.cpp -o gl-base -lGL -lGLU -lGLEW -lglfw -lportaudio
