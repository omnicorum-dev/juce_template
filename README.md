# JUCE Template

My own personal JUCE template, with everything set up and ready for basic DSP

## Setup

Change the project name and all `juce_add_plugin` fields.

Also, set the `JUCE_PATH` to wherever your JUCE clone is.
If you don't have a local JUCE install, it will fallback
to a FetchContent, so if you don't want multiple copies
of JUCE hanging around your computer, just set `JUCE_PATH`

To generate the compile commands used by clangd, make sure
to run the CMake configure step. (I usually use Ninja.
XCode and MSVC won't generate the clang compile commands!)

Configure debug build:

```sh
cmake --preset debug 
```

Configure release build:

```sh
cmake --preset release
```

## Building (on my machine, at least)

Build debug build:

```sh
cmake --build --prest debug
```

Build release build:

```sh
cmake --build --preset release
```

## Releasing

A github workflow is built in. This workflow builds for
universal macOS, x86_64 Windows, and x86_64 Linux.
It doesn't run for every `git push`, so to have it run,

```sh
git tag v*.*.*
git push origin v*.*.*
```
