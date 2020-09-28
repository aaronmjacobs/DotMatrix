# Dot Matrix

![Dot Matrix](https://user-images.githubusercontent.com/1409522/94489050-a764f400-0198-11eb-89d4-d2d207b424e7.png)

Dot Matrix is a Game Boy emulator written in C++.

## Features

![Pokemon Yellow](https://user-images.githubusercontent.com/1409522/94495370-6b389000-01a6-11eb-901b-50a452f2cc3e.png)

Dot Matrix performs audio emulation, supports a number of cartridge types (ROM, MBC1, MBC2, MBC3, MBC5), and has a [Dear Imgui](https://github.com/ocornut/imgui) powered UI.

The UI allows real-time modification of registers / memory, has visualizations for the LCD and sound controllers (inlcuding pitches for the two square wave channels), and has a debugger that supports loading symbol files.

## Accuracy

Dot Matrix is capable of playing most Game Boy games. It passes all of [Blargg's tests](Test/Results/blargg.csv) and over half of [Gekkio's mooneye-gb tests](Test/Results/mooneye.csv).
