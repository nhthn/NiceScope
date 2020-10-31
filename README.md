## Visualizer

This is an unfinished attempt to create a minimal, beautiful, and highly configurable scope for the Linux pro audio ecosystem. Drawing is done purely on the GPU with a fragment shader.

Done:

- Basic PortAudio + OpenGL app working. PortAudio implies compatibility with ALSA, JACK, and PulseAudio.
- Cubic spline implemented on GPU.

Todo:

- Attractive GPU-based curve drawing by computing distance from a cubic spline.
- Consider using UBO or texture to transfer point data.

Goals:

- Logarithmic FFT spectrum plotter that looks at least as good as Fab-Filter Pro-Q and offerings from Ableton and Image Line.
  - "Ghost" images left behind.
  - Stereo plot.
  - Smart adaptation to dynamic range.
- Pretty oscilloscope with adaptive period maybe?
- Spectrogram plot?
- Text-based configuration, no fancy-pants GUI.
