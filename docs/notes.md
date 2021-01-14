In the high frequencies, there are usually several FFT bins per pixel. This causes a lot of CPU/GPU usage to draw for very small features, and makes the curves look noisy and unattractive.

We propose a scheme that displays less data, but looks nicer. At high frequencies, group the frequencies together that belong to chunks of n pixels. (n is small, usually only 2 or 3, and increase that for HiDPI displays.) Each chunk displays only the highest amplitude of the bins it contains.

Let W be the width of the scope in pixels. Let `position` denote the function mapping frequency to X position from 0 to 1. Then this function gives us the nominal chunk index of a given frequency:

    frequency_to_nominal_chunk_index(f) = floor(position(f) / (n / W))

I write "nominal" here because there will be many chunk indices in the low range that have zero associated frequencies. Removing these will give you the real chunk index.

Let `i_c` be the first chunk index such that there exist two bins with frequencies `f_1 != f_2` such that `frequency_to_chunk(f_1) = frequency_to_chunk(f_2)`. Let `f_c` be the lowest frequency associated with `i_c`. Then if `i >= i_c`:

    chunk_to_position(i) = position(f_c) + (i - i_c + 0.5) * (n / W)

0.5 is added so that the position of the chunk plot point is in the center of the chunk.

If `i < i_c`, the position of the chunk is merely the position of the unique frequency associated with that chunk.

Putting these all together, you should get a static array called `bin_to_chunk` that maps bins to their associated chunks, and an array called `chunk_position` indicating the X-position of the chunk's plot point. Given a magnitude spectrum, the Y-positions `chunk_values` are set like so:

    for bin_ in range(num_bins):
        chunk = bin_to_chunk[bin_]
        chunk_values[chunk] = max(chunk_values[chunk], spectrum[bin_])

The chunking scheme is now completely described and is agnostic to your choice of curve, provided that all chunks above i_c have two or more bins (which is an implication of a monotonically nonincreasing first derivative of `position`).
