#include "PluginProcessor.h"
#include "PluginEditor.h"

JTuneAudioProcessor::JTuneAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()) {
}

juce::AudioProcessorValueTreeState::ParameterLayout JTuneAudioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "key", "Key",
        juce::StringArray{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 9));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "scale", "Scale",
        juce::StringArray{ "Major", "Major Pentatonic", "Minor", "Minor Pentatonic" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "retuneSpeed", "Retune Speed", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "flexTune", "Flex-Tune", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "humanize", "Humanize", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "formant", "Formant", juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "vibrato", "Vibrato", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "outputGain", "Output Gain", juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "latencyMode", "Latency Mode", juce::StringArray{ "Low Latency", "High Quality" }, 0));

    return { params.begin(), params.end() };
}

void JTuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    pitchDetector.prepare(currentSampleRate, samplesPerBlock);
    smoothedMidi = -1.0;
    pitchRatio = 1.0;
    phase = 0.0;
}

bool JTuneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void JTuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    if (numChannels == 0 || numSamples == 0) return;

    const int keyIdx = (int)apvts.getRawParameterValue("key")->load();
    const int scaleIdx = (int)apvts.getRawParameterValue("scale")->load();
    const float retuneMs = apvts.getRawParameterValue("retuneSpeed")->load();
    const float flexPct = apvts.getRawParameterValue("flexTune")->load();
    const float humanizePct = apvts.getRawParameterValue("humanize")->load();
    const float outputDb = apvts.getRawParameterValue("outputGain")->load();
    const bool lowLatency = ((int)apvts.getRawParameterValue("latencyMode")->load() == 0);

    quantizer.setRootNote(keyIdx);
    quantizer.setScaleType((ScaleType)scaleIdx);

    const float* in0 = buffer.getReadPointer(0);
    pitchDetector.pushSamples(in0, numSamples);

    const double detectedHz = pitchDetector.detectFrequencyHz(lowLatency);
    const double detectedMidi = PitchDetector::freqToMidi(detectedHz);

    if (detectedMidi > 0.0) {
        const double targetMidi = quantizer.quantizeMidiNote(detectedMidi);
        const double flexFactor = (double)flexPct / 100.0;
        const double humanizeFactor = (double)humanizePct / 100.0;

        // Zero-Slip Magnetic Lock: ยิ่ง retuneMs ใกล้ 0 ยิ่งดึงเข้าโน้ตเป้าหมายฉับพลัน
        double effectiveTarget = targetMidi + (detectedMidi - targetMidi) * flexFactor * 0.5;
        if (retuneMs <= 0.5f) {
            smoothedMidi = effectiveTarget;
        } else {
            double tauSec = (double)retuneMs * 0.001 * (1.0 + humanizeFactor * 0.4);
            double alpha = 1.0 - std::exp(-((double)numSamples / currentSampleRate) / std::max(0.0005, tauSec));
            if (smoothedMidi < 0.0) smoothedMidi = effectiveTarget;
            smoothedMidi += (effectiveTarget - smoothedMidi) * alpha;
        }

        double semitoneShift = smoothedMidi - detectedMidi;
        pitchRatio = std::pow(2.0, semitoneShift / 12.0);
        currentCentsError.store((float)((detectedMidi - targetMidi) * 100.0));
        currentDetectedMidi.store((float)detectedMidi);
    } else {
        pitchRatio = 1.0;
        currentCentsError.store(0.0f);
        currentDetectedMidi.store(-1.0f);
    }

    // Delay-line Pitch Shifter สำหรับโหมดเรียลไทม์
    const int maxDelay = 2048;
    std::vector<float> delayLine((size_t)maxDelay, 0.0f);
    const float gainLinear = juce::Decibels::decibelsToGain(outputDb);

    for (int ch = 0; ch < numChannels; ++ch) {
        float* data = buffer.getWritePointer(ch);
        int writeIdx = 0;
        double localPhase = phase;

        for (int i = 0; i < numSamples; ++i) {
            float sample = data[i];
            delayLine[(size_t)writeIdx] = sample;

            double rate = 1.0 - pitchRatio;
            localPhase += rate / 256.0;
            if (localPhase >= 1.0) localPhase -= 1.0;
            if (localPhase < 0.0)  localPhase += 1.0;

            double readPos = (double)writeIdx - (localPhase * 256.0);
            while (readPos < 0.0) readPos += (double)maxDelay;

            int idxA = (int)readPos % maxDelay;
            int idxB = (idxA + 1) % maxDelay;
            float frac = (float)(readPos - (double)(int)readPos);
            float shifted = delayLine[(size_t)idxA] * (1.0f - frac) + delayLine[(size_t)idxB] * frac;

            data[i] = (detectedMidi > 0.0 ? shifted : sample) * gainLinear;
            writeIdx = (writeIdx + 1) % maxDelay;
        }

        if (ch == 0) phase = localPhase;
    }
}

juce::AudioProcessorEditor* JTuneAudioProcessor::createEditor() {
    return new JTuneAudioProcessorEditor(*this);
}

void JTuneAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void JTuneAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new JTuneAudioProcessor();
}