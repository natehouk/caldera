// Caldera — stereo filter effect engine: up to four filters with eleven styles and eight shapes, flexible
// routing (serial, parallel and two mixed layouts; stereo, left/right or mid/side), per-filter drive, delay,
// level and pan, optional 4x oversampling, the 2.x LFOs/envelope/grid ("classic" modulation) and the
// shared modulation system v2 (XLFOs, envelopes, followers, XY pads, MIDI, 50 slots).
// Clean-room implementation from public, generic techniques: cascaded topology-preserving-transform
// state-variable filters (Zavalishin) with per-style nonlinearities, Butterworth stage damping, SVF
// mixing for bell/shelf/notch/all-pass responses, one-pole TPT sections for 6 dB/oct, halfband oversampling.
// No third-party product code, assets or disassembly were used.
// License: MIT (see LICENSE).

#pragma once
#include "common/Halfband.hpp"
#include "common/ModSystem.hpp"
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>

namespace caldera {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float dbToGain(float db) { return std::pow(10.f, db / 20.f); }

using namespace epsilon::mod;     // modulation system v2: field enums, sources, Xlfo, ADSR, divBars
using epsilon::OversamplerN;

static constexpr int kNumFilters = 4, kNumSlots = 50, kCtl = 16;

// ------------------------------------------------------------------ params
// Order is part of the saved-session format: only ever append.
enum FField { kFOn, kFStyle, kFType, kFSlope, kFFreq, kFPeak, kFPan, kFMidi, kFBaseFields,
              kFDrive = kFBaseFields, kFEqGain, kFDelay, kFLevel, kFOutPan, kFAllFields };
static constexpr int kFxFields = kFAllFields - kFBaseFields;     // the fields added in 3.0, for every filter
#define FILTER_PARAMS(n) kF##n##On, kF##n##Style, kF##n##Type, kF##n##Slope, kF##n##Freq, kF##n##Peak, kF##n##Pan, kF##n##Midi
#define LFO_PARAMS(n)    kL##n##Wave, kL##n##Balance, kL##n##Sync, kL##n##Rate, kL##n##Div, kL##n##Midi
enum Param : uint32_t {
    FILTER_PARAMS(1),
    FILTER_PARAMS(2),
    kRouting, kInGain, kOutGain, kMix, kAutoMute,
    LFO_PARAMS(1),
    LFO_PARAMS(2),
    kEgAttack, kEgDecay, kEgSustain, kEgRelease, kEgThreshold, kEgMidi,
    // classic modulation grid: source x destination, bipolar amounts
    kModFirst,
    kModLast = kModFirst + 3 * 6 - 1,
    kMidiOn,
    // outputs (read-only, for the UI)
    kOutEg, kOutF1Live, kOutF2Live, kOutLearn,
    // ---- 3.0
    kChanMode,                                       // Stereo, Left/Right, Mid/Side
    kHq,                                             // 4x oversampling
    kFxFirst,                                        // filters 1-4: drive, gain, delay, level, output pan
    kF34First = kFxFirst + kNumFilters * kFxFields,  // filters 3 and 4: the fields filters 1 and 2 had in 2.x
    kXlfoFirst = kF34First + 2 * kFBaseFields,
    kEgFirst = kXlfoFirst + kNumXlfo * kXlfoFields,
    kEfFirst = kEgFirst + kNumEg * kEgFields,
    kXyFirst = kEfFirst + kNumEf * kEfFields,
    kSlotFirst = kXyFirst + kNumXy * 2,
    kInPan = kSlotFirst + kNumSlots * kSlotFields,   // input balance
    kPiano,                                          // view: piano keyboard under the display
    kParamCount
};
#undef FILTER_PARAMS
#undef LFO_PARAMS

inline uint32_t fpar(int f, int field) {
    if (field >= kFBaseFields) return kFxFirst + (uint32_t)(f * kFxFields + field - kFBaseFields);
    if (f == 0) return kF1On + field;
    if (f == 1) return kF2On + field;
    return kF34First + (uint32_t)((f - 2) * kFBaseFields + field);
}
inline uint32_t lp_(int l, int f) { return kXlfoFirst + l * kXlfoFields + f; }
inline uint32_t ep(int e, int f) { return kEgFirst + e * kEgFields + f; }
inline uint32_t fp(int e, int f) { return kEfFirst + e * kEfFields + f; }
inline uint32_t xyp(int x, int axis) { return kXyFirst + x * 2 + axis; }
inline uint32_t sp(int s, int f) { return kSlotFirst + s * kSlotFields + f; }

// classic (2.x) modulation: two LFOs and an envelope on a fixed grid
enum ClassicSource { kSrcLfo1, kSrcLfo2, kSrcEg, kNumClassicSources };
enum ClassicDest   { kDstF1Freq, kDstF1Peak, kDstF1Pan, kDstF2Freq, kDstF2Peak, kDstF2Pan, kNumClassicDests };
inline uint32_t modParam(int src, int dst) { return kModFirst + src * kNumClassicDests + dst; }

// shapes, slopes, routing and channel modes (append-only)
enum Shape { kLowPass, kBandPass, kHighPass, kBell, kLowShelf, kHighShelf, kNotch, kAllPass, kNumShapes };
enum Slope { kSlope12, kSlope24, kSlope48, kSlope6, kNumSlopes };
enum Routing { kSerial, kParallel, kPairsParallel, kPairsSerial, kNumRoutings };     // 1>2>3>4, 1|2|3|4, (1>2)|(3>4), (1|2)>(3|4)
enum ChanMode { kStereo, kLeftRight, kMidSide, kNumChanModes };

// v2 modulation destinations
static constexpr int kFilterDestFields[] = { kFDrive, kFFreq, kFPeak, kFEqGain, kFPan, kFDelay, kFLevel, kFOutPan };
static constexpr int kNumFilterDests = sizeof(kFilterDestFields) / sizeof(kFilterDestFields[0]);
enum DestBase {
    kDstNone = 0,
    kDstFilterFirst = 1,
    kDstInGain = kDstFilterFirst + kNumFilters * kNumFilterDests, kDstOutGain, kDstMix,
    kDstXlfoRateFirst, kDstXlfoBalFirst = kDstXlfoRateFirst + kNumXlfo,
    kDstEgFirst = kDstXlfoBalFirst + kNumXlfo,            // A, D, S, R per EG
    kDstSlotFirst = kDstEgFirst + kNumEg * 4,
    kNumDests = kDstSlotFirst + kNumSlots
};

static constexpr int kNumStyles = 11, kNumStyles2 = 7;

// value range of every setting: min, max, default, log, integer
struct Range { float min, max, def; bool log, integer; };
inline Range paramRange(uint32_t p) {
    if (p < kRouting) {
        static const Range f[kFBaseFields] = { { 0, 1, 0, false, true }, { 0, kNumStyles - 1, 0, false, true }, { 0, kNumShapes - 1, 0, false, true },
            { 0, kNumSlopes - 1, 0, false, true }, { 20, 20000, 1000, true, false }, { 0, 1, 0, false, false }, { -1, 1, 0, false, false }, { 0, 1, 0, false, true } };
        return f[(p - kF1On) % kFBaseFields];
    }
    switch (p) {
    case kRouting: return { 0, kNumRoutings - 1, 0, false, true };
    case kInGain: case kOutGain: return { -24, 24, 0, false, false };
    case kMix: return { 0, 1, 1, false, false };
    case kAutoMute: return { 0, 1, 1, false, true };
    case kL1Wave: case kL2Wave: return { 0, 1, 0, false, true };
    case kL1Balance: case kL2Balance: return { 0, 1, 0.5f, false, false };
    case kL1Sync: case kL2Sync: return { 0, 3, 0, false, true };
    case kL1Rate: case kL2Rate: return { 0.01f, 100, 1, true, false };
    case kL1Div: case kL2Div: return { 0, kNumDivs - 1, 4, false, true };
    case kL1Midi: case kL2Midi: case kEgMidi: case kMidiOn: return { 0, 1, 0, false, true };
    case kEgAttack: return { 0, 5, 0.01f, false, false };
    case kEgDecay: case kEgRelease: return { 0.001f, 5, 0.3f, false, false };
    case kEgSustain: return { 0, 1, 0.5f, false, false };
    case kEgThreshold: return { -60, 0, -30, false, false };
    case kOutEg: return { 0, 1, 0, false, false };
    case kOutF1Live: case kOutF2Live: return { 10, 22000, 1000, true, false };
    case kOutLearn: return { 0, 10000000, 0, false, true };
    case kChanMode: return { 0, kNumChanModes - 1, 0, false, true };
    case kHq: return { 0, 1, 0, false, true };
    case kInPan: return { -1, 1, 0, false, false };
    case kPiano: return { 0, 1, 0, false, true };
    default: break;
    }
    if (p >= kModFirst && p <= kModLast) return { -1, 1, 0, false, false };
    if (p >= kFxFirst && p < kF34First) {
        static const Range x[kFxFields] = { { 0, 24, 0, false, false }, { -24, 24, 0, false, false }, { 0, 50, 0, false, false }, { -24, 24, 0, false, false }, { -1, 1, 0, false, false } };
        return x[(p - kFxFirst) % kFxFields];
    }
    if (p >= kF34First && p < kXlfoFirst) return paramRange(kF1On + (p - kF34First) % kFBaseFields);
    if (p >= kXlfoFirst && p < kEgFirst) { const FieldRange r = xlfoRange((int)((p - kXlfoFirst) % kXlfoFields)); return { r.min, r.max, r.def, r.log, r.integer }; }
    if (p >= kEgFirst && p < kEfFirst) { const FieldRange r = egRange((int)((p - kEgFirst) % kEgFields)); return { r.min, r.max, r.def, r.log, r.integer }; }
    if (p >= kEfFirst && p < kXyFirst) { const FieldRange r = efRange((int)((p - kEfFirst) % kEfFields)); return { r.min, r.max, r.def, r.log, r.integer }; }
    if (p >= kXyFirst && p < kSlotFirst) return { 0, 1, 0.5f, false, false };
    if (p >= kSlotFirst && p < kInPan) { const FieldRange r = slotRange((int)((p - kSlotFirst) % kSlotFields), kNumDests); return { r.min, r.max, r.def, r.log, r.integer }; }
    return { 0, 1, 0, false, false };
}
inline float normOf(uint32_t p, float v) {
    const Range r = paramRange(p);
    if (r.log) return std::log(clampf(v, r.min, r.max) / r.min) / std::log(r.max / r.min);
    return (clampf(v, r.min, r.max) - r.min) / (r.max - r.min);
}
inline float valueOf(uint32_t p, float n) {
    const Range r = paramRange(p); n = clampf(n, 0.f, 1.f);
    float v = r.log ? r.min * std::pow(r.max / r.min, n) : r.min + n * (r.max - r.min);
    if (r.integer) v = std::round(v);
    return v;
}
// the parameter a v2 destination modulates; -1 for slot amounts and none
inline int destParam(int d) {
    if (d >= kDstFilterFirst && d < kDstInGain) { const int k = d - kDstFilterFirst; return (int)fpar(k / kNumFilterDests, kFilterDestFields[k % kNumFilterDests]); }
    if (d == kDstInGain) return kInGain;
    if (d == kDstOutGain) return kOutGain;
    if (d == kDstMix) return kMix;
    if (d >= kDstXlfoRateFirst && d < kDstXlfoBalFirst) return (int)lp_(d - kDstXlfoRateFirst, kLRate);
    if (d >= kDstXlfoBalFirst && d < kDstEgFirst) return (int)lp_(d - kDstXlfoBalFirst, kLBalance);
    if (d >= kDstEgFirst && d < kDstSlotFirst) { const int k = d - kDstEgFirst; static const int f[4] = { kEAttack, kEDecay, kESustain, kERelease }; return (int)ep(k / 4, f[k % 4]); }
    return -1;
}
inline int destForParam(uint32_t p) {
    for (int d = 1; d < kDstSlotFirst; ++d) if (destParam(d) == (int)p) return d;
    if (p >= kSlotFirst && p < kInPan && (p - kSlotFirst) % kSlotFields == kSlotAmt) return kDstSlotFirst + (int)((p - kSlotFirst) / kSlotFields);
    return -1;
}

// ------------------------------------------------------------------ styles
// Per-style behaviour: input drive, stage nonlinearity and how far resonance can go.
//  nl: 0 linear, 1 soft-limit state (Monolith), 2 tanh integrator, 3 hard-clip state
//  in: input stage, 0 as 2.x (tanh, or x/(1+|x|) for nl 3), 1 asymmetric tube, 2 hard clip
//  scoop: resonance carves out the band around the cutoff (hollow tone)
struct StyleDef { float drive; int nl; float kMin; float bassLoss; int in; float scoop; };
inline const StyleDef& styleDef(int s) {
    static const StyleDef t[kNumStyles] = {
        { 1.0f, 0,  0.08f, 0.0f, 0, 0 },   // Clean    : linear, high-Q but never self-oscillates
        { 1.0f, 0,  0.35f, 0.0f, 0, 0 },   // Smooth   : gentle, wide resonance
        { 1.6f, 0,  0.04f, 0.0f, 0, 0 },   // Warm     : saturated input, linear filter
        { 1.2f, 1, -0.04f, 0.0f, 0, 0 },   // Monolith : Monolith's filter, self-oscillating
        { 2.0f, 2, -0.05f, 0.0f, 0, 0 },   // Hot      : saturating integrators, screaming peak
        { 1.8f, 1, -0.06f, 0.5f, 0, 0 },   // Acid     : resonance thins the bass, squelchy
        { 3.0f, 3, -0.08f, 0.0f, 0, 0 },   // Raw      : hard-clipped feedback, buzzy self-oscillation
        // 3.0
        { 1.5f, 2, -0.02f, 0.0f, 1, 0 },   // Tube     : asymmetric warm overdrive, soft singing peak
        { 1.15f, 0, 0.18f, 0.0f, 0, 0 },   // Gentle   : clean, broad and polite; never self-oscillates
        { 2.6f, 3, -0.10f, 0.0f, 2, 0 },   // Metal    : hard-clipped input and feedback: rough, sharp, distorted
        { 1.3f, 1, -0.06f, 0.0f, 0, 0.6f } // Hollow   : the band at the cutoff is carved out; deep low self-oscillation
    };
    return t[s < 0 ? 0 : (s >= kNumStyles ? kNumStyles - 1 : s)];
}
inline const char* styleName(int s) {
    static const char* const n[kNumStyles] = { "Clean", "Smooth", "Warm", "Monolith", "Hot", "Acid", "Raw", "Tube", "Gentle", "Metal", "Hollow" };
    return n[s < 0 ? 0 : (s >= kNumStyles ? kNumStyles - 1 : s)];
}

// Butterworth stage damping (k = 2*cos(theta)) for 1, 2 and 4 cascaded 2-pole stages.
inline const float* bwK(int stages) {
    static const float k1[1] = { 1.41421356f };
    static const float k2[2] = { 1.84775907f, 0.76536686f };
    static const float k4[4] = { 1.96157056f, 1.66293922f, 1.11114047f, 0.39018064f };
    return stages == 1 ? k1 : (stages == 2 ? k2 : k4);
}
inline int slopeStages(int slope) { return slope == kSlope12 ? 1 : (slope == kSlope24 ? 2 : (slope == kSlope48 ? 4 : 0)); }   // 0: 6 dB/oct
// bell/shelf/notch/all-pass damping: Peak narrows the band
inline float eqDamping(float reso) { return 1.41421356f * (1.f - 0.85f * std::pow(clampf(reso, 0.f, 1.f), 0.8f)); }

struct Stage { float ic1 = 0, ic2 = 0; };

// One filter, one channel: up to 4 cascaded TPT SVF stages (or one one-pole section for 6 dB/oct).
struct FilterChan {
    Stage st[4]; float z1 = 0, dcx = 0, dcy = 0;
    void reset() { for (Stage& s : st) s.ic1 = s.ic2 = 0; z1 = dcx = dcy = 0; }

