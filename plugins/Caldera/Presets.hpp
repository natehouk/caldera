// Factory presets (each lists only what it changes from the engine defaults). License: MIT.
#pragma once
#include "CalderaEngine.hpp"
#include <algorithm>

namespace caldera {

struct PV { uint32_t id; float v; };
struct PresetDef { const char* name; const char* category; const PV* pv; };
struct Preset { const char* name; const char* category; float v[kParamCount]; };

static const char* const kCategories[] = { "Sweeps", "Rhythmic", "Resonant", "Stereo", "Character", "Utility" };
static constexpr int kNumCategories = 6;

#define M(s, d) modParam(kSrc##s, kDst##d)
#define END { kParamCount, 0 }
namespace pv {
static const PV init[] = { END };

// ---------------------------------------------------------------- Sweeps
static const PV slowSweep[]  = { {kF1Freq,600},{kF1Peak,.45f},{M(Lfo1,F1Freq),.55f},{kL1Rate,.08f}, END };
static const PV barSweep[]   = { {kF1Freq,700},{kF1Peak,.5f},{kL1Sync,1},{kL1Div,3},{M(Lfo1,F1Freq),.6f}, END };
static const PV riser[]      = { {kF1Type,2},{kF1Slope,2},{kF1Freq,120},{kF1Peak,.35f},{kL1Sync,1},{kL1Div,1},{kL1Balance,.98f},{M(Lfo1,F1Freq),.7f}, END };
static const PV faller[]     = { {kF1Freq,1500},{kF1Peak,.4f},{kL1Sync,1},{kL1Div,1},{kL1Balance,.02f},{M(Lfo1,F1Freq),.6f}, END };
static const PV bpSweep[]    = { {kF1Type,1},{kF1Slope,1},{kF1Freq,900},{kF1Peak,.6f},{kL1Rate,.25f},{M(Lfo1,F1Freq),.6f}, {kOutGain,10}, END };
static const PV dualSweep[]  = { {kF2On,1},{kRouting,1},{kF2Type,1},{kF2Style,0},{kF1Freq,500},{kF2Freq,3000},{kF1Peak,.5f},{kF2Peak,.5f},
    {kL1Rate,.15f},{M(Lfo1,F1Freq),.5f},{M(Lfo1,F2Freq),-.5f}, END };
static const PV notchSweep[] = { {kF2On,1},{kRouting,1},{kF1Type,0},{kF2Type,2},{kF1Slope,2},{kF2Slope,2},{kF1Peak,0},{kF2Peak,0},{kF1Freq,700},{kF2Freq,1000},
    {kL1Rate,.2f},{M(Lfo1,F1Freq),.4f},{M(Lfo1,F2Freq),.4f}, END };

// ---------------------------------------------------------------- Rhythmic
static const PV gate8[]      = { {kF1Freq,250},{kF1Peak,.2f},{kL2Sync,1},{kL2Div,7},{kL2Balance,.35f},{M(Lfo2,F1Freq),.8f}, END };
static const PV gate16[]     = { {kF1Freq,300},{kF1Peak,.3f},{kL2Sync,1},{kL2Div,8},{kL2Balance,.5f},{M(Lfo2,F1Freq),.7f}, END };
static const PV wobble[]     = { {kF1Style,5},{kF1Freq,350},{kF1Peak,.65f},{kL1Sync,1},{kL1Div,7},{M(Lfo1,F1Freq),.55f},{kInGain,3}, END };
static const PV tripletWob[] = { {kF1Style,3},{kF1Freq,400},{kF1Peak,.6f},{kL1Sync,3},{kL1Div,6},{M(Lfo1,F1Freq),.5f}, END };
static const PV dottedPump[] = { {kF1Type,2},{kF1Freq,80},{kF1Peak,.2f},{kL1Sync,2},{kL1Div,7},{kL1Balance,.9f},{M(Lfo1,F1Freq),.55f}, END };
static const PV trance[]     = { {kF1Freq,500},{kF1Peak,.4f},{kL2Sync,1},{kL2Div,8},{kL2Balance,.25f},{M(Lfo2,F1Freq),.6f},{kL1Sync,1},{kL1Div,2},{M(Lfo1,F1Freq),.3f}, END };
static const PV envFollow[]  = { {kF1Freq,250},{kF1Peak,.5f},{kEgThreshold,-28},{kEgAttack,.005f},{kEgDecay,.25f},{kEgSustain,.3f},{kEgRelease,.2f},{M(Eg,F1Freq),.6f}, END };

// ---------------------------------------------------------------- Resonant
static const PV acidSquelch[]= { {kF1Style,5},{kF1Freq,450},{kF1Peak,.85f},{kInGain,6},{kEgThreshold,-30},{kEgAttack,0},{kEgDecay,.18f},{kEgSustain,0},{M(Eg,F1Freq),.55f}, END };
static const PV scream[]     = { {kF1Style,4},{kF1Freq,1200},{kF1Peak,.9f},{kInGain,9},{kOutGain,-6}, END };
static const PV whistle[]    = { {kF1Style,3},{kF1Type,1},{kF1Freq,2000},{kF1Peak,.97f},{kAutoMute,1},{kMix,.5f}, END };
static const PV keyedRing[]  = { {kF1Style,3},{kF1Midi,1},{kF1Freq,261.6f},{kF1Peak,1},{kMix,.6f}, END };
static const PV rawBuzz[]    = { {kF1Style,6},{kF1Freq,700},{kF1Peak,.8f},{kInGain,6},{kOutGain,-6}, END };
static const PV peakLfo[]    = { {kF1Style,3},{kF1Freq,900},{kF1Peak,.6f},{kL1Rate,.5f},{M(Lfo1,F1Peak),.4f}, END };
static const PV formant[]    = { {kF2On,1},{kRouting,1},{kF1Type,1},{kF2Type,1},{kF1Freq,700},{kF2Freq,1200},{kF1Peak,.8f},{kF2Peak,.8f},
    {kL1Rate,.4f},{M(Lfo1,F1Freq),.15f},{M(Lfo1,F2Freq),.25f}, {kOutGain,10}, END };

// ---------------------------------------------------------------- Stereo
static const PV wide[]       = { {kF1Freq,2500},{kF1Peak,.3f},{kF1Pan,.4f}, END };
static const PV panSweep[]   = { {kF1Freq,1200},{kF1Peak,.45f},{kL1Rate,.3f},{M(Lfo1,F1Pan),.8f}, END };
static const PV stereoChop[] = { {kF1Freq,800},{kF1Peak,.3f},{kL2Sync,1},{kL2Div,7},{M(Lfo2,F1Pan),1}, END };
static const PV spinner[]    = { {kF1Type,1},{kF1Freq,1000},{kF1Peak,.55f},{kL1Rate,1.5f},{kL2Rate,.11f},{M(Lfo1,F1Pan),.6f},{M(Lfo2,F1Freq),.4f}, {kOutGain,10}, END };
static const PV mirrored[]   = { {kF2On,1},{kRouting,1},{kF1Pan,.7f},{kF2Pan,-.7f},{kF2Type,0},{kF1Freq,900},{kF2Freq,900},{kF1Peak,.5f},{kF2Peak,.5f}, END };
static const PV widthEnv[]   = { {kF1Freq,1500},{kF1Peak,.3f},{kEgThreshold,-24},{kEgDecay,.5f},{kEgSustain,0},{M(Eg,F1Pan),.9f}, END };

// ---------------------------------------------------------------- Character
static const PV warmTape[]   = { {kF1Style,2},{kF1Freq,9000},{kF1Peak,.1f},{kF1Slope,0},{kF2On,1},{kF2Freq,35},{kInGain,6},{kOutGain,-5}, END };
static const PV telephone[]  = { {kF1Type,1},{kF1Slope,1},{kF1Freq,1400},{kF1Peak,.3f},{kF1Style,2},{kInGain,6},{kOutGain,8}, END };
static const PV radio[]      = { {kF1Type,1},{kF1Freq,1800},{kF1Peak,.45f},{kF1Style,6},{kInGain,4},{kOutGain,12},{kL1Rate,.07f},{M(Lfo1,F1Freq),.08f}, END };
static const PV muffled[]    = { {kF1Style,1},{kF1Freq,450},{kF1Peak,.15f},{kF1Slope,2}, END };
static const PV crunchHp[]   = { {kF1Style,6},{kF1Type,2},{kF1Freq,300},{kF1Peak,.4f},{kInGain,8},{kOutGain,3}, END };
static const PV underwater[] = { {kF1Style,1},{kF1Freq,350},{kF1Peak,.4f},{kL1Rate,.6f},{M(Lfo1,F1Freq),.12f},{kF1Pan,.15f}, END };
static const PV lofiDrive[]  = { {kF1Style,4},{kF1Freq,5000},{kF1Peak,.25f},{kF2On,1},{kF2Freq,150},{kF2Slope,1},{kInGain,10},{kOutGain,-4}, END };

// ---------------------------------------------------------------- Utility
static const PV lowCut[]     = { {kF1Style,0},{kF1Type,2},{kF1Slope,1},{kF1Freq,80},{kF1Peak,0}, END };
static const PV highCut[]    = { {kF1Style,0},{kF1Type,0},{kF1Slope,1},{kF1Freq,12000},{kF1Peak,0}, END };
static const PV bandLimit[]  = { {kF2On,1},{kF1Style,0},{kF2Style,0},{kF1Type,2},{kF2Type,0},{kF1Freq,100},{kF2Freq,10000},{kF1Peak,0},{kF2Peak,0}, END };
static const PV djFilter[]   = { {kF1Style,1},{kF1Slope,1},{kF1Freq,20000},{kF1Peak,.35f}, END };
static const PV djHp[]       = { {kF1Style,1},{kF1Type,2},{kF1Slope,1},{kF1Freq,20},{kF1Peak,.35f}, END };
static const PV keyFollow[]  = { {kF1Style,0},{kF1Midi,1},{kF1Freq,523.3f},{kF1Peak,.3f}, END };
} // namespace pv
#undef END
#undef M

static const PresetDef kPresetDefs[] = {
    {"Init", "Init", pv::init},
    {"Slow Sweep", "Sweeps", pv::slowSweep}, {"Bar Sweep", "Sweeps", pv::barSweep}, {"Riser (4 bars)", "Sweeps", pv::riser},
    {"Faller (4 bars)", "Sweeps", pv::faller}, {"Band Sweep", "Sweeps", pv::bpSweep}, {"Dual Sweep", "Sweeps", pv::dualSweep},
    {"Notch Sweep", "Sweeps", pv::notchSweep},
    {"Gate 1/8", "Rhythmic", pv::gate8}, {"Gate 1/16", "Rhythmic", pv::gate16}, {"Wobble 1/8", "Rhythmic", pv::wobble},
    {"Triplet Wobble", "Rhythmic", pv::tripletWob}, {"Dotted Pump", "Rhythmic", pv::dottedPump}, {"Trance Gate", "Rhythmic", pv::trance},
    {"Envelope Follow", "Rhythmic", pv::envFollow},
    {"Acid Squelch", "Resonant", pv::acidSquelch}, {"Scream", "Resonant", pv::scream}, {"Whistler", "Resonant", pv::whistle},
    {"Keyed Ring", "Resonant", pv::keyedRing}, {"Raw Buzz", "Resonant", pv::rawBuzz}, {"Peak Wander", "Resonant", pv::peakLfo},
    {"Formant", "Resonant", pv::formant},
    {"Wide", "Stereo", pv::wide}, {"Pan Sweep", "Stereo", pv::panSweep}, {"Stereo Chop", "Stereo", pv::stereoChop},
    {"Spinner", "Stereo", pv::spinner}, {"Mirrored", "Stereo", pv::mirrored}, {"Width Envelope", "Stereo", pv::widthEnv},
    {"Warm Tape", "Character", pv::warmTape}, {"Telephone", "Character", pv::telephone}, {"Old Radio", "Character", pv::radio},
    {"Muffled", "Character", pv::muffled}, {"Crunch HP", "Character", pv::crunchHp}, {"Underwater", "Character", pv::underwater},
    {"Lo-Fi Drive", "Character", pv::lofiDrive},
    {"Low Cut 80 Hz", "Utility", pv::lowCut}, {"High Cut 12 kHz", "Utility", pv::highCut}, {"Band Limit", "Utility", pv::bandLimit},
    {"DJ Low Pass", "Utility", pv::djFilter}, {"DJ High Pass", "Utility", pv::djHp}, {"Key Follow", "Utility", pv::keyFollow},
};
static constexpr uint32_t kNumPresets = sizeof(kPresetDefs) / sizeof(kPresetDefs[0]);

inline const Preset* presets() {
    static Preset table[kNumPresets];
    static bool done = false;
    if (done) return table;
    done = true;
    Engine e;
    for (uint32_t i = 0; i < kNumPresets; ++i) {
        table[i].name = kPresetDefs[i].name; table[i].category = kPresetDefs[i].category;
        std::copy(e.p, e.p + kParamCount, table[i].v);
        for (const PV* x = kPresetDefs[i].pv; x->id != kParamCount; ++x) table[i].v[x->id] = x->v;
    }
    return table;
}

} // namespace caldera
