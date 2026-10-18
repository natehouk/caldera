// Caldera — stereo dual-filter effect engine.
// Clean-room implementation from public, generic techniques: cascaded
// topology-preserving-transform state-variable filters (Zavalishin) with
// per-style nonlinearities, Butterworth stage damping, one-pole smoothing.
// No third-party product code, assets or disassembly were used.
// License: MIT (see LICENSE).

#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace caldera {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float dbToGain(float db) { return std::pow(10.f, db / 20.f); }

// ------------------------------------------------------------------ params
// Order is part of the saved-session format: only ever append.
#define FILTER_PARAMS(n) kF##n##On, kF##n##Style, kF##n##Type, kF##n##Slope, kF##n##Freq, kF##n##Peak, kF##n##Pan, kF##n##Midi
#define LFO_PARAMS(n)    kL##n##Wave, kL##n##Balance, kL##n##Sync, kL##n##Rate, kL##n##Div, kL##n##Midi
enum Param : uint32_t {
    FILTER_PARAMS(1),
    FILTER_PARAMS(2),
    kRouting, kInGain, kOutGain, kMix, kAutoMute,
    LFO_PARAMS(1),
    LFO_PARAMS(2),
    kEgAttack, kEgDecay, kEgSustain, kEgRelease, kEgThreshold, kEgMidi,
    // modulation grid: source x destination, bipolar amounts
    kModFirst,
    kModLast = kModFirst + 3 * 6 - 1,
    kMidiOn,
    // outputs (read-only, for the UI)
    kOutEg, kOutF1Live, kOutF2Live, kOutLearn,
    kParamCount
};
#undef FILTER_PARAMS
#undef LFO_PARAMS

enum ModSource { kSrcLfo1, kSrcLfo2, kSrcEg, kNumSources };
enum ModDest   { kDstF1Freq, kDstF1Peak, kDstF1Pan, kDstF2Freq, kDstF2Peak, kDstF2Pan, kNumDests };
inline uint32_t modParam(int src, int dst) { return kModFirst + src * kNumDests + dst; }

static constexpr int kNumStyles = 7;
static constexpr int kNumDivs = 11;
inline float divBars(int i) {
    static const float t[kNumDivs] = { 16, 8, 4, 2, 1, 1.f/2, 1.f/4, 1.f/8, 1.f/16, 1.f/32, 1.f/64 };
    return t[i < 0 ? 0 : (i >= kNumDivs ? kNumDivs - 1 : i)];
}

// Per-style behaviour: input drive, stage nonlinearity and how far resonance can go.
struct StyleDef { float drive; int nl; float kMin; float bassLoss; };
//  nl: 0 linear, 1 soft-limit state (Monolith), 2 tanh integrator, 3 hard-clip state
inline const StyleDef& styleDef(int s) {
    static const StyleDef t[kNumStyles] = {
        { 1.0f, 0,  0.08f, 0.0f },   // Clean    : linear, high-Q but never self-oscillates
        { 1.0f, 0,  0.35f, 0.0f },   // Smooth   : gentle, wide resonance
        { 1.6f, 0,  0.04f, 0.0f },   // Warm     : saturated input, linear filter
        { 1.2f, 1, -0.04f, 0.0f },   // Monolith : Monolith's filter, self-oscillating
        { 2.0f, 2, -0.05f, 0.0f },   // Hot      : saturating integrators, screaming peak
        { 1.8f, 1, -0.06f, 0.5f },   // Acid     : resonance thins the bass, squelchy
        { 3.0f, 3, -0.08f, 0.0f },   // Raw      : hard-clipped feedback, buzzy self-oscillation
    };
    return t[s < 0 ? 0 : (s >= kNumStyles ? kNumStyles - 1 : s)];
}

// Butterworth stage damping (k = 2*cos(theta)) for 1, 2 and 4 cascaded 2-pole stages.
inline const float* bwK(int stages) {
    static const float k1[1] = { 1.41421356f };
    static const float k2[2] = { 1.84775907f, 0.76536686f };
    static const float k4[4] = { 1.96157056f, 1.66293922f, 1.11114047f, 0.39018064f };
    return stages == 1 ? k1 : (stages == 2 ? k2 : k4);
}

