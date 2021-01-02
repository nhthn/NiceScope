## Visualizer

This is a WIP attempt to create a minimal, beautiful, and highly configurable scope for the Linux pro audio ecosystem.

Done:

- Basic PortAudio + OpenGL app working. PortAudio implies compatibility with ALSA, JACK, and PulseAudio.

Todo:

- Cubic interpolation on the CPU.
- Investigate whether cubic interpolation in vertex shader is a good idea.
- Constant-thickness spline.

Goals:

- Logarithmic FFT spectrum plotter that looks at least as good as Fab-Filter Pro-Q and offerings from Ableton and Image Line.
  - "Ghost" images left behind.
  - Stereo plot.
  - Smart adaptation to dynamic range.
- Pretty oscilloscope with adaptive period maybe?
- Spectrogram plot?
- Text-based configuration, no fancy-pants GUI.