    inline float input(float x, const StyleDef& sd) {
        if (sd.drive == 1.f) return x;
        if (sd.in == 1) {                     // asymmetric tube stage, DC removed
            const float d = sd.drive, b = 0.25f;
            const float y = (std::tanh(d * x + b) - std::tanh(b)) / d * 1.1f;
            const float o = y - dcx + 0.9995f * dcy; dcx = y; dcy = o; return o;
        }
        if (sd.in == 2) return clampf(x * sd.drive, -1.f, 1.f) / sd.drive * 1.1f;
        if (sd.nl == 3) return (x * sd.drive) / (1.f + std::fabs(x * sd.drive)) / sd.drive * 1.1f;
        return std::tanh(x * sd.drive) / sd.drive * 1.1f;
    }
    // one SVF stage; returns the band (v1) and low (v2) outputs, updating the state with the style's nonlinearity
    static inline void svf(Stage& s, float x, float g, float k, int nl, float& v1, float& v2) {
        const float a1 = 1.f / (1.f + g * (g + k));
        const float v3 = x - s.ic2;
        v1 = a1 * s.ic1 + g * a1 * v3;
        v2 = s.ic2 + g * v1;
        switch (nl) {
        case 1: s.ic1 = 2.f * v1 - s.ic1; s.ic1 = s.ic1 / std::sqrt(1.f + 0.08f * s.ic1 * s.ic1); break;
        case 2: s.ic1 = 2.f * 2.5f * std::tanh(v1 / 2.5f) - s.ic1; break;
        case 3: s.ic1 = clampf(2.f * v1 - s.ic1, -1.0f, 1.0f); break;
        default: s.ic1 = 2.f * v1 - s.ic1; break;
        }
        s.ic2 = 2.f * v2 - s.ic2;
    }

