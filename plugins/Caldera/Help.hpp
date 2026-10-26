// Caldera — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "ParamDefs.hpp"

namespace caldera {

static const char* const kManualURL = "https://github.com/natehouk/caldera/blob/main/docs/MANUAL.md";

inline const char* styleTip(int s) {
    switch (s) {
    case 0: return "Clean: linear, high-Q, never self-oscillates. The only style with no drive or saturation at all.";
    case 1: return "Smooth: gentle and broad, with a soft, wide resonance.";
    case 2: return "Warm: a saturated input into a linear filter. Rounded, never harsh.";
    case 3: return "Monolith: the Monolith synth's filter. Soft-limited, and it self-oscillates at high Peak.";
    case 4: return "Hot: saturating integrators. A thick sound with a screaming peak.";
    case 5: return "Acid: resonance thins out the bass. Squelchy and rubbery.";
    case 6: return "Raw: hard-clipped feedback. Buzzy, aggressive self-oscillation.";
    case 7: return "Tube: an asymmetric, valve-like input stage. Warm overdrive with mostly even harmonics and a soft, singing peak.";
    case 8: return "Gentle: clean, polite and broad. General purpose; never self-oscillates.";
    case 9: return "Metal: a hard-clipped input and hard-clipped feedback. Rough, sharp and distorted.";
    case 10: return "Hollow: the band right at the cutoff is carved out as the peak rises, leaving a hollow, nasal tone; deep, low self-oscillation.";
    default: return "";
    }
}
inline const char* shapeTip() {
    return "LP keeps the lows, BP a band around the cutoff, HP the highs. BELL boosts or cuts around the cutoff; LO SH and HI SH boost or cut everything "
           "below or above it (GAIN sets how much). NOTCH removes a narrow band. AP (all pass) changes only the phase: on its own it sounds the same, "
           "mixed with the dry signal or another filter it makes phasing. The shapes follow the style, so a Hot bell distorts.";
}

inline const char* filterFieldTip(int k) {
    switch (k) {
    case kFOn: return "Switches this filter on or off. Off, the signal passes it untouched (in series) or it drops out of the mix (in parallel).";
    case kFStyle: return "The filter's character, from Clean (linear) to Metal (hard-clipped). Click to choose.";
    case kFType: return shapeTip();
    case kFSlope: return "How steep the filter is: 6, 12, 24 or 48 dB per octave. For bell and shelf shapes, steeper means a narrower band. Band pass and notch have no 6 dB/oct; they use 12.";
    case kFFreq: return "The cutoff (or centre) frequency, 20 Hz to 20 kHz. With KEY on, this is the cutoff at middle C. You can also drag the filter's numbered handle in the display.";
    case kFPeak: return "Resonance: emphasises the frequencies around the cutoff; most styles self-oscillate near the top. For bell, shelf, notch and all pass it narrows the band instead.";
    case kFPan: return "Stereo frequency spread: lowers the left channel's cutoff and raises the right's (or the reverse), up to 2 octaves each way. Stereo mode only.";
    case kFMidi: return "Key tracking: the cutoff follows the MIDI notes you play. FREQ then sets the cutoff at middle C.";
    case kFDrive: return "Drives the filter harder, up to 24 dB, without changing its level much: from clean to gritty. Clean and Smooth stay clean.";
    case kFEqGain: return "For bell and shelf shapes: how much to boost or cut, up to 24 dB. The other shapes ignore it.";
    case kFDelay: return "Delays this filter's output by up to 50 ms. With parallel routing and the MIX control it makes comb filtering, Haas widening, chorus and flanging, especially when modulated.";
    case kFLevel: return "This filter's output level, up to 24 dB either way. Most useful with parallel routing.";
    case kFOutPan: return "Pans this filter's output left or right. Stereo mode only.";
    default: return "";
    }
}

inline const char* paramTip(uint32_t p) {
    for (int f = 0; f < kNumFilters; ++f) for (int k = 0; k < kFAllFields; ++k) if (fpar(f, k) == p) return filterFieldTip(k);
    if (isModParam(p)) {
        const int d = (int)((p - kModFirst) % kNumClassicDests) % 3;
        if (d == 0) return "Classic modulation: how far this source moves the filter's cutoff, up to 4 octaves either way. Turn left for the inverted signal.";
        if (d == 1) return "Classic modulation: how far this source moves the filter's peak. Turn left for the inverted signal.";
        return "Classic modulation: how far this source moves the filter's stereo spread. Turn left for the inverted signal.";
    }
    switch (p) {
    case kRouting: return "How the filters connect. SERIAL: 1 > 2 > 3 > 4. PARALLEL: every filter gets the input and their outputs are mixed. "
                          "SERIAL PAIRS: (1 > 2) and (3 > 4) side by side. PARALLEL PAIRS: (1 + 2) into (3 + 4). Filters that are off are skipped.";
    case kChanMode: return "STEREO: every filter processes both channels. L/R: filters 1 and 2 process the left channel, 3 and 4 the right. "
                           "M/S: filters 1 and 2 process the mid (centre), 3 and 4 the side (stereo difference). Pan and output pan are off in L/R and M/S.";
    case kHq: return "4x oversampling for the filters and their saturation: less aliasing with high drive and resonance, at about three times the CPU and 48 samples of latency.";
    case kInGain: return "Input level, up to 24 dB either way. Driving the input harder pushes the saturating styles into distortion.";
    case kInPan: return "Input balance: turns the left or right input down before the filters.";
    case kOutGain: return "Output level, up to 24 dB either way.";
    case kMix: return "Dry/wet balance. 100% is only the filtered signal.";
    case kAutoMute: return "Silences self-oscillation 50 ms after the input goes quiet, so a ringing filter doesn't whistle on through the gaps.";
    case kL1Wave: case kL2Wave: return "Classic LFO shape: TRI (triangle) or SQR (square). BALANCE skews it.";
    case kL1Balance: case kL2Balance: return "For the triangle, skews the shape from a falling ramp through symmetric to a rising ramp. For the square, sets the duty cycle.";
    case kL1Sync: case kL2Sync: return "FREE runs at RATE in Hz. STR, DOT and TRIP lock the LFO to the host tempo and song position.";
    case kL1Rate: case kL2Rate: return "Classic LFO speed, 0.01 to 100 Hz.";
    case kL1Div: case kL2Div: return "The classic LFO's cycle length when synced, from 16 bars to a 64th note.";
    case kL1Midi: case kL2Midi: return "Restarts the classic LFO on every MIDI note.";
    case kEgAttack: return "Classic envelope attack.";
    case kEgDecay: return "Classic envelope decay.";
    case kEgSustain: return "Classic envelope sustain level.";
    case kEgRelease: return "Classic envelope release.";
    case kEgThreshold: return "The input level that triggers the classic envelope.";
    case kEgMidi: return "Triggers the classic envelope from MIDI notes instead of the input level.";
    case kMidiOn: return "Turns all MIDI handling on or off: key tracking, MIDI triggers and sources, LFO retrigger and MIDI Learn.";
    case kPiano: return "Shows a piano keyboard under the display, so you can line filters up with notes.";
    default: break;
    }
    if (p >= kXlfoFirst && p < kEgFirst) {
        switch ((p - kXlfoFirst) % kXlfoFields) {
        case kLMode: return "WAVE: a classic LFO shape. STEPS: a 16-step sequencer you draw in the step display.";
        case kLWave: return "The LFO shape: sine, triangle, saw up, saw down, square, smooth random, sample & hold.";
        case kLRate: return "LFO speed in Hz when not synced.";
        case kLSync: return "FREE runs at the Rate. SYNC, DOTTED and TRIPLET lock to the host tempo and follow the song position.";
        case kLDiv: return "Length of one cycle when synced, from 16 bars down to a 1/64 note.";
        case kLBalance: return "Skews the shape: for triangle and sine from falling to rising ramp; for square it sets the duty cycle.";
        case kLGlide: return "In step mode, how smoothly the value slides from one step to the next.";
        case kLSteps: return "How many steps the sequence uses before it repeats.";
        default: return "One step of the sequence. Drag up and down in the step display.";
        }
    }
    if (p >= kEgFirst && p < kEfFirst) {
        switch ((p - kEgFirst) % kEgFields) {
        case kEAttack: return "Envelope attack time."; case kEDecay: return "Envelope decay time."; case kESustain: return "Level the envelope holds while triggered.";
        case kERelease: return "Envelope release time.";
        case kETrigger: return "MIDI: the envelope starts with each note. AUDIO: it starts when the input rises above the threshold.";
        default: return "Input level that triggers the envelope in AUDIO mode.";
        }
    }
    if (p >= kEfFirst && p < kXyFirst) {
        switch ((p - kEfFirst) % kEfFields) {
        case kFAttack: return "How fast the follower rises with the level."; case kFRelease: return "How fast it falls back.";
        case kFGain: return "Sensitivity: raises or lowers the level the follower sees.";
        default: return "INPUT follows the plugin's input. SIDECHAIN follows the sidechain input (route another track to it in your DAW).";
        }
    }
    if (p >= kXyFirst && p < kSlotFirst) return "XY controller position. Drag the dot in the pad; X and Y are two separate sources.";
    if (p >= kSlotFirst && p < kInPan) {
        switch ((p - kSlotFirst) % kSlotFields) {
        case kSlotSrc: return "The modulation source. Click to change.";
        case kSlotDst: return "What this slot modulates. Click to change.";
        case kSlotAmt: return "How much, and in which direction. Turn left to invert.";
        default: return "Switches this slot off without deleting it.";
        }
    }
    return "";
}

enum TipTarget { kTipUndo, kTipRedo, kTipLearn, kTipMidi, kTipHelp, kTipA, kTipB, kTipCopy, kTipPrev, kTipPresets, kTipNext,
                 kTipDisplay, kTipHandle, kTipChip, kTipStepEdit, kTipXyPad, kTipSourceTab, kTipCompPresets, kTipModRing, kTipFilterTab,
                 kTipClassicChip, kTipClassicRow, kTipStyle, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
        "Previous preset", "Presets", "Next preset", "Filter display", "Filter handle", "Modulation source", "Steps", "XY pad", "Sources",
        "Source presets", "Modulation", "Filter", "Classic source", "Classic modulation", "Style" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including preset loads and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click a control to unassign.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: filters, routing, modulation, MIDI and shortcuts.";
    case kTipA: case kTipB: return "Caldera keeps two complete settings, A and B. Click the other letter to switch and compare.";
    case kTipCopy: return "Copies the current setting to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser. A * after the name means you have changed it. In the browser, type to search names and categories, and click a star to keep a preset in Favourites.";
    case kTipNext: return "Load the next preset.";
    case kTipDisplay: return "The filters' responses and their combined curve. Double-click an empty spot to switch on the next free filter there (up to four). Drag a numbered handle: left/right is frequency, up/down is peak (or gain for bell and shelf). Right-click a handle to switch that filter off. Alt-drag a handle to move all filters together.";
    case kTipHandle: return "Drag left/right for frequency, up/down for peak (gain for bell and shelf shapes). Click to select the filter; right-click to switch it off. Shift-drag for fine adjustment.";
    case kTipChip: return "Drag this onto any knob to modulate it. A coloured ring then shows the knob is modulated, and the slot appears in the list on the right.";
    case kTipStepEdit: return "Draw the sequence: click or drag up and down across the steps. Cmd/Ctrl-click resets a step to zero.";
    case kTipXyPad: return "Drag the dot. X and Y are separate modulation sources; drag their chips onto knobs.";
    case kTipSourceTab: return "Choose a modulation source to edit. Each source has a chip you drag onto the knobs it should move. CLASSIC holds the two LFOs and the envelope of Caldera 2.";
    case kTipCompPresets: return "Presets for just this source: load a ready-made LFO shape, sequence, envelope or follower setting without touching the rest of the patch, or save your own. Right-click a saved one to delete it.";
    case kTipModRing: return "This knob is modulated. Click the ring (or right-click the knob) to list every slot that moves it, with its amount, bypass and delete.";
    case kTipFilterTab: return "Click to edit this filter. The dot switches it on or off.";
    case kTipClassicChip: return "A classic source from Caldera 2. Drag it onto the FREQ, PEAK or PAN of filter 1 or 2; it appears in the slot list as a classic row. For everything else, use the XLFOs, envelopes and followers.";
    case kTipClassicRow: return "A classic modulation (Caldera 2's grid). Drag the amount; the cross sets it to zero.";
    case kTipStyle: return "Click to choose the filter's style.";
    default: return "";
    }
}

