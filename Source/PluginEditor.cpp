#include "PluginEditor.h"

JTuneAudioProcessorEditor::JTuneAudioProcessorEditor(JTuneAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      keyAtt(p.apvts, "key", keyBox),
      scaleAtt(p.apvts, "scale", scaleBox),
      latencyAtt(p.apvts, "latencyMode", latencyBox),
      retuneAtt(p.apvts, "retuneSpeed", retuneSlider),
      flexAtt(p.apvts, "flexTune", flexSlider),
      humanizeAtt(p.apvts, "humanize", humanizeSlider),
      formantAtt(p.apvts, "formant", formantSlider),
      vibratoAtt(p.apvts, "vibrato", vibratoSlider),
      outputAtt(p.apvts, "outputGain", outputSlider) {

    setSize(980, 560);

    keyBox.addItemList({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    scaleBox.addItemList({ "Major", "Major Pentatonic", "Minor", "Minor Pentatonic" }, 1);
    latencyBox.addItemList({ "Low Latency", "High Quality" }, 1);

    for (auto* box : { &keyBox, &scaleBox, &latencyBox }) {
        box->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1226));
        box->setColour(juce::ComboBox::textColourId, juce::Colour(0xfff3e8ff));
        box->setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff9333ea));
        addAndMakeVisible(*box);
    }

    auto setupRotary = [&](juce::Slider& s, const juce::String& suffix) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
        s.setTextValueSuffix(suffix);
        s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffa855f7));
        s.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe9d5ff));
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff3e8ff));
        addAndMakeVisible(s);
    };

    setupRotary(retuneSlider, " ms");
    setupRotary(flexSlider, " %");
    setupRotary(humanizeSlider, " %");
    setupRotary(formantSlider, " %");
    setupRotary(vibratoSlider, " %");
    setupRotary(outputSlider, " dB");

    hardLockBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff581c87));
    hardLockBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    hardLockBtn.onClick = [this]() {
        retuneSlider.setValue(0.0, juce::sendNotificationSync);
        flexSlider.setValue(0.0, juce::sendNotificationSync);
        humanizeSlider.setValue(0.0, juce::sendNotificationSync);
    };
    addAndMakeVisible(hardLockBtn);

    startTimerHz(30);
}

void JTuneAudioProcessorEditor::timerCallback() {
    repaint();
}

void JTuneAudioProcessorEditor::paint(juce::Graphics& g) {
    // พื้นหลังม่วง-ดำไล่ระดับ
    juce::ColourGradient bg(juce::Colour(0xff09060f), 0.0f, 0.0f,
                          juce::Colour(0xff190b2e), (float)getWidth(), (float)getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    // แถบหัวข้อด้านบน
    g.setColour(juce::Colour(0xff130a22));
    g.fillRect(0, 0, getWidth(), 48);
    g.setColour(juce::Colour(0xffa855f7));
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("J-TUNE  |  VOCAL AUTO-TUNE", 20, 8, 340, 32, juce::Justification::left);

    // กรอบคอลัมน์ซ้าย / กลาง / ขวา
    g.setColour(juce::Colour(0xff7e22ce).withAlpha(0.55f));
    g.drawRoundedRectangle(16.0f, 60.0f, 230.0f, 345.0f, 10.0f, 1.5f);
    g.drawRoundedRectangle(258.0f, 60.0f, 440.0f, 345.0f, 10.0f, 1.5f);
    g.drawRoundedRectangle(710.0f, 60.0f, 254.0f, 345.0f, 10.0f, 1.5f);

    // ป้ายกำกับหมวดหมู่
    g.setColour(juce::Colour(0xffd8b4fe));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("KEY & SCALE", 28, 68, 200, 18, juce::Justification::left);
    g.drawText("RETUNE SPEED", 28, 172, 105, 18, juce::Justification::centred);
    g.drawText("FLEX-TUNE", 135, 172, 105, 18, juce::Justification::centred);
    g.drawText("HUMANIZE", 722, 175, 115, 18, juce::Justification::centred);
    g.drawText("FORMANT", 842, 175, 115, 18, juce::Justification::centred);
    g.drawText("VIBRATO", 722, 285, 115, 18, juce::Justification::centred);
    g.drawText("OUTPUT GAIN", 842, 285, 115, 18, juce::Justification::centred);

    // หน้าปัดวงกลมแสดงค่าเบี่ยงเบนระดับเสียง (Cents) ตรงกลาง
    const float cx = 478.0f;
    const float cy = 225.0f;
    const float radius = 92.0f;

    g.setColour(juce::Colour(0xffa855f7).withAlpha(0.28f));
    g.fillEllipse(cx - radius - 8.0f, cy - radius - 8.0f, (radius + 8.0f) * 2.0f, (radius + 8.0f) * 2.0f);

    g.setColour(juce::Colour(0xff140924));
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    g.setColour(juce::Colour(0xffc084fc));
    g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 2.5f);

    const float cents = audioProcessor.currentCentsError.load();
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(34.0f, juce::Font::bold));
    g.drawText(juce::String(cents, 1), (int)(cx - 70), (int)(cy - 28), 140, 38, juce::Justification::centred);

    g.setColour(juce::Colour(0xffd8b4fe));
    g.setFont(juce::Font(13.0f));
    g.drawText("CENTS", (int)(cx - 50), (int)(cy + 8), 100, 18, juce::Justification::centred);

    g.setColour(juce::Colour(0xffa855f7));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(std::abs(cents) < 5.0f ? "LOCKED" : "TUNING", (int)(cx - 50), (int)(cy + 28), 100, 18, juce::Justification::centred);

    // แถบคีย์บอร์ดด้านล่างไฮไลต์โน้ตตาม 4 สเกลที่เลือก
    const int kbX = 16;
    const int kbY = 418;
    const int kbW = getWidth() - 32;
    const int kbH = 124;
    const int numWhiteKeys = 14;
    const float whiteW = (float)kbW / (float)numWhiteKeys;

    const int whiteSemitones[14] = { 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23 };
    for (int i = 0; i < numWhiteKeys; ++i) {
        int pc = whiteSemitones[i] % 12;
        bool inScale = audioProcessor.quantizer.isInActiveScale(pc);
        float x = (float)kbX + i * whiteW;
        g.setColour(inScale ? juce::Colour(0xff9333ea) : juce::Colour(0xff271b38));
        g.fillRect(x, (float)kbY, whiteW - 2.0f, (float)kbH);
        g.setColour(juce::Colour(0xff09060f));
        g.drawRect(x, (float)kbY, whiteW - 2.0f, (float)kbH, 1.5f);
    }
}

void JTuneAudioProcessorEditor::resized() {
    keyBox.setBounds(28, 92, 96, 28);
    scaleBox.setBounds(132, 92, 102, 28);
    hardLockBtn.setBounds(28, 128, 206, 28);

    retuneSlider.setBounds(26, 192, 108, 105);
    flexSlider.setBounds(134, 192, 108, 105);

    latencyBox.setBounds(722, 82, 230, 28);
    humanizeSlider.setBounds(722, 194, 112, 90);
    formantSlider.setBounds(842, 194, 112, 90);
    vibratoSlider.setBounds(722, 304, 112, 90);
    outputSlider.setBounds(842, 304, 112, 90);
}