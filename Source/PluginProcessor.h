/*
 EQIsolator4 - A transparent 4-band equalizer VST3 plugin
 Copyright (C) 2025 ivaoniria
 Licensed under GPL v3: https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_utils/juce_audio_utils.h>

//==============================================================================
/**
 * EQIsolator4 - 4-band EQ Isolator plugin
 * Audio processor class for the EQIsolator4 VST3 plugin
 */
class EQIsolator4AudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    EQIsolator4AudioProcessor();
    ~EQIsolator4AudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // ULTRA-OPTIMIZED PERFORMANCE ENHANCEMENTS
    //==============================================================================
    
    // CREATOR WATERMARK ACCESS 
    static const char* getCreatorWatermark() noexcept 
    { 
        static const char watermark[] = {
            'c', 'r', 'e', 'a', 't', 'e', 'd', ' ', 'b', 'y', ' ', 
            'i', 'v', 'a', 'o', 'n', 'i', 'r', 'i', 'a', '\0'
        };
        return watermark;
    }

    // Parameter access methods for the editor
    float getLowGain() const;
    float getLowMidGain() const;
    float getMidGain() const;
    float getHighGain() const;
    bool getLowBypass() const;
    bool getLowMidBypass() const;
    bool getMidBypass() const;
    bool getHighBypass() const;

    //==============================================================================
    // Parameter IDs for 4 bands
    static constexpr const char* LOW_GAIN_ID = "low_gain";
    static constexpr const char* LOWMID_GAIN_ID = "lowmid_gain";
    static constexpr const char* MID_GAIN_ID = "mid_gain";
    static constexpr const char* HIGH_GAIN_ID = "high_gain";
    static constexpr const char* LOW_BYPASS_ID = "low_bypass";
    static constexpr const char* LOWMID_BYPASS_ID = "lowmid_bypass";
    static constexpr const char* MID_BYPASS_ID = "mid_bypass";
    static constexpr const char* HIGH_BYPASS_ID = "high_bypass";

    // Filter cutoff frequencies for 4-band EQ
    // Low: 20 Hz – ~200 Hz 
    // Low-Mid: ~200 Hz – ~700-800 Hz
    // Mid: ~700-800 Hz – ~2.5-3.5 kHz
    // High: ~2.5-3.5 kHz – ~15-20 kHz
    static constexpr float LOW_LOWMID_CROSSOVER_FREQ = 200.0f; // Hz
    static constexpr float LOWMID_MID_CROSSOVER_FREQ = 750.0f; // Hz  
    static constexpr float MID_HIGH_CROSSOVER_FREQ = 3000.0f; // Hz

    // Audio Parameters for 4 bands
    juce::AudioParameterFloat* lowGainParam;
    juce::AudioParameterFloat* lowMidGainParam;
    juce::AudioParameterFloat* midGainParam;
    juce::AudioParameterFloat* highGainParam;
    juce::AudioParameterBool* lowBypassParam;
    juce::AudioParameterBool* lowMidBypassParam;
    juce::AudioParameterBool* midBypassParam;
    juce::AudioParameterBool* highBypassParam;

