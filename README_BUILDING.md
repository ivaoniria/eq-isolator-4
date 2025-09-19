# Building EQIsolator3 - Troubleshooting Guide

If you're having issues building the EQIsolator3 plugin, this guide will help you set up your environment correctly.

## Prerequisites

1. **Install CMake**
   - Download and install CMake from [https://cmake.org/download/](https://cmake.org/download/)
   - Make sure to add CMake to your system PATH during installation

2. **Install Visual Studio with C++ Support**
   - Install Visual Studio 2019 or Visual Studio 2022
   - Make sure to include the "Desktop development with C++" workload
   - This includes the necessary C++ compilers and build tools

3. **Verify JUCE Installation**
   - Make sure JUCE is installed at `C:\audio-plugins-dev\tools\JUCE`
   - If installed elsewhere, update the path in:
     - CMakeLists.txt
     - .vscode/settings.json
     - .vscode/tasks.json
     - build.bat

## Building Options

### Option 1: Using the build.bat Script (Recommended)

1. Navigate to the EQIsolator3 folder
2. Double-click the `build.bat` file
3. The script will:
   - Check for CMake and Visual Studio
   - Set up the build environment
   - Configure and build the project

### Option 2: Using Visual Studio Code

1. Make sure CMake Tools extension is installed in VS Code
2. Open the EQIsolator3 folder in VS Code
3. Select the proper kit (Visual Studio 2019 or 2022)
4. Click on the "Build" button in the status bar

### Option 3: Using Command Prompt Manually

```powershell
# Navigate to the project directory
cd c:\audio-plugins-dev\proyects\EQMixerPro\EQIsolator3

# Configure CMake
cmake -B build -DJUCE_PATH=C:/audio-plugins-dev/tools/JUCE -G "Visual Studio 16 2019"

# Build the project
cmake --build build --config Debug
```

## Common Issues and Solutions

### CMake not found
- Make sure CMake is installed and added to your PATH
- You may need to restart your computer after installation

### Visual Studio Build Tools not found
- Make sure you've installed the "Desktop development with C++" workload
- For VS 2019: In the installer, select "Desktop development with C++"
- For VS 2022: Same process as above

### JUCE modules not found
- Verify the JUCE_PATH is pointing to the correct location
- Check if the modules directory exists in your JUCE installation

### Build errors related to missing headers
- This usually indicates that JUCE is not properly found
- Double-check the JUCE_PATH in all configuration files

## After Building

Once built successfully, the VST3 plugin will be located at:
```
c:\audio-plugins-dev\proyects\EQMixerPro\EQIsolator3\build\Debug\VST3\EQIsolator3.vst3
```

You can copy this file to your VST3 plugins directory or load it directly in your DAW for testing.