struct HelpPage { const char* title; const char* body; };
static const HelpPage kHelpPages[] = {
    { "Overview",
      "Caldera is a filter effect with up to four filters. Each has its own style (from clean to screaming), shape (pass, bell, shelf, notch, all pass), "
      "slope, drive, delay, level and pan. The filters connect in series, in parallel or in pairs, in stereo, left/right or mid/side.\n\n"
      "A modulation system with XLFOs (wave or 16-step), envelopes, envelope followers (input or sidechain), XY pads and MIDI can move almost every control. "
      "Caldera 2's two LFOs and envelope are still here, under CLASSIC.\n\n"
      "Hover over anything for a moment to see what it does. Turn the tips off at the bottom of this window." },
    { "Filters",
      "Select a filter with the 1 to 4 buttons in the filter panel; the dot switches it on or off. In the display, double-click to switch on the next free filter "
      "at that frequency and drag the numbered handles (right-click a handle to switch the filter off).\n\n"
      "STYLE sets the character, SHAPE the response and SLOPE the steepness (6 to 48 dB/oct; for bell and shelf, a narrower band). "
      "DRIVE pushes the filter harder; GAIN sets the boost or cut of bell and shelf; DELAY delays the filter's output up to 50 ms; LEVEL and OUT PAN place it in the mix. "
      "PAN spreads the left and right cutoffs apart. KEY makes the cutoff follow MIDI notes." },
    { "Routing & global",
      "ROUTING: SERIAL (1 > 2 > 3 > 4), PARALLEL (all four side by side), SERIAL PAIRS ((1 > 2) beside (3 > 4)) and PARALLEL PAIRS ((1 + 2) into (3 + 4)).\n\n"
      "CHANNELS: STEREO, L/R (filters 1-2 on the left, 3-4 on the right) or M/S (1-2 on the mid, 3-4 on the side).\n\n"
      "HQ runs the filters at 4x oversampling (48 samples of latency). AUTO MUTE stops self-oscillation once the input goes quiet. "
      "INPUT, IN PAN, OUTPUT and MIX are at the bottom of the global panel." },
    { "Modulation",
      "Pick a source in the tabs at the bottom left: four XLFOs (wave or 16-step), two envelope generators (MIDI or audio), two envelope followers "
      "(input or sidechain), two XY pads, the MIDI sources, and CLASSIC.\n\n"
      "Drag the source's chip onto any knob. That creates a modulation slot (up to 50) in the list at the bottom right, where you can change its amount, "
      "source and destination, bypass it or delete it. A slot's amount can itself be modulated. Click a modulated knob's ring, or right-click it, to see every slot that moves it.\n\n"
      "LFO, EG and EF PRESETS load or save a setting for one source only." },
    { "Classic sources",
      "CLASSIC holds Caldera 2's two LFOs and its envelope, unchanged, so older projects sound and work exactly as before. "
      "Their chips reach the FREQ, PEAK and PAN of filters 1 and 2 (Caldera 2's grid); those connections show in the slot list as classic rows, "
      "and they add to any slots on the same knob." },
    { "A/B, undo & MIDI",
      "A/B holds two complete settings; COPY copies the current one to the other slot. Every edit is an undo step (Cmd/Ctrl-Z, Cmd/Ctrl-Shift-Z).\n\n"
      "MIDI LEARN: click LEARN, click a control, move a controller. MIDI notes drive KEY tracking, the Velocity and Key sources and can trigger envelopes.\n\n"
      "Double-click a knob to type a value; Cmd/Ctrl-click resets it. Shift-drag is fine adjustment. Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace caldera