    // type: Shape; stages: 1, 2, 4 or 0 (6 dB/oct); eqGain: linear gain for bell and shelves
    inline float process(float x, float g, float reso, int type, int stages, int style, float eqGain = 1.f) {
        const StyleDef& sd = styleDef(style);
        if (type <= kHighPass && stages > 0 && sd.in == 0 && sd.scoop == 0) {
            // the 2.x path, unchanged
            if (sd.drive != 1.f) {
                if (sd.nl == 3) x = (x * sd.drive) / (1.f + std::fabs(x * sd.drive)) / sd.drive * 1.1f;
                else            x = std::tanh(x * sd.drive) / sd.drive * 1.1f;
            }
            const float* kb = bwK(stages);
            const float r = std::pow(clampf(reso, 0.f, 1.f), 0.6f);
            if (sd.bassLoss > 0.f) x *= 1.f - sd.bassLoss * r;
            for (int i = 0; i < stages; ++i) {
                const bool last = i == stages - 1;
                const float k = last ? kb[i] * (1.f - r) + sd.kMin * r : kb[i];
                Stage& s = st[i];
                const float a1 = 1.f / (1.f + g * (g + k));
                const float v3 = x - s.ic2;
                const float v1 = a1 * s.ic1 + g * a1 * v3;
                const float v2 = s.ic2 + g * v1;
                switch (sd.nl) {
                case 1: s.ic1 = 2.f * v1 - s.ic1; s.ic1 = s.ic1 / std::sqrt(1.f + 0.08f * s.ic1 * s.ic1); break;
                case 2: s.ic1 = 2.f * 2.5f * std::tanh(v1 / 2.5f) - s.ic1; break;
                case 3: s.ic1 = clampf(2.f * v1 - s.ic1, -1.0f, 1.0f); break;
                default: s.ic1 = 2.f * v1 - s.ic1; break;
                }
                s.ic2 = 2.f * v2 - s.ic2;
                if (type == 0)      x = v2;                                   // low pass
                else if (type == 2) x = x - k * v1 - v2;                      // high pass
                else                x = v1 * std::max(std::fabs(k), 0.12f);   // band pass, ~unity peak
            }
            return x;
        }
        x = input(x, sd);
        const float r = std::pow(clampf(reso, 0.f, 1.f), 0.6f);
        if (sd.bassLoss > 0.f && type <= kHighPass) x *= 1.f - sd.bassLoss * r;
        bool wide = false;
        if (stages == 0) {
            if (type == kLowPass || type == kHighPass || type == kLowShelf || type == kHighShelf || type == kAllPass) {
                // 6 dB/oct: one-pole TPT section (no resonance)
                const float G = g / (1.f + g), v = (x - z1) * G, lo = v + z1;
                z1 = lo + v;
                const float hi = x - lo;
                switch (type) {
                case kLowPass: return lo;
                case kHighPass: return hi;
                case kLowShelf: return x + (eqGain - 1.f) * lo;
                case kHighShelf: return x + (eqGain - 1.f) * hi;
                default: return lo - hi;            // first-order all pass
                }
            }
            wide = type == kBell;                    // a bell at 6 dB/oct is the widest bell; band pass and notch have no 6 dB/oct
        }
        if (stages == 0) stages = 1;
        if (type <= kHighPass) {
            const float* kb = bwK(stages);
            for (int i = 0; i < stages; ++i) {
                const bool last = i == stages - 1;
                const float k = last ? kb[i] * (1.f - r) + sd.kMin * r : kb[i];
                float v1, v2; svf(st[i], x, g, k, sd.nl, v1, v2);
                float y;
                if (type == kLowPass)       y = v2;
                else if (type == kHighPass) y = x - k * v1 - v2;
                else                        y = v1 * std::max(std::fabs(k), 0.12f);
                if (last && sd.scoop > 0.f) y -= sd.scoop * r * v1 * std::max(std::fabs(k), 0.12f);
                x = y;
            }
            return x;
        }
        // EQ shapes: every stage at the same frequency; the gain is shared between the stages, so more stages make the band narrower
        const float k = std::max(eqDamping(reso) * (wide ? 2.f : 1.f) + std::min(0.f, sd.kMin) * r, 0.02f);
        const float gs = stages == 1 ? eqGain : std::pow(eqGain, 1.f / stages);
        for (int i = 0; i < stages; ++i) {
            float v1, v2; svf(st[i], x, g, k, sd.nl, v1, v2);
            const float hp = x - k * v1 - v2;
            switch (type) {
            case kBell:      x = x + (gs - 1.f) * k * v1; break;
            case kLowShelf:  x = x + (gs - 1.f) * v2; break;
            case kHighShelf: x = x + (gs - 1.f) * hp; break;
            case kNotch:     x = x - k * v1; break;
            default:         x = x - 2.f * k * v1; break;        // all pass
            }
        }
        return x;
    }
};

// ------------------------------------------------------------------ classic (2.x) sources
struct ClassicAdsr {
    enum Stage { Idle, Attack, Decay, Sustain, Release } stage = Idle;
    float value = 0.f, s = 0.5f, sr = 48000.f, coefA = 0, coefD = 0, coefR = 0;
    static float coef(float t, float sr) { return 1.f - std::exp(-1.f / std::max(t * sr, 1.f)); }
    void set(float A, float D, float S, float R) { s = S; coefA = coef(A / 1.5f, sr); coefD = coef(D / 4.f, sr); coefR = coef(R / 4.f, sr); }
    void gate(bool on) { stage = on ? Attack : (stage == Idle ? Idle : Release); }
    bool gated() const { return stage == Attack || stage == Decay || stage == Sustain; }
    float process() {
        switch (stage) {
        case Attack:  value += (1.2f - value) * coefA; if (value >= 1.f) { value = 1.f; stage = Decay; } break;
        case Decay:   value += (s - value) * coefD; if (std::fabs(value - s) < 1e-4f) { value = s; stage = Sustain; } break;
        case Sustain: value = s; break;
        case Release: value += -value * coefR; if (value < 1e-5f) { value = 0.f; stage = Idle; } break;
        case Idle:    value = 0.f; break;
        }
        return value;
    }
};
struct ClassicLfo {
    double phase = 0; float out = 0;
    static float shape(float t, int wave, float bal) {
        bal = clampf(bal, 0.02f, 0.98f);
        if (wave == 1) return t < bal ? 1.f : -1.f;                                  // square (duty)
        const float u = t < bal ? 0.5f * t / bal : 0.5f + 0.5f * (t - bal) / (1.f - bal);
        return u < 0.5f ? -1.f + 4.f * u : 3.f - 4.f * u;                           // triangle (ramps)
    }
};

// ------------------------------------------------------------------ engine
class Engine {
public:
    float p[kParamCount];
    // live values for the UI (direct access)
    float srcVal[kNumSources] = {};
    float modOff[kNumDests] = {};
    float classicVal[kNumClassicSources] = {};

