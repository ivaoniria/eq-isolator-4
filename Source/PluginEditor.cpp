/*
 EQIsolator4 - A transparent 4-band equalizer VST3 plugin
 Copyright (C) 2025 ivaoniria
 Licensed under GPL v3: https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EQIsolator4AudioProcessorEditor::EQIsolator4AudioProcessorEditor (EQIsolator4AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Set up title label with creator watermark
    titleLabel.setText("EQIsolator4", juce::dontSendNotification);
    titleLabel.setFont(juce::Font("Roboto", 18.0f, juce::Font::plain));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // 💎 PROTECTED CREATOR WATERMARK 💎
    watermarkLabel.setText(juce::String(audioProcessor.getCreatorWatermark()), juce::dontSendNotification);
    watermarkLabel.setFont(juce::Font("Roboto", 12.0f, juce::Font::italic));
    watermarkLabel.setJustificationType(juce::Justification::centredRight);
    watermarkLabel.setColour(juce::Label::textColourId, juce::Colours::grey.withAlpha(0.9f));
    addAndMakeVisible(watermarkLabel);
    
    // Set up band labels with frequency ranges
    lowLabel.setText("Low\n(20-200Hz)", juce::dontSendNotification);
    lowLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lowLabel);
    
    lowMidLabel.setText("Low-Mid\n(200-750Hz)", juce::dontSendNotification);
    lowMidLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lowMidLabel);
    
    midLabel.setText("Mid\n(750Hz-3kHz)", juce::dontSendNotification);
    midLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(midLabel);
    
    highLabel.setText("High\n(3-20kHz)", juce::dontSendNotification);
    highLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(highLabel);
    
    // Set up sliders using parameter ranges
    lowGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    lowGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible(lowGainSlider);
    
    lowMidGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    lowMidGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible(lowMidGainSlider);
    
    midGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    midGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible(midGainSlider);
    
    highGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    highGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible(highGainSlider);
    
    // Set up bypass buttons
    lowBypassButton.setButtonText("Bypass");
    addAndMakeVisible(lowBypassButton);
    
    lowMidBypassButton.setButtonText("Bypass");
    addAndMakeVisible(lowMidBypassButton);
    
    midBypassButton.setButtonText("Bypass");
    addAndMakeVisible(midBypassButton);
    
    highBypassButton.setButtonText("Bypass");
    addAndMakeVisible(highBypassButton);
    
    // Create parameter attachments for automatic synchronization
    lowGainAttachment = std::make_unique<juce::SliderParameterAttachment>(*audioProcessor.lowGainParam, lowGainSlider);
    lowMidGainAttachment = std::make_unique<juce::SliderParameterAttachment>(*audioProcessor.lowMidGainParam, lowMidGainSlider);
    midGainAttachment = std::make_unique<juce::SliderParameterAttachment>(*audioProcessor.midGainParam, midGainSlider);
    highGainAttachment = std::make_unique<juce::SliderParameterAttachment>(*audioProcessor.highGainParam, highGainSlider);
    
    lowBypassAttachment = std::make_unique<juce::ButtonParameterAttachment>(*audioProcessor.lowBypassParam, lowBypassButton);
    lowMidBypassAttachment = std::make_unique<juce::ButtonParameterAttachment>(*audioProcessor.lowMidBypassParam, lowMidBypassButton);
    midBypassAttachment = std::make_unique<juce::ButtonParameterAttachment>(*audioProcessor.midBypassParam, midBypassButton);
    highBypassAttachment = std::make_unique<juce::ButtonParameterAttachment>(*audioProcessor.highBypassParam, highBypassButton);
    
    // Set editor size for 4 bands
    setSize (580, 300);
}

EQIsolator4AudioProcessorEditor::~EQIsolator4AudioProcessorEditor()
{
}

//==============================================================================
void EQIsolator4AudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill the background
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    // Draw borders for each band section
    g.setColour(juce::Colours::grey);
    g.drawRect(10, 40, 135, 240);   // Low band
    g.drawRect(150, 40, 135, 240);  // Low-Mid band
    g.drawRect(290, 40, 135, 240);  // Mid band
    g.drawRect(430, 40, 135, 240);  // High band
}

void EQIsolator4AudioProcessorEditor::resized()
{
    // Layout components for 4 bands - TÍTULO PERFECTAMENTE CENTRADO
    titleLabel.setBounds(0, 5, getWidth(), 30);   // Más arriba, casi en el top
    
    // 💎 PROTECTED WATERMARK POSITIONING 💎
    watermarkLabel.setBounds(getWidth() - 120, getHeight() - 18, 115, 16);
    
    // Low band (20-200Hz)
    lowLabel.setBounds(10, 50, 135, 35);
    lowGainSlider.setBounds(25, 90, 105, 130);
    lowBypassButton.setBounds(30, 225, 95, 25);
    
    // Low-Mid band (200-750Hz)
    lowMidLabel.setBounds(150, 50, 135, 35);
    lowMidGainSlider.setBounds(165, 90, 105, 130);
    lowMidBypassButton.setBounds(170, 225, 95, 25);
    
    // Mid band (750Hz-3kHz)
    midLabel.setBounds(290, 50, 135, 35);
    midGainSlider.setBounds(305, 90, 105, 130);
    midBypassButton.setBounds(310, 225, 95, 25);
    
    // High band (3-20kHz)
    highLabel.setBounds(430, 50, 135, 35);
    highGainSlider.setBounds(445, 90, 105, 130);
    highBypassButton.setBounds(450, 225, 95, 25);
}

