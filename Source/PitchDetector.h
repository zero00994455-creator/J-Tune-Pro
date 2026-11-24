#pragma once
#include <cmath>
#include <vector>

class PitchDetector {
public:
    void prepare(double newSampleRate, int maxBlockSize) {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        ringBuffer.assign((size_t)(maxBlockSize * 4 + 2048), 0.0f);
        writePos = 0;
    }

    void pushSamples(const float* data, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            ringBuffer[writePos] = data[i];
            writePos = (writePos + 1) % ringBuffer.size();
        }
    }

    // คืนค่าความถี่ (Hz) แบบ YIN/Autocorrelation เบื้องต้น (0.0 หากไม่พบเสียงร้อง)
    double detectFrequencyHz(bool lowLatencyMode) const {
        const int windowSize = lowLatencyMode ? 512 : 1024;
        if (ringBuffer.size() < (size_t)windowSize)
            return 0.0;

        const double minFreq = 65.0;   // ~C2
        const double maxFreq = 1046.5; // ~C6
        const int minLag = std::max(2, (int)(sampleRate / maxFreq));
        const int maxLag = std::min(windowSize - 1, (int)(sampleRate / minFreq));

        double energy = 0.0;
        for (int i = 0; i < windowSize; ++i) {
            size_t idx = (writePos + ringBuffer.size() - (size_t)windowSize + (size_t)i) % ringBuffer.size();
            double s = ringBuffer[idx];
            energy += s * s;
        }
        if (energy / windowSize < 1e-6)
            return 0.0;

        double bestCorr = -1.0;
        int bestLag = -1;

        for (int lag = minLag; lag <= maxLag; ++lag) {
            double corr = 0.0;
            for (int i = 0; i < windowSize - lag; ++i) {
                size_t idx1 = (writePos + ringBuffer.size() - (size_t)windowSize + (size_t)i) % ringBuffer.size();
                size_t idx2 = (idx1 + (size_t)lag) % ringBuffer.size();
                corr += (double)ringBuffer[idx1] * (double)ringBuffer[idx2];
            }
            if (corr > bestCorr) {
                bestCorr = corr;
                bestLag = lag;
            }
        }

        if (bestLag <= 0)
            return 0.0;

        return sampleRate / (double)bestLag;
    }

    static double freqToMidi(double hz) {
        if (hz <= 0.0) return -1.0;
        return 69.0 + 12.0 * std::log2(hz / 440.0);
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> ringBuffer;
    size_t writePos = 0;
};