// Parameter metadata shared by the DSP wrapper and the UI. License: MIT.
#pragma once
#include "CalderaEngine.hpp"
#include <cstdio>
#include <string>

namespace caldera {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4, kHintOutput = 8, kHintBipolar = 16;

struct ParamDef { const char* name; const char* sym; const char* unit; float min, max; uint32_t hints; };

inline const char* filterFieldName(int f) {
    static const char* const n[kFAllFields] = { "On", "Style", "Shape", "Slope", "Freq", "Peak", "Pan", "Key", "Drive", "Gain", "Delay", "Level", "Out Pan" };
    return n[f];
}
inline const char* srcShort(int s) { return sourceShort(s); }
inline const char* destName(int d) {
    static std::string names[kNumDests]; static bool init = false;
    if (!init) {
        init = true; char b[48];
        names[kDstNone] = "None";
        for (int f = 0; f < kNumFilters; ++f) for (int k = 0; k < kNumFilterDests; ++k) { std::snprintf(b, sizeof b, "Filter %d %s", f + 1, filterFieldName(kFilterDestFields[k])); names[kDstFilterFirst + f * kNumFilterDests + k] = b; }
        names[kDstInGain] = "Input Gain"; names[kDstOutGain] = "Output Gain"; names[kDstMix] = "Mix";
        for (int l = 0; l < kNumXlfo; ++l) { std::snprintf(b, sizeof b, "XLFO %d Rate", l + 1); names[kDstXlfoRateFirst + l] = b; std::snprintf(b, sizeof b, "XLFO %d Balance", l + 1); names[kDstXlfoBalFirst + l] = b; }
        static const char* const adsr[4] = { "Attack", "Decay", "Sustain", "Release" };
        for (int e = 0; e < kNumEg; ++e) for (int k = 0; k < 4; ++k) { std::snprintf(b, sizeof b, "EG %d %s", e + 1, adsr[k]); names[kDstEgFirst + e * 4 + k] = b; }
        for (int s = 0; s < kNumSlots; ++s) { std::snprintf(b, sizeof b, "Slot %d Amount", s + 1); names[kDstSlotFirst + s] = b; }
    }
    return names[d < 0 || d >= kNumDests ? 0 : d].c_str();
}

inline const ParamDef& paramDef(uint32_t i) {
    static ParamDef t[kParamCount];
    static std::string nameStore[kParamCount], symStore[kParamCount];
    static bool done = false;
    if (!done) {
        done = true;
        auto set = [](uint32_t i, const std::string& n, const std::string& s, const char* u, uint32_t h) {
            nameStore[i] = n; symStore[i] = s; const Range r = paramRange(i);
            t[i] = { nameStore[i].c_str(), symStore[i].c_str(), u, r.min, r.max, h | (r.integer ? kHintInteger : 0) | (r.log ? kHintLog : 0) | (r.min < 0 && r.max > 0 ? kHintBipolar : 0) };
        };
        const uint32_t B = kHintBoolean;
        // filters: 2.x symbols for filters 1 and 2 ("f1freq"), the same scheme for 3 and 4
        static const char* const fs[kFAllFields] = { "on", "style", "type", "slope", "freq", "peak", "pan", "midi", "drive", "gain", "delay", "level", "opan" };
        static const char* const fn[kFAllFields] = { "On", "Style", "Type", "Slope", "Freq", "Peak", "Pan", "MIDI", "Drive", "Gain", "Delay", "Level", "Output Pan" };
        static const char* const fu[kFAllFields] = { "", "", "", "", "Hz", "", "", "", "dB", "dB", "ms", "dB", "" };
        for (int f = 0; f < kNumFilters; ++f) for (int k = 0; k < kFAllFields; ++k)
            set(fpar(f, k), "Filter " + std::to_string(f + 1) + " " + fn[k], "f" + std::to_string(f + 1) + fs[k], fu[k], (k == kFOn || k == kFMidi) ? B : 0);
        set(kRouting,  "Routing",     "routing",  "",   0);
        set(kInGain,   "Input Gain",  "ingain",   "dB", 0);
        set(kOutGain,  "Output Gain", "outgain",  "dB", 0);
        set(kMix,      "Mix",         "mix",      "",   0);
        set(kAutoMute, "Auto Mute",   "automute", "",   B);
        static const char* ln[6] = { "Wave", "Balance", "Sync", "Rate", "Division", "MIDI Retrigger" };
        static const char* ls[6] = { "wave", "bal", "sync", "rate", "div", "midi" };
        for (int l = 0; l < 2; ++l) {
            const uint32_t b = l == 0 ? kL1Wave : kL2Wave;
            for (int k = 0; k < 6; ++k) set(b + k, "LFO " + std::to_string(l + 1) + " " + ln[k], "l" + std::to_string(l + 1) + ls[k], k == 3 ? "Hz" : "", k == 5 ? B : 0);
        }
        set(kEgAttack,    "Env Attack",    "egatk",  "s",  0);
        set(kEgDecay,     "Env Decay",     "egdec",  "s",  0);
        set(kEgSustain,   "Env Sustain",   "egsus",  "",   0);
        set(kEgRelease,   "Env Release",   "egrel",  "s",  0);
        set(kEgThreshold, "Env Threshold", "egthr",  "dB", 0);
        set(kEgMidi,      "Env MIDI Trigger", "egmidi", "", B);
        static const char* srcN[3] = { "LFO 1", "LFO 2", "Env" };
        static const char* srcS[3] = { "l1", "l2", "eg" };
        static const char* dstN[6] = { "F1 Freq", "F1 Peak", "F1 Pan", "F2 Freq", "F2 Peak", "F2 Pan" };
        static const char* dstS[6] = { "f1f", "f1p", "f1n", "f2f", "f2p", "f2n" };
        for (int s = 0; s < 3; ++s) for (int d = 0; d < 6; ++d)
            set(modParam(s, d), std::string(srcN[s]) + " > " + dstN[d], std::string("m_") + srcS[s] + "_" + dstS[d], "", 0);
        set(kMidiOn,    "MIDI",        "midion",  "", B);
        set(kOutEg,     "Env Level",   "o_eg",    "", kHintOutput);
        set(kOutF1Live, "F1 Live Freq","o_f1",    "Hz", kHintOutput);
        set(kOutF2Live, "F2 Live Freq","o_f2",    "Hz", kHintOutput);
        set(kOutLearn,  "MIDI Learn",  "o_learn", "", kHintOutput);
        // 3.0
        set(kChanMode, "Channel Mode", "chmode", "", 0);
        set(kHq, "High Quality", "hq", "", B);
        set(kInPan, "Input Pan", "inpan", "", 0);
        set(kPiano, "Piano Display", "piano", "", B);
        static const char* const xn[kLStep1] = { "Mode", "Wave", "Rate", "Sync", "Division", "Balance", "Glide", "Steps" };
        static const char* const xs[kLStep1] = { "mode", "wave", "rate", "sync", "div", "bal", "glide", "steps" };
        for (int l = 0; l < kNumXlfo; ++l) {
            const std::string pre = "XLFO " + std::to_string(l + 1) + " ", sy = "x" + std::to_string(l + 1);
            for (int f = 0; f < kLStep1; ++f) set(lp_(l, f), pre + xn[f], sy + xs[f], f == kLRate ? "Hz" : "", 0);
            for (int s = 0; s < kNumSteps; ++s) set(lp_(l, kLStep1 + s), pre + "Step " + std::to_string(s + 1), sy + "s" + std::to_string(s + 1), "", 0);
        }
        static const char* const en[kEgFields] = { "Attack", "Decay", "Sustain", "Release", "Trigger", "Threshold" };
        static const char* const es[kEgFields] = { "a", "d", "s", "r", "trig", "thr" };
        static const char* const eu[kEgFields] = { "s", "s", "", "s", "", "dB" };
        for (int e = 0; e < kNumEg; ++e) for (int f = 0; f < kEgFields; ++f) set(ep(e, f), "EG " + std::to_string(e + 1) + " " + en[f], "e" + std::to_string(e + 1) + es[f], eu[f], 0);
        static const char* const fn2[kEfFields] = { "Attack", "Release", "Gain", "Source" };
        static const char* const fs2[kEfFields] = { "a", "r", "gain", "src" };
        static const char* const fu2[kEfFields] = { "s", "s", "dB", "" };
        for (int e = 0; e < kNumEf; ++e) for (int f = 0; f < kEfFields; ++f) set(fp(e, f), "EF " + std::to_string(e + 1) + " " + fn2[f], "ef" + std::to_string(e + 1) + fs2[f], fu2[f], 0);
        for (int x = 0; x < kNumXy; ++x) { set(xyp(x, 0), "XY " + std::to_string(x + 1) + " X", "xy" + std::to_string(x + 1) + "x", "", 0); set(xyp(x, 1), "XY " + std::to_string(x + 1) + " Y", "xy" + std::to_string(x + 1) + "y", "", 0); }
        static const char* const sn[kSlotFields] = { "Source", "Destination", "Amount", "Bypass" };
        static const char* const ss[kSlotFields] = { "src", "dst", "amt", "byp" };
        for (int s = 0; s < kNumSlots; ++s) for (int f = 0; f < kSlotFields; ++f) set(sp(s, f), "Slot " + std::to_string(s + 1) + " " + sn[f], "s" + std::to_string(s + 1) + ss[f], "", f == kSlotBypass ? B : 0);
    }
    return t[i];
}

inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* style[kNumStyles]; static bool sinit = false;
    if (!sinit) { sinit = true; for (int i = 0; i < kNumStyles; ++i) style[i] = styleName(i); }
    static const char* const type[]  = { "Low Pass", "Band Pass", "High Pass", "Bell", "Low Shelf", "High Shelf", "Notch", "All Pass" };
    static const char* const slope[] = { "12 dB/oct", "24 dB/oct", "48 dB/oct", "6 dB/oct" };
    static const char* const route[] = { "Serial", "Parallel", "Serial Pairs", "Parallel Pairs" };
    static const char* const wave[]  = { "Triangle", "Square" };
    static const char* const sync[]  = { "Free", "Straight", "Dotted", "Triplet" };
    static const char* const div[kNumDivs] = { "16 bars", "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
    static const char* const chm[] = { "Stereo", "Left/Right", "Mid/Side" };
    static const char* const xmode[] = { "Wave", "Steps" };
    static const char* const xwave[] = { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Random", "Sample & Hold" };
    static const char* const trig[] = { "MIDI", "Audio" };
    static const char* const efs[] = { "Input", "Sidechain" };
    static const char* srcs[kNumSources]; static const char* dsts[kNumDests]; static bool init = false;
    if (!init) { init = true; for (int i = 0; i < kNumSources; ++i) srcs[i] = sourceName(i); for (int i = 0; i < kNumDests; ++i) dsts[i] = destName(i); }
    count = 0;
    for (int f = 0; f < kNumFilters; ++f) {
        if (p == fpar(f, kFStyle)) { count = kNumStyles; return style; }
        if (p == fpar(f, kFType)) { count = kNumShapes; return type; }
        if (p == fpar(f, kFSlope)) { count = kNumSlopes; return slope; }
    }
    switch (p) {
    case kRouting:                count = kNumRoutings; return route;
    case kL1Wave:  case kL2Wave:  count = 2;  return wave;
    case kL1Sync:  case kL2Sync:  count = 4;  return sync;
    case kL1Div:   case kL2Div:   count = kNumDivs; return div;
    case kChanMode:               count = kNumChanModes; return chm;
    default: break;
    }
    if (p >= kXlfoFirst && p < kEgFirst) {
        const int f = (int)((p - kXlfoFirst) % kXlfoFields);
        if (f == kLMode) { count = 2; return xmode; }
        if (f == kLWave) { count = kNumWaves; return xwave; }
        if (f == kLSync) { count = 4; return sync; }
        if (f == kLDiv) { count = kNumDivs; return div; }
    }
    if (p >= kEgFirst && p < kEfFirst && (p - kEgFirst) % kEgFields == kETrigger) { count = 2; return trig; }
    if (p >= kEfFirst && p < kXyFirst && (p - kEfFirst) % kEfFields == kFSource) { count = 2; return efs; }
    if (p >= kSlotFirst && p < kInPan) {
        const int f = (int)((p - kSlotFirst) % kSlotFields);
        if (f == kSlotSrc) { count = kNumSources; return srcs; }
        if (f == kSlotDst) { count = kNumDests; return dsts; }
    }
    return nullptr;
}

inline bool isTimeParam(uint32_t p) { return p == kEgAttack || p == kEgDecay || p == kEgRelease; }
inline bool isModParam(uint32_t p) { return p >= kModFirst && p <= kModLast; }
inline bool isSlotAmount(uint32_t p) { return p >= kSlotFirst && p < kInPan && (p - kSlotFirst) % kSlotFields == kSlotAmt; }
// every parameter except the outputs is a setting (saved in presets, A/B and undo)
inline bool isSetting(uint32_t p) { return p < kParamCount && !(paramDef(p).hints & kHintOutput); }

// Normalised 0..1 <-> value (log for frequencies, square-law for the classic envelope times and slot amounts); used by the UI and MIDI learn.
inline float toNorm(uint32_t p, float v) {
    if (isSlotAmount(p)) return 0.5f + 0.5f * std::copysign(std::sqrt(std::fabs(clampf(v, -1.f, 1.f))), v);
    const ParamDef& d = paramDef(p);
    if (d.hints & kHintLog) return std::log(clampf(v, d.min, d.max) / d.min) / std::log(d.max / d.min);
    float n = (v - d.min) / (d.max - d.min);
    if (isTimeParam(p)) n = std::sqrt(std::max(n, 0.f));
    return n;
}
inline float fromNorm(uint32_t p, float n) {
    n = clampf(n, 0.f, 1.f);
    if (isSlotAmount(p)) { const float u = (n - 0.5f) * 2.f; return std::copysign(u * u, u); }
    const ParamDef& d = paramDef(p);
    float v;
    if (d.hints & kHintLog) v = d.min * std::pow(d.max / d.min, n);
    else { if (isTimeParam(p)) n = n * n; v = d.min + n * (d.max - d.min); }
    if (d.hints & kHintInteger) v = std::round(v);
    return v;
}

} // namespace caldera
