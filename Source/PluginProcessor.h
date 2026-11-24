#pragma once
#include <JuceHeader.h>
#include "PitchDetector.h"
#include "ScaleQuantizer.h"

class JTuneAudioProcessor : public juce::AudioProcessor {
public:
    JTuneAudioProcessor();
    ~JTuneAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "J-Tune"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Init"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    ScaleQuantizer quantizer;

    std::atomic<float> currentCentsError { 0.0f };
    std::atomic<float> currentDetectedMidi { -1.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    PitchDetector pitchDetector;
    double currentSampleRate = 44100.0;
    double smoothedMidi = -1.0;
    double pitchRatio = 1.0;
    double phase = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JTuneAudioProcessor)
};