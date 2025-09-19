cmake_minimum_required(VERSION 3.15)

project(EQIsolator4 VERSION 1.0.0)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE_PATH should be defined when running CMake
if(NOT DEFINED JUCE_PATH)
    set(JUCE_PATH "C:/audio-plugins-dev/tools/JUCE" CACHE PATH "Path to JUCE")
    message(STATUS "JUCE_PATH defaulted to: ${JUCE_PATH}")
endif()

# Add JUCE as a subdirectory
if(NOT EXISTS "${JUCE_PATH}")
    message(FATAL_ERROR "JUCE not found at ${JUCE_PATH}")
endif()

add_subdirectory(${JUCE_PATH} JUCE)

# No need to explicitly add modules as they are already included by JUCE

# Plugin specifications
juce_add_plugin(EQIsolator4
    COMPANY_NAME "EQMixerPro"
    PLUGIN_MANUFACTURER_CODE "EQMP"
    PLUGIN_CODE "EQI4"
    FORMATS VST3
    PRODUCT_NAME "EQIsolator4"
    COMPANY_WEBSITE "www.example.com"
    COMPANY_EMAIL "info@example.com"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Fx" "EQ"
)

# Source files for the plugin
target_sources(EQIsolator4
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

# Plugin include directories
target_include_directories(EQIsolator4
    PRIVATE
        Source
)

# Link to JUCE modules
target_link_libraries(EQIsolator4
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Add definitions to prevent VST2/VST3 conflicts
target_compile_definitions(EQIsolator4
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Set binary output directory
set_target_properties(EQIsolator4 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/VST3"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/VST3"
)