    Engine() {
        for (uint32_t i = 0; i < kParamCount; ++i) p[i] = paramRange(i).def;
        for (int f = 0; f < kNumFilters; ++f) for (int k = 0; k < kFBaseFields; ++k) p[fpar(f, k)] = 0;
        // filter 1: Monolith low-pass 24 dB
        p[kF1On] = 1; p[kF1Style] = 3; p[kF1Type] = 0; p[kF1Slope] = 1; p[kF1Freq] = 1000; p[kF1Peak] = 0.3f;
        // filter 2: off, clean high-pass 12 dB
        p[kF2On] = 0; p[kF2Style] = 0; p[kF2Type] = 2; p[kF2Slope] = 0; p[kF2Freq] = 200; p[kF2Peak] = 0.2f;
        // filters 3 and 4: off, clean band pass and low pass
        p[fpar(2, kFStyle)] = 0; p[fpar(2, kFType)] = kBandPass; p[fpar(2, kFSlope)] = kSlope12; p[fpar(2, kFFreq)] = 2000; p[fpar(2, kFPeak)] = 0.3f;
        p[fpar(3, kFStyle)] = 0; p[fpar(3, kFType)] = kLowPass; p[fpar(3, kFSlope)] = kSlope12; p[fpar(3, kFFreq)] = 6000; p[fpar(3, kFPeak)] = 0.2f;
        p[kRouting] = 0; p[kInGain] = 0; p[kOutGain] = 0; p[kMix] = 1; p[kAutoMute] = 1;
        p[kL1Wave] = 0; p[kL1Balance] = 0.5f; p[kL1Sync] = 0; p[kL1Rate] = 1; p[kL1Div] = 4; p[kL1Midi] = 0;
        p[kL2Wave] = 1; p[kL2Balance] = 0.5f; p[kL2Sync] = 0; p[kL2Rate] = 4; p[kL2Div] = 6; p[kL2Midi] = 0;
        p[kEgAttack] = 0.01f; p[kEgDecay] = 0.3f; p[kEgSustain] = 0.5f; p[kEgRelease] = 0.3f; p[kEgThreshold] = -30; p[kEgMidi] = 0;
        for (int m = 0; m < 18; ++m) p[kModFirst + m] = 0;
        p[kMidiOn] = 1; p[kOutEg] = 0; p[kOutF1Live] = 1000; p[kOutF2Live] = 200; p[kOutLearn] = 0;
        for (int l = 0; l < kNumXlfo; ++l) for (int s = 0; s < kNumSteps; ++s) p[lp_(l, kLStep1 + s)] = (s % 4) / 1.5f - 1.f;
        snapSmoothing();
    }
    static const Layout& layout() { static const Layout L = { kXlfoFirst, kEgFirst, kEfFirst, kXyFirst, kSlotFirst, kNumSlots, kDstSlotFirst, kNumDests }; return L; }
    void setSlot(int s, int src, int dst, float amt) { p[sp(s, kSlotSrc)] = (float)src; p[sp(s, kSlotDst)] = (float)dst; p[sp(s, kSlotAmt)] = amt; p[sp(s, kSlotBypass)] = 0; }

