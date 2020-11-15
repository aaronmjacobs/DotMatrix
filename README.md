# Dot Matrix

![Dot Matrix](https://user-images.githubusercontent.com/1409522/94489050-a764f400-0198-11eb-89d4-d2d207b424e7.png)

Dot Matrix is a Game Boy emulator written in C++.

## Features

![Dear Imgui](https://user-images.githubusercontent.com/1409522/94495370-6b389000-01a6-11eb-901b-50a452f2cc3e.png)

Dot Matrix performs audio emulation, supports a number of cartridge types (ROM, MBC1, MBC2, MBC3, MBC5), and has a [Dear Imgui](https://github.com/ocornut/imgui) powered UI.

The UI allows real-time modification of registers / memory, has visualizations for the LCD and sound controllers (inlcuding pitches for the two square wave channels), and has a debugger that supports loading symbol files.

### Accuracy

Dot Matrix is capable of playing most Game Boy games. It passes all of [Blargg's tests](Test/Results/blargg.csv) and over half of [Gekkio's mooneye-gb tests](Test/Results/mooneye.csv).

## Building

### Platforms

Windows, macOS, and Linux are supported.

### Build System

Dot Matrix uses [CMake](https://cmake.org/) as a meta build system. Run CMake (with the provided [Build](Build/) directory) to generate project files for your compiler / IDE of choice.

### Options

The following CMake options are provided:

* `DOT_MATRIX_WITH_AUDIO` - Whether to compile support for audio playback
* `DOT_MATRIX_WITH_BOOTSTRAP` - Whether to compile support for loading the Game Boy bootstrap ROM (not provided in the repo)
* `DOT_MATRIX_WITH_DEBUGGER` - Whether to compile the Game Boy debugger (breakpoints, CPU stepping, etc.) - decreases emulation performance
* `DOT_MATRIX_WITH_UI` - Whether to compile in the Dear ImGui UI

### Targets

There are three build targets:

1. `DotMatrix` - The standalone emulator executable
2. `DotMatrixRetro` - A [Libretro](https://www.libretro.com/) core (dynamically loaded library)
3. `DotMatrixTest` - An executable that runs test roms

### Dependencies

Dot Matrix comes will all necessary dependencies (except for GTK+ 3 on Linux) either as raw source files or git submodules. Run `git submodule init` and `git submodule update` to grab the submodules. If compiling on Linux, GTK+ 3 should be obtained via your package manager.
