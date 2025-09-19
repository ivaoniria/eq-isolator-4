/*
 EQIsolator4 - A transparent 4-band equalizer VST3 plugin
 Copyright (C) 2025 ivaoniria
 Licensed under GPL v3: https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
EQIsolator4AudioProcessor::EQIsolator4AudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    static const juce::NormalisableRange<float> gainRange(-100.0f, 24.0f, 0.1f);
    static const auto gainStringConverter = [](float value, int) -> juce::String 
    { 
        return juce::String(value, 1) + " dB"; 
    };
    
    addParameter(lowGainParam = new juce::AudioParameterFloat(
        LOW_GAIN_ID, "Low Gain (20-200Hz)", gainRange, 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter, gainStringConverter));
                                                           
    addParameter(lowMidGainParam = new juce::AudioParameterFloat(
        LOWMID_GAIN_ID, "Low-Mid Gain (200-750Hz)", gainRange, 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter, gainStringConverter));
                                                           
    addParameter(midGainParam = new juce::AudioParameterFloat(
        MID_GAIN_ID, "Mid Gain (750Hz-3kHz)", gainRange, 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter, gainStringConverter));
                                                           
    addParameter(highGainParam = new juce::AudioParameterFloat(
        HIGH_GAIN_ID, "High Gain (3-20kHz)", gainRange, 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter, gainStringConverter));
                                                            
    addParameter(lowBypassParam = new juce::AudioParameterBool(LOW_BYPASS_ID, "Low Band Bypass", false));
    addParameter(lowMidBypassParam = new juce::AudioParameterBool(LOWMID_BYPASS_ID, "Low-Mid Band Bypass", false));
    addParameter(midBypassParam = new juce::AudioParameterBool(MID_BYPASS_ID, "Mid Band Bypass", false));
    addParameter(highBypassParam = new juce::AudioParameterBool(HIGH_BYPASS_ID, "High Band Bypass", false));
    
    
    preallocatedBandBuffers.reserve(NUM_BANDS * MAX_CHANNELS);
}

EQIsolator4AudioProcessor::~EQIsolator4AudioProcessor()
{
}

//==============================================================================
const juce::String EQIsolator4AudioProcessor::getName() const
{
    return "EQIsolator4";
}

bool EQIsolator4AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EQIsolator4AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EQIsolator4AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EQIsolator4AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EQIsolator4AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EQIsolator4AudioProcessor::getCurrentProgram()
{
    return 0;
}

void EQIsolator4AudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String EQIsolator4AudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void EQIsolator4AudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void EQIsolator4AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Processing specs
    processSpec.sampleRate = sampleRate;
    processSpec.maximumBlockSize = samplesPerBlock;
    processSpec.numChannels = getTotalNumOutputChannels();
    
    // Parameter smoothing (ramp times)
    const float rampTimeMsLow    = 160.0f;  // Low band (more smoothing to avoid zipper noise)
    const float rampTimeMsLowMid = 15.0f;
    const float rampTimeMsMid    = 12.0f;
    const float rampTimeMsHigh   = 10.0f;
    
    smoothedLowGain.reset(sampleRate, rampTimeMsLow / 1000.0f);
    smoothedLowMidGain.reset(sampleRate, rampTimeMsLowMid / 1000.0f);
    smoothedMidGain.reset(sampleRate, rampTimeMsMid / 1000.0f);
    smoothedHighGain.reset(sampleRate, rampTimeMsHigh / 1000.0f);
    
    // Initial values without jumps (dB domain -> seed with current params)
    smoothedLowGain.setCurrentAndTargetValue(lowGainParam->get());
    smoothedLowMidGain.setCurrentAndTargetValue(lowMidGainParam->get());
    smoothedMidGain.setCurrentAndTargetValue(midGainParam->get());
    smoothedHighGain.setCurrentAndTargetValue(highGainParam->get());
    lastLowGainTargetDb = lowGainParam->get();
    
    // Filter frequency smoothing (static crossovers)
    smoothedLowCutoff.reset(sampleRate, rampTimeMsLow / 1000.0f);
    smoothedLowMidCutoff.reset(sampleRate, rampTimeMsLowMid / 1000.0f);
    smoothedMidCutoff.reset(sampleRate, rampTimeMsMid / 1000.0f);
    
    smoothedLowCutoff.setCurrentAndTargetValue(LOW_LOWMID_CROSSOVER_FREQ);
    smoothedLowMidCutoff.setCurrentAndTargetValue(LOWMID_MID_CROSSOVER_FREQ);
    smoothedMidCutoff.setCurrentAndTargetValue(MID_HIGH_CROSSOVER_FREQ);
    
    // Bypass smoothing (per-band)
    const float bypassRampTimeMsLow    = 80.0f; // longer to avoid low-band pops
    const float bypassRampTimeMsLowMid = 50.0f;
    const float bypassRampTimeMsMid    = 40.0f;
    const float bypassRampTimeMsHigh   = 25.0f;
    
    smoothedLowBypass.reset(sampleRate, bypassRampTimeMsLow / 1000.0f);
    smoothedLowMidBypass.reset(sampleRate, bypassRampTimeMsLowMid / 1000.0f);
    smoothedMidBypass.reset(sampleRate, bypassRampTimeMsMid / 1000.0f);
    smoothedHighBypass.reset(sampleRate, bypassRampTimeMsHigh / 1000.0f);
    
    // Set initial bypass values (1.0 = no bypass, 0.0 = bypass activo)
    smoothedLowBypass.setCurrentAndTargetValue(lowBypassParam->get() ? 0.0f : 1.0f);
    smoothedLowMidBypass.setCurrentAndTargetValue(lowMidBypassParam->get() ? 0.0f : 1.0f);
    smoothedMidBypass.setCurrentAndTargetValue(midBypassParam->get() ? 0.0f : 1.0f);
    smoothedHighBypass.setCurrentAndTargetValue(highBypassParam->get() ? 0.0f : 1.0f);
    
    // Prepare filters for processing
    prepareFilters(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    updateFilters();

    // Allocate temp buffers per channel
    const int numCh = getTotalNumOutputChannels();
    lowTempBuffers.resize(numCh);
    lowMidTempBuffers.resize(numCh);
    midTempBuffers.resize(numCh);
    highTempBuffers.resize(numCh);
    for (int c = 0; c < numCh; ++c)
    {
        lowTempBuffers[c].setSize(1, samplesPerBlock);
        lowMidTempBuffers[c].setSize(1, samplesPerBlock);
        midTempBuffers[c].setSize(1, samplesPerBlock);
        highTempBuffers[c].setSize(1, samplesPerBlock);
    }

    // Preallocate control curves to maximum block size
    lowGainCurve.assign(samplesPerBlock, 1.0f);
    lowMidGainCurve.assign(samplesPerBlock, 1.0f);
    midGainCurve.assign(samplesPerBlock, 1.0f);
    highGainCurve.assign(samplesPerBlock, 1.0f);
    lowBypassCurve.assign(samplesPerBlock, 1.0f);
    lowMidBypassCurve.assign(samplesPerBlock, 1.0f);
    midBypassCurve.assign(samplesPerBlock, 1.0f);
    highBypassCurve.assign(samplesPerBlock, 1.0f);

    // Initialize DC blocker for low band (simple 1st-order high-pass at ~5 Hz)
    // r = exp(-2*pi*fc/fs)
    const double fc = 5.0;
    dcBlockerR = (float) std::exp(-2.0 * juce::MathConstants<double>::pi * fc / sampleRate);
    dcPrevXLow.assign(numCh, 0.0f);
    dcPrevYLow.assign(numCh, 0.0f);
}

void EQIsolator4AudioProcessor::prepareFilters(double sampleRate, int samplesPerBlock, int numChannels)
{
    // Initialize filters for each channel and 4 bands
    lowPassFilters.clear();
    lowMidFilters.clear();
    midFilters.clear();
    highPassFilters.clear();
    
    for (int i = 0; i < numChannels; ++i)
    {
        // Create filters for each channel and each of the 4 bands
        lowPassFilters.push_back(ProcessorChain());
        lowMidFilters.push_back(ProcessorChain());
        midFilters.push_back(ProcessorChain());
        highPassFilters.push_back(ProcessorChain());
        
        // Prepare each filter chain
        lowPassFilters[i].prepare(processSpec);
        lowMidFilters[i].prepare(processSpec);
        midFilters[i].prepare(processSpec);
        highPassFilters[i].prepare(processSpec);
    }
}

void EQIsolator4AudioProcessor::updateFilters()
{
    // Filter update with coefficient caching
    
    const auto sampleRate = processSpec.sampleRate;
    
    static thread_local double lastSampleRate = 0.0;
    static thread_local auto cachedLowPassCoeff = 
        juce::dsp::IIR::Coefficients<float>::makeLowPass(44100.0, LOW_LOWMID_CROSSOVER_FREQ);
    static thread_local auto cachedLowMidHighPassCoeff = 
        juce::dsp::IIR::Coefficients<float>::makeHighPass(44100.0, LOW_LOWMID_CROSSOVER_FREQ);
    static thread_local auto cachedLowMidLowPassCoeff = 
        juce::dsp::IIR::Coefficients<float>::makeLowPass(44100.0, LOWMID_MID_CROSSOVER_FREQ);
    static thread_local auto cachedMidHighPassCoeff = 
        juce::dsp::IIR::Coefficients<float>::makeHighPass(44100.0, LOWMID_MID_CROSSOVER_FREQ);
    static thread_local auto cachedMidLowPassCoeff = 
        juce::dsp::IIR::Coefficients<float>::makeLowPass(44100.0, MID_HIGH_CROSSOVER_FREQ);
    static thread_local auto cachedHighPassCoeff = 
        juce::dsp::IIR::Coefficients<float>::makeHighPass(44100.0, MID_HIGH_CROSSOVER_FREQ);
    
    // Only recalculate coefficients if sample rate changed
    if (sampleRate != lastSampleRate)
    {
        lastSampleRate = sampleRate;
        
        // Recalculate all coefficients for new sample rate
        cachedLowPassCoeff = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, LOW_LOWMID_CROSSOVER_FREQ);
        cachedLowMidHighPassCoeff = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, LOW_LOWMID_CROSSOVER_FREQ);
        cachedLowMidLowPassCoeff = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, LOWMID_MID_CROSSOVER_FREQ);
        cachedMidHighPassCoeff = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, LOWMID_MID_CROSSOVER_FREQ);
        cachedMidLowPassCoeff = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, MID_HIGH_CROSSOVER_FREQ);
        cachedHighPassCoeff = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, MID_HIGH_CROSSOVER_FREQ);
    }
    
    // Apply cached coefficients to all filter chains
    // Low band: 20 Hz - 200 Hz (Low Pass at 200Hz)
    for (auto& chain : lowPassFilters)
    {
        *chain.get<0>().coefficients = *cachedLowPassCoeff;
        *chain.get<1>().coefficients = *cachedLowPassCoeff; // Second-order for steeper cutoff
    }
    
    // Low-Mid band: 200 Hz - 750 Hz (High Pass at 200Hz + Low Pass at 750Hz)
    for (auto& chain : lowMidFilters)
    {
        *chain.get<0>().coefficients = *cachedLowMidHighPassCoeff;
        *chain.get<1>().coefficients = *cachedLowMidLowPassCoeff;
    }
    
    // Mid band: 750 Hz - 3000 Hz (High Pass at 750Hz + Low Pass at 3000Hz)
    for (auto& chain : midFilters)
    {
        *chain.get<0>().coefficients = *cachedMidHighPassCoeff;
        *chain.get<1>().coefficients = *cachedMidLowPassCoeff;
    }
    
    // High band: 3000 Hz - 20 kHz (High Pass at 3000Hz)
    for (auto& chain : highPassFilters)
    {
        *chain.get<0>().coefficients = *cachedHighPassCoeff;
        *chain.get<1>().coefficients = *cachedHighPassCoeff; // Second-order for steeper cutoff
    }
}

void EQIsolator4AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EQIsolator4AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EQIsolator4AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);
    
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Clear any output channels that don't contain input data
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);
    
    const float lowGain = lowGainParam->get();
    const float lowMidGain = lowMidGainParam->get();
    const float midGain = midGainParam->get();
    const float highGain = highGainParam->get();
    
    const bool lowBypass = lowBypassParam->get();
    const bool lowMidBypass = lowMidBypassParam->get();
    const bool midBypass = midBypassParam->get();
    const bool highBypass = highBypassParam->get();
    
    const bool allBandsAtZero = (lowGain == 0.0f && lowMidGain == 0.0f && 
                                midGain == 0.0f && highGain == 0.0f) &&
                               (!lowBypass && !lowMidBypass && !midBypass && !highBypass);
    
    if (allBandsAtZero)
    {
        // Perfect transparency - pass through unprocessed
        return;
    }
    
    // Smooth in dB domain (less sensitivity around 0 dB). No deadband to avoid under-tracking.
    smoothedLowGain.setTargetValue(lowGain);
    smoothedLowMidGain.setTargetValue(lowMidGain);
    smoothedMidGain.setTargetValue(midGain);
    smoothedHighGain.setTargetValue(highGain);
    
    // Bypass smoothing for all bands
    smoothedLowBypass.setTargetValue(lowBypass ? 0.0f : 1.0f);
    smoothedLowMidBypass.setTargetValue(lowMidBypass ? 0.0f : 1.0f);
    smoothedMidBypass.setTargetValue(midBypass ? 0.0f : 1.0f);
    smoothedHighBypass.setTargetValue(highBypass ? 0.0f : 1.0f);
    
    // No dynamic filter recalc in processBlock
    
    // Precompute per-sample control curves once (used for all channels)
    {
        auto ensureSize = [numSamples](std::vector<float>& v) { if ((int)v.size() < numSamples) v.resize(numSamples); };
        ensureSize(lowGainCurve); ensureSize(lowMidGainCurve); ensureSize(midGainCurve); ensureSize(highGainCurve);
        ensureSize(lowBypassCurve); ensureSize(lowMidBypassCurve); ensureSize(midBypassCurve); ensureSize(highBypassCurve);

        // Generate curves
        for (int i = 0; i < numSamples; ++i)
        {
            lowGainCurve[i]     = juce::Decibels::decibelsToGain(smoothedLowGain.getNextValue());
            lowMidGainCurve[i]  = juce::Decibels::decibelsToGain(smoothedLowMidGain.getNextValue());
            midGainCurve[i]     = juce::Decibels::decibelsToGain(smoothedMidGain.getNextValue());
            highGainCurve[i]    = juce::Decibels::decibelsToGain(smoothedHighGain.getNextValue());

            auto smoothStep = [](float x) noexcept { x = juce::jlimit(0.0f, 1.0f, x); return x * x * (3.0f - 2.0f * x); };
            lowBypassCurve[i]    = smoothStep(smoothedLowBypass.getNextValue());
            lowMidBypassCurve[i] = smoothStep(smoothedLowMidBypass.getNextValue());
            midBypassCurve[i]    = smoothStep(smoothedMidBypass.getNextValue());
            highBypassCurve[i]   = smoothStep(smoothedHighBypass.getNextValue());
        }
    }

    // Process each channel
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        // Use persistent temp buffers for each band
        juce::AudioBuffer<float>& lowBuffer = lowTempBuffers[channel];
        juce::AudioBuffer<float>& lowMidBuffer = lowMidTempBuffers[channel];
        juce::AudioBuffer<float>& midBuffer = midTempBuffers[channel];
        juce::AudioBuffer<float>& highBuffer = highTempBuffers[channel];
        if (lowBuffer.getNumSamples() < numSamples)
        {
            lowBuffer.setSize(1, numSamples, false, false, true);
            lowMidBuffer.setSize(1, numSamples, false, false, true);
            midBuffer.setSize(1, numSamples, false, false, true);
            highBuffer.setSize(1, numSamples, false, false, true);
        }
        
        // Copy input data to each band buffer
        lowBuffer.copyFrom(0, 0, buffer, channel, 0, numSamples);
        lowMidBuffer.copyFrom(0, 0, buffer, channel, 0, numSamples);
        midBuffer.copyFrom(0, 0, buffer, channel, 0, numSamples);
        highBuffer.copyFrom(0, 0, buffer, channel, 0, numSamples);
        
        // Get pointers for processing
        float* lowData = lowBuffer.getWritePointer(0);
        float* lowMidData = lowMidBuffer.getWritePointer(0);
        float* midData = midBuffer.getWritePointer(0);
        float* highData = highBuffer.getWritePointer(0);
        
        // Low band
        juce::dsp::AudioBlock<float> lowBlock(&lowData, 1, numSamples);
        lowPassFilters[channel].process(juce::dsp::ProcessContextReplacing<float>(lowBlock));
        // Apply lightweight DC blocker to low band to avoid pops on transitions
        {
            float prevX = dcPrevXLow[channel];
            float prevY = dcPrevYLow[channel];
            const float r = dcBlockerR;
            for (int i = 0; i < numSamples; ++i)
            {
                const float x = lowData[i];
                const float y = lowData[i] - prevX + r * prevY; // H(z) = 1 - z^-1 / 1 - r z^-1
                lowData[i] = y;
                prevX = x;
                prevY = y;
            }
            dcPrevXLow[channel] = prevX;
            dcPrevYLow[channel] = prevY;
        }
        
        // Low-Mid band
        juce::dsp::AudioBlock<float> lowMidBlock(&lowMidData, 1, numSamples);
        lowMidFilters[channel].process(juce::dsp::ProcessContextReplacing<float>(lowMidBlock));
        
        // Mid band
        juce::dsp::AudioBlock<float> midBlock(&midData, 1, numSamples);
        midFilters[channel].process(juce::dsp::ProcessContextReplacing<float>(midBlock));
        
        // High band  
        juce::dsp::AudioBlock<float> highBlock(&highData, 1, numSamples);
        highPassFilters[channel].process(juce::dsp::ProcessContextReplacing<float>(highBlock));
        float* channelData = buffer.getWritePointer(channel);
        
        for (int i = 0; i < numSamples; ++i)
        {
            channelData[i] = (lowData[i]     * lowGainCurve[i]    * lowBypassCurve[i]) +
                             (lowMidData[i]  * lowMidGainCurve[i] * lowMidBypassCurve[i]) +
                             (midData[i]     * midGainCurve[i]    * midBypassCurve[i]) +
                             (highData[i]    * highGainCurve[i]   * highBypassCurve[i]);
        }
    }
}

//==============================================================================
bool EQIsolator4AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EQIsolator4AudioProcessor::createEditor()
{
    return new EQIsolator4AudioProcessorEditor(*this);
}

//==============================================================================
void EQIsolator4AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Create an XML element to store parameter values
    juce::MemoryOutputStream stream(destData, true);
    
    // Create XML for storing parameters
    juce::ValueTree state("EQIsolator4Parameters");
    
    state.setProperty(LOW_GAIN_ID, lowGainParam->get(), nullptr);
    state.setProperty(LOWMID_GAIN_ID, lowMidGainParam->get(), nullptr);
    state.setProperty(MID_GAIN_ID, midGainParam->get(), nullptr);
    state.setProperty(HIGH_GAIN_ID, highGainParam->get(), nullptr);
    state.setProperty(LOW_BYPASS_ID, lowBypassParam->get(), nullptr);
    state.setProperty(LOWMID_BYPASS_ID, lowMidBypassParam->get(), nullptr);
    state.setProperty(MID_BYPASS_ID, midBypassParam->get(), nullptr);
    state.setProperty(HIGH_BYPASS_ID, highBypassParam->get(), nullptr);
    
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void EQIsolator4AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameters from XML
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        juce::ValueTree state = juce::ValueTree::fromXml(*xmlState);
        
        if (state.hasProperty(LOW_GAIN_ID))
            *lowGainParam = static_cast<float>(state.getProperty(LOW_GAIN_ID));
            
        if (state.hasProperty(LOWMID_GAIN_ID))
            *lowMidGainParam = static_cast<float>(state.getProperty(LOWMID_GAIN_ID));
            
        if (state.hasProperty(MID_GAIN_ID))
            *midGainParam = static_cast<float>(state.getProperty(MID_GAIN_ID));
            
        if (state.hasProperty(HIGH_GAIN_ID))
            *highGainParam = static_cast<float>(state.getProperty(HIGH_GAIN_ID));
            
        if (state.hasProperty(LOW_BYPASS_ID))
            *lowBypassParam = static_cast<bool>(state.getProperty(LOW_BYPASS_ID));
            
        if (state.hasProperty(LOWMID_BYPASS_ID))
            *lowMidBypassParam = static_cast<bool>(state.getProperty(LOWMID_BYPASS_ID));
            
        if (state.hasProperty(MID_BYPASS_ID))
            *midBypassParam = static_cast<bool>(state.getProperty(MID_BYPASS_ID));
            
        if (state.hasProperty(HIGH_BYPASS_ID))
            *highBypassParam = static_cast<bool>(state.getProperty(HIGH_BYPASS_ID));
    }
}

// Parameter access methods
float EQIsolator4AudioProcessor::getLowGain() const { return lowGainParam->get(); }
float EQIsolator4AudioProcessor::getLowMidGain() const { return lowMidGainParam->get(); }
float EQIsolator4AudioProcessor::getMidGain() const { return midGainParam->get(); }
float EQIsolator4AudioProcessor::getHighGain() const { return highGainParam->get(); }
bool EQIsolator4AudioProcessor::getLowBypass() const { return lowBypassParam->get(); }
bool EQIsolator4AudioProcessor::getLowMidBypass() const { return lowMidBypassParam->get(); }
bool EQIsolator4AudioProcessor::getMidBypass() const { return midBypassParam->get(); }
bool EQIsolator4AudioProcessor::getHighBypass() const { return highBypassParam->get(); }

//==============================================================================
// Performance helpers
//==============================================================================

inline void EQIsolator4AudioProcessor::updateCachedParameters() const noexcept
{
    const float lowGain = lowGainParam->get();
    const float lowMidGain = lowMidGainParam->get();
    const float midGain = midGainParam->get();
    const float highGain = highGainParam->get();
    
    // Only update if values actually changed (branch prediction friendly)
    if (lowGain != lastLowGain) {
        lastLowGain = lowGain;
        cachedLowGainLinear.store(juce::Decibels::decibelsToGain(lowGain), std::memory_order_relaxed);
    }
    if (lowMidGain != lastLowMidGain) {
        lastLowMidGain = lowMidGain;
        cachedLowMidGainLinear.store(juce::Decibels::decibelsToGain(lowMidGain), std::memory_order_relaxed);
    }
    if (midGain != lastMidGain) {
        lastMidGain = midGain;
        cachedMidGainLinear.store(juce::Decibels::decibelsToGain(midGain), std::memory_order_relaxed);
    }
    if (highGain != lastHighGain) {
        lastHighGain = highGain;
        cachedHighGainLinear.store(juce::Decibels::decibelsToGain(highGain), std::memory_order_relaxed);
    }
    
    // Check bypass changes
    const bool lowBypass = lowBypassParam->get();
    const bool lowMidBypass = lowMidBypassParam->get();
    const bool midBypass = midBypassParam->get();
    const bool highBypass = highBypassParam->get();
    
    if (lowBypass != lastLowBypass || lowMidBypass != lastLowMidBypass || 
        midBypass != lastMidBypass || highBypass != lastHighBypass) {
        lastLowBypass = lowBypass;
        lastLowMidBypass = lowMidBypass;
        lastMidBypass = midBypass;
        lastHighBypass = highBypass;
        filtersNeedUpdate.store(true, std::memory_order_relaxed);
    }
}

inline bool EQIsolator4AudioProcessor::checkParametersChanged() const noexcept
{
    // Parameter change detection
    return (lowGainParam->get() != lastLowGain ||
            lowMidGainParam->get() != lastLowMidGain ||
            midGainParam->get() != lastMidGain ||
            highGainParam->get() != lastHighGain ||
            lowBypassParam->get() != lastLowBypass ||
            lowMidBypassParam->get() != lastLowMidBypass ||
            midBypassParam->get() != lastMidBypass ||
            highBypassParam->get() != lastHighBypass);
}

void EQIsolator4AudioProcessor::mixBandsOptimized(float* const* channelData, 
                                                 const float* const* lowData,
                                                 const float* const* lowMidData,
                                                 const float* const* midData,
                                                 const float* const* highData,
                                                 int numChannels, int numSamples) const noexcept
{
    // Load cached gain values once
    const float lowGain = cachedLowGainLinear.load(std::memory_order_relaxed);
    const float lowMidGain = cachedLowMidGainLinear.load(std::memory_order_relaxed);
    const float midGain = cachedMidGainLinear.load(std::memory_order_relaxed);
    const float highGain = cachedHighGainLinear.load(std::memory_order_relaxed);
    
    // Vectorized mixing
    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* const output = channelData[channel];
        const float* const low = lowData[channel];
        const float* const lowMid = lowMidData[channel];
        const float* const mid = midData[channel];
        const float* const high = highData[channel];
        
        // Vectorized loop with manual unrolling for better performance
        int i = 0;
        const int vectorizedEnd = numSamples & ~3; // Process 4 samples at a time
        
        for (; i < vectorizedEnd; i += 4)
        {
            // Unrolled loop
            output[i + 0] = low[i + 0] * lowGain + lowMid[i + 0] * lowMidGain + 
                           mid[i + 0] * midGain + high[i + 0] * highGain;
            output[i + 1] = low[i + 1] * lowGain + lowMid[i + 1] * lowMidGain + 
                           mid[i + 1] * midGain + high[i + 1] * highGain;
            output[i + 2] = low[i + 2] * lowGain + lowMid[i + 2] * lowMidGain + 
                           mid[i + 2] * midGain + high[i + 2] * highGain;
            output[i + 3] = low[i + 3] * lowGain + lowMid[i + 3] * lowMidGain + 
                           mid[i + 3] * midGain + high[i + 3] * highGain;
        }
        
        // Handle remaining samples
        for (; i < numSamples; ++i)
        {
            output[i] = low[i] * lowGain + lowMid[i] * lowMidGain + 
                       mid[i] * midGain + high[i] * highGain;
        }
    }
}

//==============================================================================
// Smoothed filter updates (not used in processBlock)
//==============================================================================

void EQIsolator4AudioProcessor::updateFiltersSmooth(int numSamples)
{
    // Set target values for smooth interpolation
    smoothedLowCutoff.setTargetValue(LOW_LOWMID_CROSSOVER_FREQ);
    smoothedLowMidCutoff.setTargetValue(LOWMID_MID_CROSSOVER_FREQ);
    smoothedMidCutoff.setTargetValue(MID_HIGH_CROSSOVER_FREQ);
    
    const auto sampleRate = processSpec.sampleRate;
    
    const int chunkSize = 32;
    
    for (int startSample = 0; startSample < numSamples; startSample += chunkSize)
    {
        const int samplesToProcess = juce::jmin(chunkSize, numSamples - startSample);
        juce::ignoreUnused(samplesToProcess);
        
        // Get current smoothed frequencies for this block
        const float currentLowFreq = smoothedLowCutoff.getNextValue();
        const float currentLowMidFreq = smoothedLowMidCutoff.getNextValue();
        const float currentMidFreq = smoothedMidCutoff.getNextValue();
        
        // Update coefficients only if frequencies changed significantly
        static float lastLowFreq = 0.0f;
        static float lastLowMidFreq = 0.0f;
        static float lastMidFreq = 0.0f;
        
    const float lowFreqThreshold = 5.0f;
    const float midFreqThreshold = 2.0f;
    const float highFreqThreshold = 1.0f;
        
        if (std::abs(currentLowFreq - lastLowFreq) > lowFreqThreshold ||
            std::abs(currentLowMidFreq - lastLowMidFreq) > midFreqThreshold ||
            std::abs(currentMidFreq - lastMidFreq) > highFreqThreshold)
        {
            // Update filter coefficients with smoothed frequencies
            auto lowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, currentLowFreq);
            auto lowMidHighPassCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, currentLowFreq);
            auto lowMidLowPassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, currentLowMidFreq);
            auto midHighPassCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, currentLowMidFreq);
            auto midLowPassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, currentMidFreq);
            auto highCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, currentMidFreq);
            
            // Apply to all filter chains
            for (auto& chain : lowPassFilters)
            {
                chain.get<0>().coefficients = lowCoeffs;
                chain.get<1>().coefficients = lowCoeffs;
            }
            
            for (auto& chain : lowMidFilters)
            {
                chain.get<0>().coefficients = lowMidHighPassCoeffs;
                chain.get<1>().coefficients = lowMidLowPassCoeffs;
            }
            
            for (auto& chain : midFilters)
            {
                chain.get<0>().coefficients = midHighPassCoeffs;
                chain.get<1>().coefficients = midLowPassCoeffs;
            }
            
            for (auto& chain : highPassFilters)
            {
                chain.get<0>().coefficients = highCoeffs;
                chain.get<1>().coefficients = highCoeffs;
            }
            
            lastLowFreq = currentLowFreq;
            lastLowMidFreq = currentLowMidFreq;
            lastMidFreq = currentMidFreq;
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EQIsolator4AudioProcessor();
}