    void setSampleRate(float sr) {
        fs = sr; eg.sr = sr;
        smooth = 1.f - std::exp(-1.f / (0.010f * sr));
        sqSmooth = 1.f - std::exp(-1.f / (0.001f * sr));
        followAtk = 1.f - std::exp(-1.f / (0.001f * sr));
        followRel = 1.f - std::exp(-1.f / (0.050f * sr));
        gateAtk = 1.f - std::exp(-1.f / (0.005f * sr));
        gateRel = 1.f - std::exp(-1.f / (0.150f * sr));
        int n = 64; while (n < (int)(0.051f * sr * 4) + 8) n <<= 1;
        for (auto& f : dly) for (auto& c : f) c.assign(n, 0.f);
        int dn = 64; while (dn < OversamplerN::latency(4) + 8) dn <<= 1;
        for (auto& d : dry) d.assign(dn, 0.f);
        reset();
    }
    void reset() {
        for (auto& f : flt) for (auto& c : f) c.reset();
        for (auto& l : lfo) { l.phase = 0; l.out = 0; }
        eg.stage = ClassicAdsr::Idle; eg.value = 0; level = 0; muteGain = 1; silentFor = 0; held = 0;
        os = p[kHq] > 0.5f ? 4 : 1; fsOs = fs * os;
        for (auto& o : ovs) o.reset();
        for (auto& f : dly) for (auto& c : f) std::fill(c.begin(), c.end(), 0.f);
        for (auto& d : dry) std::fill(d.begin(), d.end(), 0.f);
        dlyPos = 0; dryPos = 0; ctl = 0; fadeState = 0; fadeGain = 1;
        mods.resetFollowers();
        std::fill(modOff, modOff + kNumDests, 0.f); anyMod = false;
        control();
        snapSmoothing();
    }
    void setTransport(bool valid, bool playing, double barPos, double bpm, double beatsPerBar) {
        tValid = valid; tPlaying = playing; tBarPos = barPos;
        tBpm = bpm > 1 ? bpm : 120.0; tBeatsPerBar = beatsPerBar > 0 ? beatsPerBar : 4.0;
    }
    uint32_t latency() const { return (uint32_t)OversamplerN::latency(os); }

    // MIDI (ignored entirely when the MIDI switch is off)
    void noteOn(int note, int vel = 100) {
        if (p[kMidiOn] < 0.5f) return;
        if (held < 16) stack[held++] = note;
        lastNote = note;
        for (int i = 0; i < 2; ++i) if (p[i == 0 ? kL1Midi : kL2Midi] > 0.5f) lfo[i].phase = 0;
        if (p[kEgMidi] > 0.5f) eg.gate(true);
        mods.noteOn(p, layout(), note, vel);
    }
    void noteOff(int note) {
        if (p[kMidiOn] < 0.5f) return;
        int j = 0; for (int i = 0; i < held; ++i) if (stack[i] != note) stack[j++] = stack[i];
        held = j;
        if (held > 0) lastNote = stack[held - 1];
        else if (p[kEgMidi] > 0.5f) eg.gate(false);
        mods.noteOff(p, layout(), note);
    }
    void allNotesOff() { held = 0; if (p[kEgMidi] > 0.5f) eg.gate(false); mods.allNotesOff(p, layout()); }
    void pitchBend(int v14) { if (p[kMidiOn] > 0.5f) mods.pitchBend(v14); }
    void modWheel(int v) { if (p[kMidiOn] > 0.5f) mods.modWheel(v); }
    void channelPressure(int v) { if (p[kMidiOn] > 0.5f) mods.channelPressure(v); }
    void polyPressure(int note, int v) { if (p[kMidiOn] > 0.5f) mods.polyPressure(note, v); }

    // read-back for the UI
    float egValue() const { return eg.value; }
    bool egGated() const { return eg.gated(); }
    float liveFreq(int f) const { return liveHz[f]; }
    double xlfoPhase(int l) const { return mods.xlfo[l].phase; }
    double classicPhase(int l) const { return lfo[l].phase; }
    // effective (modulated) value of a parameter
    float eff(uint32_t q) const { const int d = destForParam(q); return (d > 0 && d < kDstSlotFirst && modOff[d] != 0.f) ? valueOf(q, normOf(q, p[q]) + modOff[d]) : p[q]; }