struct Stage { float ic1 = 0, ic2 = 0; };

// One filter, one channel: up to 4 cascaded TPT SVF stages.
struct FilterChan {
    Stage st[4];
    void reset() { for (Stage& s : st) s.ic1 = s.ic2 = 0; }

    inline float process(float x, float g, float reso, int type, int stages, int style) {
        const StyleDef& sd = styleDef(style);
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
};

// ------------------------------------------------------------------ ADSR
struct ADSR {
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

// ------------------------------------------------------------------ LFO
struct LFO {
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

    Engine() {
        std::fill(p, p + kParamCount, 0.f);
        // filter 1: Monolith low-pass 24 dB
        p[kF1On] = 1; p[kF1Style] = 3; p[kF1Type] = 0; p[kF1Slope] = 1; p[kF1Freq] = 1000; p[kF1Peak] = 0.3f;
        // filter 2: off, clean high-pass 12 dB
        p[kF2On] = 0; p[kF2Style] = 0; p[kF2Type] = 2; p[kF2Slope] = 0; p[kF2Freq] = 200; p[kF2Peak] = 0.2f;
        p[kRouting] = 0; p[kInGain] = 0; p[kOutGain] = 0; p[kMix] = 1; p[kAutoMute] = 1;
        p[kL1Wave] = 0; p[kL1Balance] = 0.5f; p[kL1Sync] = 0; p[kL1Rate] = 1; p[kL1Div] = 4;
        p[kL2Wave] = 1; p[kL2Balance] = 0.5f; p[kL2Sync] = 0; p[kL2Rate] = 4; p[kL2Div] = 6;
        p[kEgAttack] = 0.01f; p[kEgDecay] = 0.3f; p[kEgSustain] = 0.5f; p[kEgRelease] = 0.3f; p[kEgThreshold] = -30;
        p[kMidiOn] = 1;
        snapSmoothing();
    }

    void setSampleRate(float sr) {
        fs = sr; eg.sr = sr;
        smooth = 1.f - std::exp(-1.f / (0.010f * sr));
        sqSmooth = 1.f - std::exp(-1.f / (0.001f * sr));
        followAtk = 1.f - std::exp(-1.f / (0.001f * sr));
        followRel = 1.f - std::exp(-1.f / (0.050f * sr));
        gateAtk = 1.f - std::exp(-1.f / (0.005f * sr));
        gateRel = 1.f - std::exp(-1.f / (0.150f * sr));
        reset();
    }
    void reset() {
        for (auto& f : flt) for (auto& c : f) c.reset();
        for (auto& l : lfo) { l.phase = 0; l.out = 0; }
        eg.stage = ADSR::Idle; eg.value = 0; level = 0; muteGain = 1; silentFor = 0; held = 0;
        snapSmoothing();
    }
    void setTransport(bool valid, bool playing, double barPos, double bpm, double beatsPerBar) {
        tValid = valid; tPlaying = playing; tBarPos = barPos;
        tBpm = bpm > 1 ? bpm : 120.0; tBeatsPerBar = beatsPerBar > 0 ? beatsPerBar : 4.0;
    }

    // MIDI (ignored entirely when the MIDI switch is off)
    void noteOn(int note) {
        if (p[kMidiOn] < 0.5f) return;
        if (held < 16) stack[held++] = note;
        lastNote = note;
        for (int i = 0; i < 2; ++i) if (p[i == 0 ? kL1Midi : kL2Midi] > 0.5f) lfo[i].phase = 0;
        if (p[kEgMidi] > 0.5f) eg.gate(true);
    }
    void noteOff(int note) {
        if (p[kMidiOn] < 0.5f) return;
        int j = 0; for (int i = 0; i < held; ++i) if (stack[i] != note) stack[j++] = stack[i];
        held = j;
        if (held > 0) lastNote = stack[held - 1];
        else if (p[kEgMidi] > 0.5f) eg.gate(false);
    }
    void allNotesOff() { held = 0; if (p[kEgMidi] > 0.5f) eg.gate(false); }

