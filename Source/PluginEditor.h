/*
 EQIsolator4 - A transparent 4-band equalizer VST3 plugin
 Copyright (C) 2025 ivaoniria
 Licensed under GPL v3: https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * EQIsolator4 - Basic editor component
 * A minimal editor with sliders and toggles for the 4-band EQ
 */
class EQIsolator4AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    EQIsolator4AudioProcessorEditor(EQIsolator4AudioProcessor&);
    ~EQIsolator4AudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Reference to the processor to update parameters
    EQIsolator4AudioProcessor& audioProcessor;
    
    // GUI Components for 4 bands
    juce::Slider lowGainSlider, lowMidGainSlider, midGainSlider, highGainSlider;
    juce::ToggleButton lowBypassButton, lowMidBypassButton, midBypassButton, highBypassButton;
    
    // Labels
    juce::Label lowLabel, lowMidLabel, midLabel, highLabel;
    juce::Label titleLabel;
    juce::Label watermarkLabel; // 💎 Protected creator watermark 💎
    
    // Parameter attachments for automatic synchronization
    std::unique_ptr<juce::SliderParameterAttachment> lowGainAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> lowMidGainAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> midGainAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> highGainAttachment;
    std::unique_ptr<juce::ButtonParameterAttachment> lowBypassAttachment;
    std::unique_ptr<juce::ButtonParameterAttachment> lowMidBypassAttachment;
    std::unique_ptr<juce::ButtonParameterAttachment> midBypassAttachment;
    std::unique_ptr<juce::ButtonParameterAttachment> highBypassAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQIsolator4AudioProcessorEditor)
};