    void process(const float* inL, const float* inR, float* outL, float* outR, uint32_t n) { process(inL, inR, nullptr, nullptr, outL, outR, n); }
    void process(const float* inL, const float* inR, const float* scL, const float* scR, float* outL, float* outR, uint32_t n) {
        eg.set(p[kEgAttack], p[kEgDecay], p[kEgSustain], p[kEgRelease]);
        const bool egMidi = p[kEgMidi] > 0.5f;
        const float thr = dbToGain(p[kEgThreshold]);
        const bool autoMute = p[kAutoMute] > 0.5f;
        const int wantOs = p[kHq] > 0.5f ? 4 : 1;
        if (wantOs != os && fadeState == 0) fadeState = 1;

        double lfoInc[2]; bool lfoLock[2];
        for (int i = 0; i < 2; ++i) {
            const uint32_t b = i == 0 ? kL1Wave : kL2Wave;
            const int sync = (int)(p[b + 2] + 0.5f);
            const bool retrig = p[b + 5] > 0.5f;
            if (sync == 0) { lfoInc[i] = p[b + 3] / fs; lfoLock[i] = false; continue; }
            double bars = divBars((int)(p[b + 4] + 0.5f));
            if (sync == 2) bars *= 1.5;          // dotted
            if (sync == 3) bars *= 2.0 / 3.0;    // triplet
            lfoInc[i] = tBpm / (60.0 * fs * tBeatsPerBar) / bars;
            lfoLock[i] = !retrig && tValid && tPlaying;
            if (lfoLock[i]) { double ph = tBarPos / bars; lfo[i].phase = ph - std::floor(ph); }
        }

        int fType[kNumFilters], fStages[kNumFilters], fStyle[kNumFilters]; bool fOn[kNumFilters], fMidi[kNumFilters];
        for (int f = 0; f < kNumFilters; ++f) {
            fOn[f] = p[fpar(f, kFOn)] > 0.5f; fStyle[f] = (int)(p[fpar(f, kFStyle)] + 0.5f); fType[f] = (int)(p[fpar(f, kFType)] + 0.5f);
            fStages[f] = slopeStages((int)(p[fpar(f, kFSlope)] + 0.5f));
            fMidi[f] = p[fpar(f, kFMidi)] > 0.5f && p[kMidiOn] > 0.5f;
        }
        const int route = (int)clampf(std::round(p[kRouting]), 0, kNumRoutings - 1);
        const int chan = (int)clampf(std::round(p[kChanMode]), 0, kNumChanModes - 1);
        const bool legacy = !fOn[2] && !fOn[3] && route <= kParallel && chan == kStereo && os == 1 && !anyMod && plainFilter(0) && plainFilter(1) && p[kInPan] == 0.f;

        for (uint32_t i = 0; i < n; ++i) {
            if (--ctl <= 0) { ctl = kCtl; control(); }
            const float xl = inL[i], xr = inR[i];

            // smoothed controls
            for (int f = 0; f < kNumFilters; ++f) {
                float tgtOct = std::log2(clampf(mv(f, kFFreq), 10.f, 22000.f));
                if (fMidi[f] && lastNote >= 0) tgtOct += (lastNote - 60) / 12.f;
                sOct[f]  += (tgtOct - sOct[f]) * smooth;
                sPeak[f] += (mv(f, kFPeak) - sPeak[f]) * smooth;
                sPan[f]  += (mv(f, kFPan) - sPan[f]) * smooth;
            }
            sIn  += (dbToGain(mvp(kInGain, kDstInGain)) - sIn) * smooth;
            sOut += (dbToGain(mvp(kOutGain, kDstOutGain)) - sOut) * smooth;
            sMix += (mvp(kMix, kDstMix) - sMix) * smooth;

            // input level follower (pre-gain) drives the EG trigger and the auto-mute
            const float a = std::max(std::fabs(xl), std::fabs(xr));
            level += (a - level) * (a > level ? followAtk : followRel);
            if (!egMidi) {
                if (!eg.gated() && level > thr) eg.gate(true);
                else if (eg.gated() && level < thr * 0.7f) eg.gate(false);
            }
            mods.follow(a, (scL && scR) ? std::max(std::fabs(scL[i]), std::fabs(scR[i])) : 0.f);

            // classic modulation sources
            float src[kNumClassicSources];
            for (int l = 0; l < 2; ++l) {
                const uint32_t b = l == 0 ? kL1Wave : kL2Wave;
                lfo[l].phase += lfoInc[l]; if (lfo[l].phase >= 1.0) lfo[l].phase -= std::floor(lfo[l].phase);
                const int wave = (int)(p[b] + 0.5f);
                const float raw = ClassicLfo::shape((float)lfo[l].phase, wave, p[b + 1]);
                lfo[l].out += (raw - lfo[l].out) * (wave == 1 ? sqSmooth : 1.f);
                src[l] = lfo[l].out;
            }
            src[kSrcEg] = eg.process();
            for (int s = 0; s < kNumClassicSources; ++s) classicVal[s] = src[s];

            float dst[kNumClassicDests] = {};
            for (int s = 0; s < kNumClassicSources; ++s)
                for (int d = 0; d < kNumClassicDests; ++d) {
                    const float amt = p[modParam(s, d)];
                    if (amt != 0.f) dst[d] += amt * src[s];
                }

            // auto-mute: fade the wet signal out when the input has been silent (stops self-oscillation)
            if (autoMute) {
                if (level < 1e-4f) silentFor++; else silentFor = 0;
                const float tgt = silentFor > (uint32_t)(0.05f * fs) ? 0.f : 1.f;
                muteGain += (tgt - muteGain) * (tgt > muteGain ? gateAtk : gateRel);
            } else muteGain = 1.f;

            if (legacy) {
                // the 2.x signal path (two filters, stereo, no oversampling), kept sample-exact
                const float gin = sIn;
                float wl = xl * gin, wr = xr * gin;
                float yl[2], yr[2];
                const bool parallel = route == kParallel;
                for (int f = 0; f < 2; ++f) {
                    const float oct = sOct[f] + 4.f * dst[f * 3 + 0];
                    const float peak = clampf(sPeak[f] + dst[f * 3 + 1], 0.f, 1.f);
                    const float pan = clampf(sPan[f] + dst[f * 3 + 2], -1.5f, 1.5f);
                    const float fcL = clampf(std::pow(2.f, oct - 2.f * pan), 10.f, 0.45f * fs);
                    const float fcR = clampf(std::pow(2.f, oct + 2.f * pan), 10.f, 0.45f * fs);
                    liveHz[f] = std::pow(2.f, oct);
                    if (!fOn[f]) { yl[f] = yr[f] = 0; continue; }
                    const bool fromF1 = !parallel && f == 1 && fOn[0];
                    const float inLf = fromF1 ? yl[0] : wl;
                    const float inRf = fromF1 ? yr[0] : wr;
                    yl[f] = flt[f][0].process(inLf, std::tan(kPi * fcL / fs), peak, fType[f], fStages[f], fStyle[f]);
                    yr[f] = flt[f][1].process(inRf, std::tan(kPi * fcR / fs), peak, fType[f], fStages[f], fStyle[f]);
                }
                for (int f = 2; f < kNumFilters; ++f) liveHz[f] = std::pow(2.f, sOct[f]);
                float ol, orr;
                if (parallel) {
                    if (fOn[0] && fOn[1]) { ol = 0.7071f * (yl[0] + yl[1]); orr = 0.7071f * (yr[0] + yr[1]); }
                    else if (fOn[0])      { ol = yl[0]; orr = yr[0]; }
                    else if (fOn[1])      { ol = yl[1]; orr = yr[1]; }
                    else                  { ol = wl; orr = wr; }
                } else {
                    if (fOn[1])      { ol = yl[1]; orr = yr[1]; }
                    else if (fOn[0]) { ol = yl[0]; orr = yr[0]; }
                    else             { ol = wl; orr = wr; }
                }
                const float wet = muteGain;
                float outl = (sMix * ol * wet + (1.f - sMix) * xl) * sOut;
                float outr = (sMix * orr * wet + (1.f - sMix) * xr) * sOut;
                outL[i] = softCeil(outl);
                outR[i] = softCeil(outr);
                continue;
            }

            // ---- 3.0 signal path ----
            // per-filter values for this sample (cutoffs per channel, peak, drive, gain, delay, level, pan)
            FiltNow fn[kNumFilters];
            const bool sep = chan != kStereo;           // left/right or mid/side: no frequency or output panning
            for (int f = 0; f < kNumFilters; ++f) {
                const float oct = sOct[f] + (f < 2 ? 4.f * dst[f * 3 + 0] : 0.f);
                const float peak = clampf(sPeak[f] + (f < 2 ? dst[f * 3 + 1] : 0.f), 0.f, 1.f);
                const float pan = sep ? 0.f : clampf(sPan[f] + (f < 2 ? dst[f * 3 + 2] : 0.f), -1.5f, 1.5f);
                liveHz[f] = std::pow(2.f, oct);
                if (!fOn[f]) continue;
                FiltSmooth& s = fs_[f];
                s.drive += (dbToGain(mv(f, kFDrive)) - s.drive) * smooth;
                // linear styles: drive is fully compensated; the saturating ones get louder more slowly than they are driven
                { const StyleDef& sd = styleDef(fStyle[f]); s.makeup = (sd.drive == 1.f && sd.nl == 0) ? 1.f / s.drive : std::pow(s.drive, -0.5f); }
                s.gain += (dbToGain(mv(f, kFEqGain)) - s.gain) * smooth;
                s.delay += (mv(f, kFDelay) * 0.001f * fsOs - s.delay) * smooth;
                s.level += (dbToGain(mv(f, kFLevel)) - s.level) * smooth;
                const float op = sep ? 0.f : clampf(mv(f, kFOutPan), -1.f, 1.f);
                s.panL += ((op > 0 ? 1.f - op : 1.f) - s.panL) * smooth; s.panR += ((op < 0 ? 1.f + op : 1.f) - s.panR) * smooth;
                const float lim = 0.45f * fsOs;
                fn[f].g[0] = std::tan(kPi * clampf(std::pow(2.f, oct - 2.f * pan), 10.f, std::min(lim, 0.49f * fsOs)) / fsOs);
                fn[f].g[1] = std::tan(kPi * clampf(std::pow(2.f, oct + 2.f * pan), 10.f, std::min(lim, 0.49f * fsOs)) / fsOs);
                fn[f].peak = peak;
            }
            const float inBalL = p[kInPan] > 0 ? 1.f - p[kInPan] : 1.f, inBalR = p[kInPan] < 0 ? 1.f + p[kInPan] : 1.f;
            float x[2] = { xl * sIn * inBalL, xr * sIn * inBalR };
            if (chan == kMidSide) { const float m = 0.5f * (x[0] + x[1]), sd = 0.5f * (x[0] - x[1]); x[0] = m; x[1] = sd; }

            // dry copy, delayed by the oversampling latency, for the mix
            const int lat = (int)latency(), dm = (int)dry[0].size() - 1;
            float dryS[2];
            dry[0][dryPos] = xl; dry[1][dryPos] = xr;
            dryS[0] = dry[0][(dryPos - lat + dm + 1) & dm]; dryS[1] = dry[1][(dryPos - lat + dm + 1) & dm];
            dryPos = (dryPos + 1) & dm;

            float up[2][4], y[2][4];
            if (os == 1) { up[0][0] = x[0]; up[1][0] = x[1]; } else { ovs[0].up(x[0], os, up[0]); ovs[1].up(x[1], os, up[1]); }
            for (int k = 0; k < os; ++k) {
                if (!sep) render(fn, fOn, fType, fStages, fStyle, route, up[0][k], up[1][k], y[0][k], y[1][k]);
                else {
                    // left/mid through filters 1 and 2, right/side through filters 3 and 4
                    float dummy;
                    renderGroup(fn, fOn, fType, fStages, fStyle, route, 0, up[0][k], y[0][k], dummy);
                    renderGroup(fn, fOn, fType, fStages, fStyle, route, 1, up[1][k], dummy, y[1][k]);
                }
                dlyPos = (dlyPos + 1) & ((int)dly[0][0].size() - 1);
            }
            float o[2];
            if (os == 1) { o[0] = y[0][0]; o[1] = y[1][0]; } else { o[0] = ovs[0].down(y[0], os); o[1] = ovs[1].down(y[1], os); }
            if (chan == kMidSide) { const float l = o[0] + o[1], r = o[0] - o[1]; o[0] = l; o[1] = r; }
            const float wet = muteGain * fadeGain;
            outL[i] = softCeil((sMix * o[0] * wet + (1.f - sMix) * dryS[0]) * sOut);
            outR[i] = softCeil((sMix * o[1] * wet + (1.f - sMix) * dryS[1]) * sOut);
            if (fadeState == 1) { fadeGain -= 1.f / 128.f; if (fadeGain <= 0) { fadeGain = 0; switchOs(); fadeState = 2; } }
            else if (fadeState == 2) { fadeGain += 1.f / 128.f; if (fadeGain >= 1) { fadeGain = 1; fadeState = 0; } }
        }
        if (fadeState == 1 && os == wantOs) fadeState = 0;
    }

private:
    struct FiltNow { float g[2] = { 0, 0 }, peak = 0; };
    struct FiltSmooth { float drive = 1, makeup = 1, gain = 1, delay = 0, level = 1, panL = 1, panR = 1; };

