# Latency Handling and Good Bypass

- STATUS: OPEN
- PRIORITY: 100
- TAGS: verify, template, dsp

STFT has latency, but nothing calls setLatencySamples().
Mix knob blends processed signal with dry signal
that isn't delay compensated, so it comb filters.

Add a "dry path" helper to fix all these issues.
- Delay matched to latency
- Crossfade on bypass
- Equal-power mix option
