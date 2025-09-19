# EQIsolator3

A 3-band EQ isolator VST3 plugin built with JUCE and CMake.

<img width="580" height="330" alt="EQIsolator4" src="https://github.com/user-attachments/assets/25b6a70d-8ea2-4daa-9f78-3b00ff78a315" />

## Description

EQIsolator3 is a 3-band equalizer isolator that lets you control the gain of low, mid, and high frequency bands with individual bypass options for each band.

### Features
- 3-band EQ isolation (Low, Mid, High)
- Per-band gain control (-100 dB to +24 dB)
- Per-band bypass options
- Minimal, easy-to-use interface

## Requirements

- Visual Studio 2019 or higher with C++ desktop development tools
- CMake 3.15 or higher
- JUCE 6.0 or higher

## Building the Project

### Using Visual Studio Code

1. Open the project folder in VS Code
2. Make sure the JUCE_PATH in .vscode/settings.json points to your JUCE installation
3. Press Ctrl+Shift+P and select "CMake: Configure"
4. Press Ctrl+Shift+P and select "CMake: Build"

### Using Command Line

```powershell
# Navigate to the project directory
cd EQIsolator3

# Configure CMake
cmake -B build -G "Visual Studio 16 2019" -DJUCE_PATH=C:/path/to/your/JUCE

# Build the project
cmake --build build --config Release
```

## Installation

After building, the VST3 plugin will be located in:
- Debug build: `build/Debug/VST3/EQIsolator3.vst3`
- Release build: `build/Release/VST3/EQIsolator3.vst3`

Copy the .vst3 file to your VST3 plugins directory:
- Windows: `C:\Program Files\Common Files\VST3`

## Quick test (no build needed)

If you just want to try the plugin without compiling:

- Use the prebuilt static bundle located at `dist/EQIsolator4.vst3` (created by running `static_build.bat`).
- Copy the `EQIsolator4.vst3` folder to `C:\Program Files\Common Files\VST3`.
- Open your DAW (Ableton Live 10/11/12, etc.) and rescan VST3 plugins.
- The static build is 64-bit and does not require JUCE or the Visual C++ Redistributable on the target machine.

## Usage

1. Load the plugin in your favorite DAW (like Ableton Live)
2. Adjust the low, mid, and high frequency band gains using the sliders
3. Use the bypass buttons to bypass individual frequency bands
4. The frequency crossover points are fixed at:
   - Low-Mid: 300 Hz
   - Mid-High: 3000 Hz

## Project Structure

```
EQIsolator3/
├── .vscode/               # VS Code configuration files
├── Source/                # Source code files
│   ├── PluginProcessor.h  # Audio processor header
│   ├── PluginProcessor.cpp# Audio processor implementation
│   ├── PluginEditor.h     # Editor component header
│   └── PluginEditor.cpp   # Editor component implementation
└── CMakeLists.txt         # CMake build configuration
```
