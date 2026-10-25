// Caldera — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "ParamDefs.hpp"

namespace caldera {

static const char* const kManualURL = "https://github.com/natehouk/caldera/blob/main/docs/MANUAL.md";

inline const char* paramTip(uint32_t p) {
    if (isModParam(p)) {
        const int d = (int)((p - kModFirst) % kNumDests) % 3;
        if (d == 0) return "How far this source moves the filter's cutoff, up to 4 octaves either way. Turn left for the inverted signal.";
        if (d == 1) return "How far this source moves the filter's peak (resonance). Turn left for the inverted signal.";
        return "How far this source moves the filter's stereo pan: the left and right cutoffs spread apart and swing. Turn left for the inverted signal.";
    }
    switch (p) {
    case kF1On: case kF2On: return "Switches this filter on or off. Off, the signal passes it untouched (in serial) or it drops out of the mix (in parallel).";
    case kF1Style: case kF2Style: return "The filter's character: CLEAN (linear, never self-oscillates), SMOOTH (gentle, broad), WARM (saturated input), MONOLITH (the Monolith synth's filter), HOT (screaming peak), ACID (resonance thins the bass), RAW (buzzy, hard-clipped feedback). Click the arrows or scroll.";
    case kF1Type: case kF2Type: return "LP keeps the lows, BP keeps a band around the cutoff, HP keeps the highs.";
    case kF1Slope: case kF2Slope: return "How steeply the filter cuts: 12, 24 or 48 dB per octave.";
    case kF1Freq: case kF2Freq: return "The cutoff frequency, 20 Hz to 20 kHz. With KEY on, this is the cutoff at middle C. You can also drag this filter's numbered handle in the display.";
    case kF1Peak: case kF2Peak: return "Resonance: emphasises the frequencies around the cutoff. In the self-oscillating styles the filter starts to sing near the top.";
    case kF1Pan: case kF2Pan: return "Offsets the cutoff between the left and right channels, up to 2 octaves either way, for stereo movement without phase problems.";
    case kF1Midi: case kF2Midi: return "Key tracking: the cutoff follows the MIDI notes you play. FREQ then sets the cutoff at middle C.";
    case kRouting: return "SERIAL runs the signal through filter 1, then filter 2. PARALLEL feeds both filters the input and mixes their outputs.";
    case kInGain: return "Input level, up to 24 dB either way. Driving the input harder pushes the saturating styles (WARM, HOT, ACID, RAW, MONOLITH) into distortion.";
    case kOutGain: return "Output level, up to 24 dB either way.";
    case kMix: return "Dry/wet balance. 100% is only the filtered signal.";
    case kAutoMute: return "Silences self-oscillation 50 ms after the input goes quiet, so a ringing filter doesn't whistle on through the gaps.";
    case kL1Wave: case kL2Wave: return "The LFO's shape: TRI (triangle) or SQR (square). BALANCE skews it.";
    case kL1Balance: case kL2Balance: return "For the triangle, skews the shape from a falling ramp through symmetric (50%) to a rising ramp. For the square, sets the duty cycle.";
    case kL1Sync: case kL2Sync: return "FREE runs at RATE in Hz. STR, DOT and TRIP lock the LFO to the host tempo and song position with straight, dotted or triplet note lengths; RATE then picks the length.";
    case kL1Rate: case kL2Rate: return "LFO speed, 0.01 to 100 Hz. When synced, this knob picks a note length (16 bars to 1/64) instead.";
    case kL1Div: case kL2Div: return "The LFO cycle length when synced, from 16 bars to a 64th note.";
    case kL1Midi: case kL2Midi: return "Restarts the LFO at the start of its cycle on every MIDI note.";
    case kEgAttack: return "Envelope attack: how long the envelope takes to rise once triggered.";
    case kEgDecay: return "Envelope decay: how long it takes to fall to the sustain level.";
    case kEgSustain: return "Envelope sustain: the level it holds while the input stays above the threshold (or the MIDI note is held).";
    case kEgRelease: return "Envelope release: how long it takes to fall back to zero.";
    case kEgThreshold: return "The input level that triggers the envelope. Each time the input rises past it, the envelope starts; when it falls below, the envelope releases. Works like an envelope follower or a gate.";
    case kEgMidi: return "Triggers the envelope from MIDI notes instead of the input level.";
    case kMidiOn: return "Turns all MIDI handling on or off: key tracking, MIDI triggers, LFO retrigger and MIDI Learn. Switch it off in hosts that send MIDI to every effect.";
    default: return "";
    }
}

enum TipTarget { kTipUndo, kTipRedo, kTipLearn, kTipMidi, kTipHelp, kTipA, kTipB, kTipCopy, kTipPrev, kTipPresets, kTipNext,
                 kTipDisplay, kTipHandle1, kTipHandle2, kTipHandleLink, kTipLfo1, kTipLfo2, kTipEnv, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
                                                   "Previous preset", "Presets", "Next preset", "Filter display", "Filter 1 handle",
                                                   "Filter 2 handle", "Both filters", "LFO 1", "LFO 2", "Envelope" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including preset loads and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click a control to unassign.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: filters, routing, modulation, the display, MIDI and shortcuts.";
    case kTipA: case kTipB: return "Caldera keeps two complete settings, A and B. Click the other letter to switch and compare.";
    case kTipCopy: return "Copies the current setting to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser: factory presets in six categories, your own presets, and SAVE CURRENT AS. A * after the name means you have changed it.";
    case kTipNext: return "Load the next preset.";
    case kTipDisplay: return "Both filter curves (orange and teal), the combined response (white), and faint left/right curves when pan is used. The thin lines show where modulation is moving each cutoff right now.";
    case kTipHandle1: case kTipHandle2: return "Drag left/right for the cutoff and up/down for the peak. Cmd/Ctrl-drag moves the other filter in the opposite direction.";
    case kTipHandleLink: return "Drag to move both filters together, keeping their distance.";
    case kTipLfo1: case kTipLfo2: return "The LFO's shape. Its rate, or its note length when synced, is in the corner. Use the LFO1 / LFO2 amount knobs next to each filter's FREQ, PEAK and PAN to route it.";
    case kTipEnv: return "The envelope's shape. The light shows when it is active. Use the ENV amount knobs next to each filter's FREQ, PEAK and PAN to route it.";
    default: return "";
    }
}

