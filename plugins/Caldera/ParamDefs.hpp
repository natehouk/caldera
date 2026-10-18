// Parameter metadata shared by the DSP wrapper and the UI. License: MIT.
#pragma once
#include "CalderaEngine.hpp"
#include <cstdio>

namespace caldera {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4, kHintOutput = 8;

struct ParamDef { const char* name; const char* sym; const char* unit; float min, max; uint32_t hints; };

inline const ParamDef& paramDef(uint32_t i) {
    static ParamDef t[kParamCount];
    static bool done = false;
    if (!done) {
        done = true;
        auto set = [](uint32_t i, const char* n, const char* s, const char* u, float lo, float hi, uint32_t h) { t[i] = { n, s, u, lo, hi, h }; };
        static const char* fn[2][8] = {
            { "Filter 1 On", "Filter 1 Style", "Filter 1 Type", "Filter 1 Slope", "Filter 1 Freq", "Filter 1 Peak", "Filter 1 Pan", "Filter 1 MIDI" },
            { "Filter 2 On", "Filter 2 Style", "Filter 2 Type", "Filter 2 Slope", "Filter 2 Freq", "Filter 2 Peak", "Filter 2 Pan", "Filter 2 MIDI" } };
        static const char* fs[2][8] = {
            { "f1on", "f1style", "f1type", "f1slope", "f1freq", "f1peak", "f1pan", "f1midi" },
            { "f2on", "f2style", "f2type", "f2slope", "f2freq", "f2peak", "f2pan", "f2midi" } };
        for (int f = 0; f < 2; ++f) {
            const uint32_t b = f == 0 ? kF1On : kF2On;
            set(b + 0, fn[f][0], fs[f][0], "",   0, 1, kHintBoolean | kHintInteger);
            set(b + 1, fn[f][1], fs[f][1], "",   0, kNumStyles - 1, kHintInteger);
            set(b + 2, fn[f][2], fs[f][2], "",   0, 2, kHintInteger);
            set(b + 3, fn[f][3], fs[f][3], "",   0, 2, kHintInteger);
            set(b + 4, fn[f][4], fs[f][4], "Hz", 20, 20000, kHintLog);
            set(b + 5, fn[f][5], fs[f][5], "",   0, 1, 0);
            set(b + 6, fn[f][6], fs[f][6], "",   -1, 1, 0);
            set(b + 7, fn[f][7], fs[f][7], "",   0, 1, kHintBoolean | kHintInteger);
        }
        set(kRouting,  "Routing",     "routing",  "",   0, 1, kHintInteger);
        set(kInGain,   "Input Gain",  "ingain",   "dB", -24, 24, 0);
        set(kOutGain,  "Output Gain", "outgain",  "dB", -24, 24, 0);
        set(kMix,      "Mix",         "mix",      "",   0, 1, 0);
        set(kAutoMute, "Auto Mute",   "automute", "",   0, 1, kHintBoolean | kHintInteger);
        static const char* ln[2][6] = {
            { "LFO 1 Wave", "LFO 1 Balance", "LFO 1 Sync", "LFO 1 Rate", "LFO 1 Division", "LFO 1 MIDI Retrigger" },
            { "LFO 2 Wave", "LFO 2 Balance", "LFO 2 Sync", "LFO 2 Rate", "LFO 2 Division", "LFO 2 MIDI Retrigger" } };
        static const char* ls[2][6] = {
            { "l1wave", "l1bal", "l1sync", "l1rate", "l1div", "l1midi" },
            { "l2wave", "l2bal", "l2sync", "l2rate", "l2div", "l2midi" } };
        for (int l = 0; l < 2; ++l) {
            const uint32_t b = l == 0 ? kL1Wave : kL2Wave;
            set(b + 0, ln[l][0], ls[l][0], "",   0, 1, kHintInteger);
            set(b + 1, ln[l][1], ls[l][1], "",   0, 1, 0);
            set(b + 2, ln[l][2], ls[l][2], "",   0, 3, kHintInteger);
            set(b + 3, ln[l][3], ls[l][3], "Hz", 0.01f, 100, kHintLog);
            set(b + 4, ln[l][4], ls[l][4], "",   0, kNumDivs - 1, kHintInteger);
            set(b + 5, ln[l][5], ls[l][5], "",   0, 1, kHintBoolean | kHintInteger);
        }
        set(kEgAttack,    "Env Attack",    "egatk",  "s",  0, 5, 0);
        set(kEgDecay,     "Env Decay",     "egdec",  "s",  0.001f, 5, 0);
        set(kEgSustain,   "Env Sustain",   "egsus",  "",   0, 1, 0);
        set(kEgRelease,   "Env Release",   "egrel",  "s",  0.001f, 5, 0);
        set(kEgThreshold, "Env Threshold", "egthr",  "dB", -60, 0, 0);
        set(kEgMidi,      "Env MIDI Trigger", "egmidi", "", 0, 1, kHintBoolean | kHintInteger);
        static const char* srcN[3] = { "LFO 1", "LFO 2", "Env" };
        static const char* srcS[3] = { "l1", "l2", "eg" };
        static const char* dstN[6] = { "F1 Freq", "F1 Peak", "F1 Pan", "F2 Freq", "F2 Peak", "F2 Pan" };
        static const char* dstS[6] = { "f1f", "f1p", "f1n", "f2f", "f2p", "f2n" };
        static char names[18][32], syms[18][16];
        for (int s = 0; s < 3; ++s)
            for (int d = 0; d < 6; ++d) {
                const int i = s * 6 + d;
                std::snprintf(names[i], sizeof names[i], "%s > %s", srcN[s], dstN[d]);
                std::snprintf(syms[i], sizeof syms[i], "m_%s_%s", srcS[s], dstS[d]);
                set(modParam(s, d), names[i], syms[i], "", -1, 1, 0);
            }
        set(kMidiOn,    "MIDI",        "midion",  "", 0, 1, kHintBoolean | kHintInteger);
        set(kOutEg,     "Env Level",   "o_eg",    "", 0, 1, kHintOutput);
        set(kOutF1Live, "F1 Live Freq","o_f1",    "Hz", 10, 22000, kHintOutput);
        set(kOutF2Live, "F2 Live Freq","o_f2",    "Hz", 10, 22000, kHintOutput);
        set(kOutLearn,  "MIDI Learn",  "o_learn", "", 0, 100000, kHintOutput);
    }
    return t[i];
}

inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* const style[] = { "Clean", "Smooth", "Warm", "Monolith", "Hot", "Acid", "Raw" };
    static const char* const type[]  = { "Low Pass", "Band Pass", "High Pass" };
    static const char* const slope[] = { "12 dB/oct", "24 dB/oct", "48 dB/oct" };
    static const char* const route[] = { "Serial", "Parallel" };
    static const char* const wave[]  = { "Triangle", "Square" };
    static const char* const sync[]  = { "Free", "Straight", "Dotted", "Triplet" };
    static const char* const div[]   = { "16 bars", "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
    switch (p) {
    case kF1Style: case kF2Style: count = 7;  return style;
    case kF1Type:  case kF2Type:  count = 3;  return type;
    case kF1Slope: case kF2Slope: count = 3;  return slope;
    case kRouting:                count = 2;  return route;
    case kL1Wave:  case kL2Wave:  count = 2;  return wave;
    case kL1Sync:  case kL2Sync:  count = 4;  return sync;
    case kL1Div:   case kL2Div:   count = 11; return div;
    default: count = 0; return nullptr;
    }
}

inline bool isTimeParam(uint32_t p) { return p == kEgAttack || p == kEgDecay || p == kEgRelease; }
inline bool isModParam(uint32_t p) { return p >= kModFirst && p <= kModLast; }

// Normalised 0..1 <-> value (log for frequencies, square-law for times); used by the UI and MIDI learn.
inline float toNorm(uint32_t p, float v) {
    const ParamDef& d = paramDef(p);
    if (d.hints & kHintLog) return std::log(v / d.min) / std::log(d.max / d.min);
    float n = (v - d.min) / (d.max - d.min);
    if (isTimeParam(p)) n = std::sqrt(std::max(n, 0.f));
    return n;
}
inline float fromNorm(uint32_t p, float n) {
    const ParamDef& d = paramDef(p);
    n = clampf(n, 0.f, 1.f);
    float v;
    if (d.hints & kHintLog) v = d.min * std::pow(d.max / d.min, n);
    else { if (isTimeParam(p)) n = n * n; v = d.min + n * (d.max - d.min); }
    if (d.hints & kHintInteger) v = std::round(v);
    return v;
}

} // namespace caldera
