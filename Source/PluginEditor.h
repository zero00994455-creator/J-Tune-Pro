#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class JTuneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer {
public:
    explicit JTuneAudioProcessorEditor(JTuneAudioProcessor&);
    ~JTuneAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    JTuneAudioProcessor& audioProcessor;

    juce::ComboBox keyBox, scaleBox, latencyBox;
    juce::Slider retuneSlider, flexSlider, humanizeSlider, formantsSlider, vibratoSlider, outputSlider;
    juce::TextButton hardLockBtn { "HARD LOCK: MAX" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    ComboBoxAttachment keyAtt, scaleAtt, latencyAtt;
    SliderAttachment retuneAtt, flexAtt, humanizeAtt, formantsAtt, vibratoAtt, outputAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JTuneAudioProcessorEditor)
};