struct HelpPage { const char* title; const char* body; };
static const HelpPage kHelpPages[] = {
    { "Overview",
      "Caldera is a stereo dual filter for sweeps, rhythmic movement, resonant tones and character.\n\n"
      "1. Switch on one or both filters and pick a STYLE, type (LP, BP, HP) and slope.\n"
      "2. Set FREQ and PEAK, or drag the numbered handles in the display.\n"
      "3. Bring it to life with the LFOs and the envelope: each has an amount knob next to every filter's FREQ, PEAK and PAN.\n\n"
      "Hover over anything for a moment to see what it does." },
    { "Filters",
      "Each filter has seven styles, from CLEAN (linear, never self-oscillates) through SMOOTH, WARM and MONOLITH to HOT, ACID and RAW, which saturate and self-oscillate.\n\n"
      "PAN offsets the cutoff between left and right. KEY makes the cutoff follow MIDI notes.\n\n"
      "SERIAL runs filter 1 into filter 2; PARALLEL mixes them. INPUT drives the saturating styles; AUTO MUTE stops self-oscillation from ringing on in silence." },
    { "Modulation",
      "Two LFOs (triangle or square, with BALANCE), free in Hz or synced to the host with straight, dotted or triplet lengths. RETRIG restarts an LFO on each MIDI note.\n\n"
      "The ENVELOPE starts when the input passes THRESH (or on MIDI notes with MIDI TRIG), so it can follow the dynamics of a drum loop.\n\n"
      "The small knobs next to each filter's FREQ, PEAK and PAN set how much LFO 1, LFO 2 and the envelope move it. Left of centre inverts." },
    { "The display",
      "Drag handle 1 or 2: left/right sets the cutoff, up/down the peak.\n\n"
      "Drag the small centre handle to move both filters together.\n\n"
      "Cmd/Ctrl-drag a numbered handle to move the two filters in opposite directions.\n\n"
      "The thin vertical lines show the live, modulated cutoffs." },
    { "MIDI & shortcuts",
      "MIDI LEARN: click LEARN, click a control, move a controller. Right-click unassigns. The MIDI button switches all MIDI handling off.\n\n"
      "A/B holds two complete settings; COPY copies the current one to the other slot. Every edit is an undo step (Cmd/Ctrl-Z, Cmd/Ctrl-Shift-Z).\n\n"
      "Double-click a knob to type a value; Cmd/Ctrl-click resets it; Shift-drag is fine adjustment; Alt-drag a filter knob to move the same knob on both filters; grab the rim of FREQ, PEAK or PAN to turn it in a circle. Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace caldera