    // read-back for the UI
    float egValue() const { return eg.value; }
    bool egGated() const { return eg.gated(); }
    float liveFreq(int f) const { return liveHz[f]; }

    void process(const float* inL, const float* inR, float* outL, float* outR, uint32_t n) {
        eg.set(p[kEgAttack], p[kEgDecay], p[kEgSustain], p[kEgRelease]);
        const bool parallel = p[kRouting] > 0.5f;
        const bool egMidi = p[kEgMidi] > 0.5f;
        const float thr = dbToGain(p[kEgThreshold]);
        const bool autoMute = p[kAutoMute] > 0.5f;

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

        int fType[2], fStages[2], fStyle[2]; bool fOn[2], fMidi[2];
        for (int f = 0; f < 2; ++f) {
            const uint32_t b = f == 0 ? kF1On : kF2On;
            fOn[f] = p[b] > 0.5f; fStyle[f] = (int)(p[b + 1] + 0.5f); fType[f] = (int)(p[b + 2] + 0.5f);
            const int sl = (int)(p[b + 3] + 0.5f); fStages[f] = sl == 0 ? 1 : (sl == 1 ? 2 : 4);
            fMidi[f] = p[b + 7] > 0.5f && p[kMidiOn] > 0.5f;
        }

        for (uint32_t i = 0; i < n; ++i) {
            const float xl = inL[i], xr = inR[i];

            // smoothed controls
            for (int f = 0; f < 2; ++f) {
                const uint32_t b = f == 0 ? kF1On : kF2On;
                float tgtOct = std::log2(clampf(p[b + 4], 10.f, 22000.f));
                if (fMidi[f] && lastNote >= 0) tgtOct += (lastNote - 60) / 12.f;
                sOct[f]  += (tgtOct - sOct[f]) * smooth;
                sPeak[f] += (p[b + 5] - sPeak[f]) * smooth;
                sPan[f]  += (p[b + 6] - sPan[f]) * smooth;
            }
            sIn  += (dbToGain(p[kInGain]) - sIn) * smooth;
            sOut += (dbToGain(p[kOutGain]) - sOut) * smooth;
            sMix += (p[kMix] - sMix) * smooth;

            // input level follower (pre-gain) drives the EG trigger and the auto-mute
            const float a = std::max(std::fabs(xl), std::fabs(xr));
            level += (a - level) * (a > level ? followAtk : followRel);
            if (!egMidi) {
                if (!eg.gated() && level > thr) eg.gate(true);
                else if (eg.gated() && level < thr * 0.7f) eg.gate(false);
            }

            // modulation sources
            float src[kNumSources];
            for (int l = 0; l < 2; ++l) {
                const uint32_t b = l == 0 ? kL1Wave : kL2Wave;
                lfo[l].phase += lfoInc[l]; if (lfo[l].phase >= 1.0) lfo[l].phase -= std::floor(lfo[l].phase);
                const int wave = (int)(p[b] + 0.5f);
                const float raw = LFO::shape((float)lfo[l].phase, wave, p[b + 1]);
                lfo[l].out += (raw - lfo[l].out) * (wave == 1 ? sqSmooth : 1.f);
                src[l] = lfo[l].out;
            }
            src[kSrcEg] = eg.process();

            float dst[kNumDests] = {};
            for (int s = 0; s < kNumSources; ++s)
                for (int d = 0; d < kNumDests; ++d) {
                    const float amt = p[modParam(s, d)];
                    if (amt != 0.f) dst[d] += amt * src[s];
                }

            // filters
            const float gin = sIn;
            float wl = xl * gin, wr = xr * gin;
            float yl[2], yr[2];
            for (int f = 0; f < 2; ++f) {
                const float oct = sOct[f] + 4.f * dst[f * 3 + 0];
                const float peak = clampf(sPeak[f] + dst[f * 3 + 1], 0.f, 1.f);
                const float pan = clampf(sPan[f] + dst[f * 3 + 2], -1.5f, 1.5f);
                const float fcL = clampf(std::pow(2.f, oct - 2.f * pan), 10.f, 0.45f * fs);
                const float fcR = clampf(std::pow(2.f, oct + 2.f * pan), 10.f, 0.45f * fs);
                liveHz[f] = std::pow(2.f, oct);
                if (!fOn[f]) { yl[f] = yr[f] = 0; continue; }
                const bool fromF1 = !parallel && f == 1 && fOn[0];     // serial: filter 2 reads filter 1
                const float inLf = fromF1 ? yl[0] : wl;
                const float inRf = fromF1 ? yr[0] : wr;
                yl[f] = flt[f][0].process(inLf, std::tan(kPi * fcL / fs), peak, fType[f], fStages[f], fStyle[f]);
                yr[f] = flt[f][1].process(inRf, std::tan(kPi * fcR / fs), peak, fType[f], fStages[f], fStyle[f]);
            }
            float ol, orr;
            if (parallel) {
                if (fOn[0] && fOn[1]) { ol = 0.7071f * (yl[0] + yl[1]); orr = 0.7071f * (yr[0] + yr[1]); }
                else if (fOn[0])      { ol = yl[0]; orr = yr[0]; }
                else if (fOn[1])      { ol = yl[1]; orr = yr[1]; }
                else                  { ol = wl; orr = wr; }
            } else {
                // serial: a bypassed filter passes its input through
                if (fOn[1])      { ol = yl[1]; orr = yr[1]; }
                else if (fOn[0]) { ol = yl[0]; orr = yr[0]; }
                else             { ol = wl; orr = wr; }
            }

            // auto-mute: fade the wet signal out when the input has been silent (stops self-oscillation)
            if (autoMute) {
                if (level < 1e-4f) silentFor++; else silentFor = 0;
                const float tgt = silentFor > (uint32_t)(0.05f * fs) ? 0.f : 1.f;
                muteGain += (tgt - muteGain) * (tgt > muteGain ? gateAtk : gateRel);
            } else muteGain = 1.f;

            const float wet = muteGain;
            float outl = (sMix * ol * wet + (1.f - sMix) * xl) * sOut;
            float outr = (sMix * orr * wet + (1.f - sMix) * xr) * sOut;
            outL[i] = softCeil(outl);
            outR[i] = softCeil(outr);
        }
    }

private:
    static float softCeil(float x) {        // transparent below -1 dBFS, soft ceiling above
        const float a = std::fabs(x);
        if (!(a < 1e6f)) return 0.f;        // NaN/Inf guard
        return a > 0.89f ? std::copysign(0.89f + 0.11f * std::tanh((a - 0.89f) / 0.11f), x) : x;
    }
    void snapSmoothing() {
        for (int f = 0; f < 2; ++f) {
            const uint32_t b = f == 0 ? kF1On : kF2On;
            sOct[f] = std::log2(clampf(p[b + 4], 10.f, 22000.f)); sPeak[f] = p[b + 5]; sPan[f] = p[b + 6];
        }
        sIn = dbToGain(p[kInGain]); sOut = dbToGain(p[kOutGain]); sMix = p[kMix];
    }

    FilterChan flt[2][2];
    LFO lfo[2];
    ADSR eg;
    float fs = 48000.f, smooth = 0.01f, sqSmooth = 0.1f, followAtk = 0.1f, followRel = 0.01f, gateAtk = 0.01f, gateRel = 0.001f;
    float sOct[2] = {10, 10}, sPeak[2] = {0, 0}, sPan[2] = {0, 0}, sIn = 1, sOut = 1, sMix = 1;
    float level = 0, muteGain = 1, liveHz[2] = {1000, 200};
    uint32_t silentFor = 0;
    int stack[16] = {}; int held = 0; int lastNote = -1;
    bool tValid = false, tPlaying = false;
    double tBarPos = 0, tBpm = 120, tBeatsPerBar = 4;
};

} // namespace caldera