private:

    // DSP Processing for 4 bands
    using Filter = juce::dsp::IIR::Filter<float>;
    using ProcessorChain = juce::dsp::ProcessorChain<Filter, Filter>;
    
    std::vector<ProcessorChain> lowPassFilters;    // Low band (20Hz - 200Hz)
    std::vector<ProcessorChain> lowMidFilters;     // Low-Mid band (200Hz - 750Hz)
    std::vector<ProcessorChain> midFilters;        // Mid band (750Hz - 3kHz)
    std::vector<ProcessorChain> highPassFilters;   // High band (3kHz - 20kHz)

    juce::dsp::ProcessSpec processSpec;

    // Persistent temp buffers per band/channel to avoid RT allocations
    std::vector<juce::AudioBuffer<float>> lowTempBuffers;
    std::vector<juce::AudioBuffer<float>> lowMidTempBuffers;
    std::vector<juce::AudioBuffer<float>> midTempBuffers;
    std::vector<juce::AudioBuffer<float>> highTempBuffers;

    // DC blocker state for low band (per channel)
    float dcBlockerR = 0.0f; // pole coefficient ~ exp(-2*pi*fc/fs) for fc ~ 5 Hz
    std::vector<float> dcPrevXLow; // previous input sample per channel
    std::vector<float> dcPrevYLow; // previous output sample per channel

    // Per-sample control curves (computed once, reused for all channels)
    std::vector<float> lowGainCurve;
    std::vector<float> lowMidGainCurve;
    std::vector<float> midGainCurve;
    std::vector<float> highGainCurve;
    std::vector<float> lowBypassCurve;
    std::vector<float> lowMidBypassCurve;
    std::vector<float> midBypassCurve;
    std::vector<float> highBypassCurve;

    // Prepare and update filters based on current parameters
    void prepareFilters(double sampleRate, int samplesPerBlock, int numChannels);
    void updateFilters();
    void updateFiltersSmooth(int numSamples);
    
    //==============================================================================
    // 🚀 ULTRA-OPTIMIZED PERFORMANCE CACHE SYSTEM 🚀
    //==============================================================================
    
    // Pre-allocated buffers to avoid dynamic allocation in processBlock()
    mutable std::vector<juce::AudioBuffer<float>> preallocatedBandBuffers;
    static constexpr int NUM_BANDS = 4;
    static constexpr int MAX_CHANNELS = 8; // Support up to 8 channels
    
    // Cached linear gain values (updated only when parameters change)
    mutable std::atomic<float> cachedLowGainLinear{1.0f};
    mutable std::atomic<float> cachedLowMidGainLinear{1.0f};
    mutable std::atomic<float> cachedMidGainLinear{1.0f};
    mutable std::atomic<float> cachedHighGainLinear{1.0f};
    
    // 🎵 PARAMETER SMOOTHING - ELIMINATES ALL AUDIO CLICKS 🎵
    juce::SmoothedValue<float> smoothedLowGain;
    juce::SmoothedValue<float> smoothedLowMidGain;
    juce::SmoothedValue<float> smoothedMidGain;
    juce::SmoothedValue<float> smoothedHighGain;
    
    // Filter coefficient smoothing to prevent filter resonance clicks
    mutable juce::SmoothedValue<float> smoothedLowCutoff;
    mutable juce::SmoothedValue<float> smoothedLowMidCutoff;
    mutable juce::SmoothedValue<float> smoothedMidCutoff;
    
    // Bypass smoothing to eliminate bypass clicks
    juce::SmoothedValue<float> smoothedLowBypass;
    juce::SmoothedValue<float> smoothedLowMidBypass;
    juce::SmoothedValue<float> smoothedMidBypass;
    juce::SmoothedValue<float> smoothedHighBypass;
    
    // Parameter change flags for smart updates
    mutable std::atomic<bool> parametersChanged{true};
    mutable std::atomic<bool> filtersNeedUpdate{true};
    
    // Previous parameter values for change detection
    mutable float lastLowGain = 0.0f;
    mutable float lastLowMidGain = 0.0f;
    mutable float lastMidGain = 0.0f;
    mutable float lastHighGain = 0.0f;
    mutable bool lastLowBypass = false;
    mutable bool lastLowMidBypass = false;
    mutable bool lastMidBypass = false;
    mutable bool lastHighBypass = false;
    
    // Ultra-fast parameter access with caching
    inline void updateCachedParameters() const noexcept;
    inline bool checkParametersChanged() const noexcept;
    
    // SIMD-optimized mixing function
    void mixBandsOptimized(float* const* channelData, 
                          const float* const* lowData,
                          const float* const* lowMidData, 
                          const float* const* midData,
                          const float* const* highData,
                          int numChannels, int numSamples) const noexcept;
                          
    // Memory alignment helpers for SIMD
    static constexpr size_t SIMD_ALIGNMENT = 16;

    // Low gain target deadband (dB domain) to suppress micro-steps
    float lastLowGainTargetDb = 0.0f;
    
    //==============================================================================
    // 💎 CREATOR WATERMARK - PROTECTED & IMMUTABLE 💎
    //==============================================================================
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQIsolator4AudioProcessor)
};