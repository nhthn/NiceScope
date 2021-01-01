all:
	g++ -g portaudio_backend.cpp main.cpp -o scope -lGL -lGLU -lGLEW -lglfw -lportaudio
