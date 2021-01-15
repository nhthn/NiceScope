## Visualizer

This is a WIP attempt to create a minimal, beautiful, and highly configurable scope for the Linux pro audio ecosystem.

Done:

- "Chunking" of spectrum plot points when plot points are denser than pixels.
- Logarithmic plot points.
- Basic PortAudio + OpenGL app working. PortAudio implies compatibility with ALSA, JACK, and PulseAudio.
- Cubic interpolation on the CPU.
- Constant-thickness spline.
- Hann window.
- History.

Todo:

- Configuration file.
- Investigate whether cubic interpolation in vertex shader is a good idea.

Goals:

- Logarithmic FFT spectrum plotter that looks at least as good as Fab-Filter Pro-Q and offerings from Ableton and Image Line.
  - Stereo plot.
  - Smart adaptation to dynamic range.
- Pretty oscilloscope with adaptive period maybe?
- Spectrogram plot?
- Text-based configuration, no fancy-pants GUI.
