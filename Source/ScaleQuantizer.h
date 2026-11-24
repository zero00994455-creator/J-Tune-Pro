#pragma once
#include <array>
#include <cmath>
#include <vector>

enum class ScaleType {
    Major = 0,
    MajorPentatonic = 1,
    Minor = 2,
    MinorPentatonic = 3
};

class ScaleQuantizer {
public:
    void setRootNote(int rootNote0To11) {
        rootNote = ((rootNote0To11 % 12) + 12) % 12;
    }

    void setScaleType(ScaleType type) {
        currentScale = type;
    }

    void setNoteEnabled(int pitchClass0To11, bool enabled) {
        if (pitchClass0To11 >= 0 && pitchClass0To11 < 12)
            noteMask[(size_t)pitchClass0To11] = enabled;
    }

    bool isNoteEnabled(int pitchClass0To11) const {
        if (pitchClass0To11 >= 0 && pitchClass0To11 < 12)
            return noteMask[(size_t)pitchClass0To11];
        return false;
    }

    bool isInActiveScale(int pitchClass0To11) const {
        const auto& intervals = getIntervals(currentScale);
        int rel = ((pitchClass0To11 - rootNote) % 12 + 12) % 12;
        for (int interval : intervals) {
            if (interval == rel)
                return noteMask[(size_t)pitchClass0To11];
        }
        return false;
    }

    // คืนค่า MIDI note ที่ใกล้ที่สุดในสเกลที่เปิดใช้งานอยู่
    double quantizeMidiNote(double inputMidi) const {
        int baseMidi = (int)std::round(inputMidi);
        double bestMidi = inputMidi;
        double minDist = 999.0;

        for (int candidate = baseMidi - 12; candidate <= baseMidi + 12; ++candidate) {
            int pc = ((candidate % 12) + 12) % 12;
            if (isInActiveScale(pc)) {
                double dist = std::abs(inputMidi - (double)candidate);
                if (dist < minDist) {
                    minDist = dist;
                    bestMidi = (double)candidate;
                }
            }
        }
        return bestMidi;
    }

private:
    int rootNote = 0; // 0 = C
    ScaleType currentScale = ScaleType::Minor;
    std::array<bool, 12> noteMask { true, true, true, true, true, true, true, true, true, true, true, true };

    static std::vector<int> getIntervals(ScaleType type) {
        switch (type) {
            case ScaleType::Major:           return { 0, 2, 4, 5, 7, 9, 11 };
            case ScaleType::MajorPentatonic: return { 0, 2, 4, 7, 9 };
            case ScaleType::Minor:           return { 0, 2, 3, 5, 7, 8, 10 };
            case ScaleType::MinorPentatonic: return { 0, 3, 5, 7, 10 };
            default:                         return { 0, 2, 3, 5, 7, 8, 10 };
        }
    }
};