    // true when a filter uses nothing new in 3.0 (so the 2.x path applies)
    bool plainFilter(int f) const {
        if (p[fpar(f, kFOn)] < 0.5f) return true;
        const int st = (int)(p[fpar(f, kFStyle)] + 0.5f), ty = (int)(p[fpar(f, kFType)] + 0.5f), sl = (int)(p[fpar(f, kFSlope)] + 0.5f);
        return st < kNumStyles2 && ty <= kHighPass && sl <= kSlope48 && p[fpar(f, kFDrive)] == 0.f && p[fpar(f, kFDelay)] == 0.f
            && p[fpar(f, kFLevel)] == 0.f && p[fpar(f, kFOutPan)] == 0.f;
    }
    // one filter on one channel, including drive, delay, level and pan
    inline float runFilter(int f, int c, float in, const FiltNow* fn, const int* fType, const int* fStages, const int* fStyle) {
        const FiltSmooth& s = fs_[f];
        float y = flt[f][c].process(in * s.drive, fn[f].g[c], fn[f].peak, fType[f], fStages[f], fStyle[f], s.gain) * s.makeup;
        std::vector<float>& d = dly[f][c];
        const int m = (int)d.size() - 1;
        d[dlyPos] = y;
        if (s.delay > 0.01f) {
            const float dp = std::min(s.delay, (float)(m - 2)); const int di = (int)dp; const float fr = dp - di;
            const float a = d[(dlyPos - di + m + 1) & m], b = d[(dlyPos - di - 1 + m + 1) & m];
            y = a + (b - a) * fr;
        }
        return y * s.level;
    }
    // chain or sum a set of filters (indices) on channel c; returns false if none is on
    inline bool chain(const int* idx, int n, bool serial, int c, float in, float& out, const FiltNow* fn, const bool* fOn, const int* fType, const int* fStages, const int* fStyle) {
        int cnt = 0; float acc = 0, x = in;
        for (int j = 0; j < n; ++j) {
            const int f = idx[j]; if (!fOn[f]) continue;
            const float y = runFilter(f, c, serial ? x : in, fn, fType, fStages, fStyle);
            if (serial) x = y; else acc += y;
            ++cnt;
        }
        if (!cnt) { out = in; return false; }
        out = serial ? x : acc * (cnt == 2 ? 0.7071f : 1.f / std::sqrt((float)cnt));
        return true;
    }
    // stereo: all four filters on both channels, with output panning
    inline void render(const FiltNow* fn, const bool* fOn, const int* fType, const int* fStages, const int* fStyle, int route, float inL, float inR, float& outL, float& outR) {
        static const int all[4] = { 0, 1, 2, 3 }, pa[2] = { 0, 1 }, pb[2] = { 2, 3 };
        float o[2];
        for (int c = 0; c < 2; ++c) {
            const float in = c ? inR : inL;
            float a, b;
            switch (route) {
            case kSerial:   chainPan(all, 4, true, c, in, o[c], fn, fOn, fType, fStages, fStyle); break;
            case kParallel: chainPan(all, 4, false, c, in, o[c], fn, fOn, fType, fStages, fStyle); break;
            case kPairsParallel: {
                const bool ha = chainPan(pa, 2, true, c, in, a, fn, fOn, fType, fStages, fStyle), hb = chainPan(pb, 2, true, c, in, b, fn, fOn, fType, fStages, fStyle);
                o[c] = ha && hb ? 0.7071f * (a + b) : (ha ? a : (hb ? b : in)); break; }
            default: {
                chainPan(pa, 2, false, c, in, a, fn, fOn, fType, fStages, fStyle);
                chainPan(pb, 2, false, c, a, o[c], fn, fOn, fType, fStages, fStyle); break; }
            }
        }
        outL = o[0]; outR = o[1];
    }
    // as chain, applying each filter's output pan for channel c
    inline bool chainPan(const int* idx, int n, bool serial, int c, float in, float& out, const FiltNow* fn, const bool* fOn, const int* fType, const int* fStages, const int* fStyle) {
        int cnt = 0; float acc = 0, x = in;
        for (int j = 0; j < n; ++j) {
            const int f = idx[j]; if (!fOn[f]) continue;
            const float y = runFilter(f, c, serial ? x : in, fn, fType, fStages, fStyle) * (c ? fs_[f].panR : fs_[f].panL);
            if (serial) x = y; else acc += y;
            ++cnt;
        }
        if (!cnt) { out = in; return false; }
        out = serial ? x : acc * (cnt == 2 ? 0.7071f : 1.f / std::sqrt((float)cnt));
        return true;
    }
    // left/right or mid/side: group 0 (filters 1, 2) on channel 0, group 1 (filters 3, 4) on channel 1
    inline void renderGroup(const FiltNow* fn, const bool* fOn, const int* fType, const int* fStages, const int* fStyle, int route, int group, float in, float& out0, float& out1) {
        static const int pa[2] = { 0, 1 }, pb[2] = { 2, 3 };
        const bool serial = route == kSerial || route == kPairsParallel;
        float y; chain(group ? pb : pa, 2, serial, group, in, y, fn, fOn, fType, fStages, fStyle);
        (group ? out1 : out0) = y;
    }

