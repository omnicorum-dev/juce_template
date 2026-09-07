# JUCE Template

My own personal JUCE template, with everything set up and ready for basic DSP

## Setup

Change the project name and all `juce_add_plugin` fields.

Also, set the `JUCE_PATH` to wherever your JUCE clone is.
If you don't have a local JUCE install, it will fallback
to a FetchContent, so if you don't want multiple copies
of JUCE hanging around your computer, just set `JUCE_PATH`

To generate the compile commands used by clangd, make sure
to run the CMake configure step.

```sh
cmake -B build -G Ninja
```

## Building (on my machine, at least)

```sh
cmake --build build --config Debug
```

```sh
cmake --build build --config Release
```
