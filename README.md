# Kib 8

A CHIP-8 emulator written with SDl3 and ImGui in c++. A learning project before moving on to the NES. :)

<img width="512" height="306" alt="Tests Running" src="https://github.com/user-attachments/assets/718e6191-fa3b-4b30-ae1f-a4ae03e00578" />

## Features

- Load ROMs from the **File → Open** menu or by drag and drop
- Status bar with emulator state, adjustable speed (instructions per tick), pause and single-step

## Building

Requirements: CMake 3.20+, a C++20 compiler (GCC, Clang or MSVC). SDL3 and ImGui are included as git submodules and built from source.

```bash
git clone --recursive https://github.com/jonassimkus/kib8.git
cd kib8

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
Run with `./build/kib8` (Linux) or `build\Release\kib8.exe` (Windows).

## Controls

The original 4×4 hex keypad is mapped to the left side of the keyboard:

```
Keypad        Keyboard
1 2 3 C       1 2 3 4
4 5 6 D       Q W E R
7 8 9 E       A S D F
A 0 B F       Z X C V
```

## Refrence

- [Guide to making a CHIP-8 emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/) by Tobias V. Langhoff
- [CHIP-8 test suite](https://github.com/Timendus/chip8-test-suite) by Timendus