    // modulated value of a filter field / of a global parameter (v2 matrix)
    inline float mv(int f, int field) const {
        const uint32_t q = fpar(f, field);
        if (!anyMod) return p[q];
        int k = 0; while (k < kNumFilterDests && kFilterDestFields[k] != field) ++k;
        if (k == kNumFilterDests) return p[q];
        const float o = modOff[kDstFilterFirst + f * kNumFilterDests + k];
        return o == 0.f ? p[q] : valueOf(q, normOf(q, p[q]) + o);
    }
    inline float mvp(uint32_t q, int d) const { const float o = anyMod ? modOff[d] : 0.f; return o == 0.f ? p[q] : valueOf(q, normOf(q, p[q]) + o); }

    // control rate: v2 sources and matrix
    void control() {
        const float cr = fs / kCtl;
        // the sources read XLFO rate and balance and the EG stages modulated (effv), everything else as set (p)
        std::copy(p + kXlfoFirst, p + kSlotFirst, effv + kXlfoFirst);
        for (int l = 0; l < kNumXlfo; ++l) { effv[lp_(l, kLRate)] = mvp(lp_(l, kLRate), kDstXlfoRateFirst + l); effv[lp_(l, kLBalance)] = mvp(lp_(l, kLBalance), kDstXlfoBalFirst + l); }
        static const int egf[4] = { kEAttack, kEDecay, kESustain, kERelease };
        for (int e = 0; e < kNumEg; ++e) for (int k = 0; k < 4; ++k) effv[ep(e, egf[k])] = mvp(ep(e, egf[k]), kDstEgFirst + e * 4 + k);
        mods.tick(p, effv, layout(), fs, cr, tValid, tPlaying, tBarPos, tBpm);
        std::copy(mods.val, mods.val + kNumSources, srcVal);
        matrix(p, layout(), srcVal, modOff);
        anyMod = false; for (int d = 1; d < kDstSlotFirst; ++d) if (modOff[d] != 0.f) { anyMod = true; break; }
    }
    void switchOs() {
        os = p[kHq] > 0.5f ? 4 : 1; fsOs = fs * os;
        for (auto& o : ovs) o.reset();
        for (auto& f : flt) for (auto& c : f) c.reset();
        for (auto& f : dly) for (auto& c : f) std::fill(c.begin(), c.end(), 0.f);
        for (auto& d : dry) std::fill(d.begin(), d.end(), 0.f);
        for (FiltSmooth& s : fs_) s.delay = 0;
    }
    static float softCeil(float x) {        // transparent below -1 dBFS, soft ceiling above
        const float a = std::fabs(x);
        if (!(a < 1e6f)) return 0.f;        // NaN/Inf guard
        return a > 0.89f ? std::copysign(0.89f + 0.11f * std::tanh((a - 0.89f) / 0.11f), x) : x;
    }
    void snapSmoothing() {
        for (int f = 0; f < kNumFilters; ++f) {
            sOct[f] = std::log2(clampf(p[fpar(f, kFFreq)], 10.f, 22000.f)); sPeak[f] = p[fpar(f, kFPeak)]; sPan[f] = p[fpar(f, kFPan)];
            FiltSmooth& s = fs_[f];
            s.drive = dbToGain(p[fpar(f, kFDrive)]); s.makeup = 1.f / s.drive; s.gain = dbToGain(p[fpar(f, kFEqGain)]); s.delay = p[fpar(f, kFDelay)] * 0.001f * fsOs; s.level = dbToGain(p[fpar(f, kFLevel)]);
            const float op = p[fpar(f, kFOutPan)]; s.panL = op > 0 ? 1.f - op : 1.f; s.panR = op < 0 ? 1.f + op : 1.f;
        }
        sIn = dbToGain(p[kInGain]); sOut = dbToGain(p[kOutGain]); sMix = p[kMix];
    }

    FilterChan flt[kNumFilters][2];
    ClassicLfo lfo[2];
    ClassicAdsr eg;
    Sources mods;
    float effv[kParamCount] = {};
    bool anyMod = false;
    FiltSmooth fs_[kNumFilters];
    std::vector<float> dly[kNumFilters][2]; int dlyPos = 0;
    std::vector<float> dry[2]; int dryPos = 0;
    OversamplerN ovs[2]; int os = 1; float fsOs = 48000.f;
    int ctl = 0, fadeState = 0; float fadeGain = 1;
    float fs = 48000.f, smooth = 0.01f, sqSmooth = 0.1f, followAtk = 0.1f, followRel = 0.01f, gateAtk = 0.01f, gateRel = 0.001f;
    float sOct[kNumFilters] = {10, 10, 10, 10}, sPeak[kNumFilters] = {}, sPan[kNumFilters] = {}, sIn = 1, sOut = 1, sMix = 1;
    float level = 0, muteGain = 1, liveHz[kNumFilters] = {1000, 200, 2000, 6000};
    uint32_t silentFor = 0;
    int stack[16] = {}; int held = 0; int lastNote = -1;
    bool tValid = false, tPlaying = false;
    double tBarPos = 0, tBpm = 120, tBeatsPerBar = 4;
};

} // namespace caldera
