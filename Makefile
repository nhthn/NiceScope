all:
	g++ -g portaudio_backend.cpp main.cpp audio_callback.cpp -o gl-base -lGL -lGLU -lGLEW -lglfw -lportaudio
