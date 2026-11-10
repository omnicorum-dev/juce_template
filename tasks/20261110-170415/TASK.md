# Better delay interpolation

- STATUS: OPEN
- PRIORITY: 100
- TAGS: verify, delay, dsp

Add cubic/Hermite, Lagrange, and Thiran allpass
reads to RingBuffer.

These are needed for chorus, flanger, and vibrato
where linear interpolation dulls the high end
