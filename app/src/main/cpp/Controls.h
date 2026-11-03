#pragma once
#include <algorithm>
#include <cmath>
struct Controls {
    float gain = 1.f, output = 0.25f, envelope = 0.f, gateGain = 0.f;
    static float db(float value) { return std::pow(10.f, value / 20.f); }
    float input(float sample, float threshold) {
        if (!std::isfinite(sample)) sample = 0.f;
        envelope = std::max(std::abs(sample), envelope * 0.9995f);
        float target = threshold <= -90.f || envelope >= db(threshold) ? 1.f : 0.f;
        gateGain += (target - gateGain) * (target > gateGain ? 0.02f : 0.001f);
        return sample * gain * gateGain;
    }
    float finish(float sample) const {
        return std::isfinite(sample) ? std::clamp(sample * output, -1.f, 1.f) : 0.f;
    }
};
