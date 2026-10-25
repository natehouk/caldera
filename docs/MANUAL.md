# Caldera 2.0 — User Manual

Caldera is a stereo dual-filter effect. Two filters with seven characters each run in series or in parallel, moved by two LFOs and an envelope. Typical uses:

- filter sweeps, risers and fallers
- rhythmic, tempo-synced filter movement and gating
- resonant, singing and self-oscillating tones
- stereo movement and width
- character: telephone, radio, lo-fi, warm drive

Caldera runs as VST3, AU, CLAP and VST2 on macOS, Windows and Linux.

**Contents**

1. [Installing](#1-installing)
2. [A quick tour](#2-a-quick-tour)
3. [The filters](#3-the-filters)
4. [Routing and output](#4-routing-and-output)
5. [The LFOs](#5-the-lfos)
6. [The envelope](#6-the-envelope)
7. [The modulation knobs](#7-the-modulation-knobs)
8. [The display](#8-the-display)
9. [Knobs](#9-knobs)
10. [A/B, undo and redo](#10-ab-undo-and-redo)
11. [MIDI](#11-midi)
12. [Presets](#12-presets)
13. [Help and hover tips](#13-help-and-hover-tips)
14. [Recipes](#14-recipes)
15. [Parameter reference](#15-parameter-reference)
16. [Troubleshooting](#16-troubleshooting)

---

## 1. Installing

Download the zip for your system from the project's **Releases** page and copy the plugin files:

| System | Copy | To |
|---|---|---|
| macOS | `Caldera.vst3`, `Caldera.component` (AU), `Caldera.clap` | `~/Library/Audio/Plug-Ins/VST3`, `.../Components`, `.../CLAP` |
| Windows | the `Caldera.vst3` folder, `Caldera.clap` | `C:\Program Files\Common Files\VST3\`, `...\CLAP\` |
| Linux | `Caldera.vst3`, `Caldera.clap` | `~/.vst3/`, `~/.clap/` |

**macOS:** if the build isn't notarized and macOS blocks it, open Terminal and run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Caldera*
```

Rescan plugins in your DAW. Caldera appears under **Epsilon Audio** as an effect.

**Logic Pro:** to use the MIDI features (key tracking, MIDI-triggered envelope, LFO retrigger, MIDI Learn), insert Caldera as an **AU MIDI-controlled effect** on an instrument track.

Projects saved with Caldera 1.x open unchanged in 2.0.

---

## 2. A quick tour

![Caldera](../screenshot.png)

- **Header:** the preset browser, **A / B** and **COPY**, **UNDO / REDO**, **LEARN** (MIDI Learn), **MIDI** on/off and **HELP**.
- **The display:** both filter curves, the combined response, the live modulated cutoffs and a draggable handle for each filter.
- **FILTER 1** and **FILTER 2** panels:
  - an on/off switch, the **style**, type (LP, BP, HP), slope (12, 24, 48) and **KEY**
  - **FREQ**, **PEAK** and **PAN**, each with three small modulation knobs: LFO1, LFO2 and ENV
- **Bottom row:**
  - **GLOBAL:** SERIAL or PARALLEL, INPUT, OUTPUT, MIX, AUTO MUTE
  - **LFO 1** and **LFO 2:** shape, wave, sync mode, RATE, BALANCE, RETRIG
  - **ENVELOPE:** shape, activity light, MIDI TRIG, A, D, S, R, THRESH

---

## 3. The filters

Each filter can be switched on or off on its own.

**Styles.** Click the arrows on either side of the style name, or scroll over it:

| Style | Character |
|---|---|
| **Clean** | Linear and transparent; high resonance, but never self-oscillates |
| **Smooth** | Gentle, broad resonance |
| **Warm** | Saturated input into a linear filter |
| **Monolith** | The Monolith synth's filter: soft-limited and self-oscillating |
| **Hot** | Saturating, with a screaming peak |
| **Acid** | Resonance thins the bass, for squelch |
| **Raw** | Hard-clipped feedback, buzzy self-oscillation |

**Type and slope.** **LP** (low pass) keeps the lows, **BP** (band pass) a band around the cutoff, **HP** (high pass) the highs. The slope is 12, 24 or 48 dB per octave.

**Controls**
- **FREQ:** the cutoff, 20 Hz to 20 kHz.
- **PEAK:** resonance. In the self-oscillating styles, the filter sings on its own near the top.
- **PAN:** offsets the cutoff between the left and right channels, up to two octaves either way. The stereo image moves without the phase problems of real panning.
- **KEY:** the cutoff follows the MIDI notes you play; FREQ then sets the cutoff at middle C.

---

## 4. Routing and output

- **SERIAL:** the signal goes through filter 1, then filter 2.
- **PARALLEL:** both filters get the input and their outputs are mixed.
- **INPUT** (±24 dB) drives the saturating styles harder.
- **OUTPUT** (±24 dB) and **MIX** (dry/wet) set the result.
- **AUTO MUTE** silences self-oscillation 50 ms after the input goes quiet.

---

## 5. The LFOs

Each LFO has:

- **Wave:** **TRI** (triangle) or **SQR** (square).
- **BALANCE:** for the triangle, it skews the shape from a falling ramp through symmetric (50%) to a rising ramp. For the square, it sets the duty cycle.
- **Sync mode:**
  - **FREE:** RATE is in Hz, 0.01 to 100.
  - **STR**, **DOT**, **TRIP:** locked to the host tempo and song position, with straight, dotted or triplet note lengths from 16 bars to 1/64. RATE then picks the length.
- **RETRIG:** restarts the LFO on every MIDI note.

The picture above the controls shows the shape; the rate or note length is in its corner.

---

## 6. The envelope

The envelope is an ADSR (**A**, **D**, **S**, **R**) that starts when the input level rises past **THRESH** and releases when it falls below. That makes it an envelope follower or a gate: it reacts to the dynamics of a drum loop, a guitar or a vocal.

With **MIDI TRIG** on, MIDI notes trigger it instead, and THRESH is greyed out.

The light at the top right of the panel shows when the envelope is active.

---

## 7. The modulation knobs

Next to each filter's **FREQ**, **PEAK** and **PAN** are three small knobs: **LFO1**, **LFO2** and **ENV**. Each one sets how much that source moves that control.

- Centre is off. Turning right applies the source; turning left applies it inverted.
- **FREQ** modulation reaches four octaves either way.
- **PAN** modulation swings the left and right cutoffs apart.

The thin vertical lines in the display show where each cutoff is right now.

---

## 8. The display

- **Curves:**
  - filter 1 in orange, filter 2 in teal
  - the combined response in white (the product in serial, the sum in parallel)
  - faint left and right curves when PAN is used
- **Handles:**
  - Drag handle **1** or **2**: left and right set the cutoff, up and down the peak.
  - Drag the small **centre handle** to move both filters together.
  - **Cmd/Ctrl-drag** a numbered handle to move the two filters in opposite directions.
- The line at the top shows the name and value of whatever is under the pointer.

---

## 9. Knobs

- **Drag** up or down; hold **Shift** for fine adjustment.
- **Grab the rim** of FREQ, PEAK or PAN to turn it in a circle.
- **Alt-drag** a filter knob to move the same knob on both filters.
- **Mouse wheel** turns a knob.
- **Double-click** to type a value: `1.2k`, `L40`, `-6`, `250ms`, `1/8`. **Enter** applies it, **Esc** cancels.
- **Cmd/Ctrl-click** resets to the default.

---

## 10. A/B, undo and redo

- **A / B** hold two complete settings. Click the other letter to switch; the first time, B starts as a copy of A.
- **COPY** copies the current setting into the other slot.
- Every edit is an undo step, including preset loads and A/B switches. Caldera remembers the last 200.
  - Undo: **UNDO**, or Cmd/Ctrl-Z
  - Redo: **REDO**, or Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y

---

## 11. MIDI

**MIDI Learn**
1. Click **LEARN**.
2. Click a knob or switch.
3. Move a knob or fader on your controller.
4. Click **LEARN** again (or press Esc) to finish.

- Right-click a control to unassign it.
- **CLEAR ALL ASSIGNMENTS** (top right of the display) removes them all.
- Assignments are saved with your project.

**The MIDI button** switches all MIDI handling off: key tracking, MIDI triggers, LFO retrigger and learned controllers. Use it in hosts that send MIDI to every effect.

---

## 12. Presets

There are 41 factory presets in six groups, plus your own:

| Group | Presets |
|---|---|
| Sweeps | Slow Sweep, Bar Sweep, Riser (4 bars), Faller (4 bars), Band Sweep, Dual Sweep, Notch Sweep |
| Rhythmic | Gate 1/8, Gate 1/16, Wobble 1/8, Triplet Wobble, Dotted Pump, Trance Gate, Envelope Follow |
| Resonant | Acid Squelch, Scream, Whistler, Keyed Ring, Raw Buzz, Peak Wander, Formant |
| Stereo | Wide, Pan Sweep, Stereo Chop, Spinner, Mirrored, Width Envelope |
| Character | Warm Tape, Telephone, Old Radio, Muffled, Crunch HP, Underwater, Lo-Fi Drive |
| Utility | Low Cut 80 Hz, High Cut 12 kHz, Band Limit, DJ Low Pass, DJ High Pass, Key Follow |

**Init** is a plain starting point.

**Browsing**
- Click the preset name to open the browser.
- **◀ ▶**, or the mouse wheel over the name, step through factory and user presets.
- A `*` after the name means you have changed the setting since loading it.

**Your own presets**
- **SAVE CURRENT AS…** in the browser stores your own presets, in one of these folders:
  - macOS: `~/Library/Audio/Presets/Epsilon Audio/Caldera`
  - Windows: `%APPDATA%\Epsilon Audio\Caldera\Presets`
  - Linux: `~/.config/epsilon-audio/caldera/presets`
- Preset files are plain text and work on every system.
- To delete one, right-click it in the browser and click again to confirm.

---

## 13. Help and hover tips

- Rest the pointer on any control, handle, display or header button to see what it does and its current value.
- **HELP** opens the in-plugin pages. The switch at the bottom of the help window turns hover tips on or off.
- **FULL MANUAL (WEB)** opens this manual.

---

## 14. Recipes

- **Classic sweep:** filter 1 LP 24, MONOLITH, PEAK 50%. Set LFO 1 to STR, 4 bars, and its FREQ amount to +60%.
- **Tempo gate:**
  - LFO 1 SQR, STR 1/16
  - filter 1 LP, with LFO1 > FREQ fully up
- **Drum-loop follower:**
  - ENVELOPE with THRESH just under the peaks, short attack, medium release
  - ENV > FREQ +50% on a band pass
- **Self-oscillating whistle:**
  - HOT or RAW, PEAK at the top, KEY on
  - play notes into Caldera; AUTO MUTE stops the ringing between phrases
- **Stereo motion:** PAN 30%, plus a slow LFO to PAN.
- **Telephone:** filter 1 HP 24 at about 400 Hz, filter 2 LP 24 at about 3 kHz, in serial, WARM style.
- **Opposite sweep:** Cmd/Ctrl-drag handle 1: filter 2 moves the other way, for opening and closing bands.

---

## 15. Parameter reference

Every control is automatable. New parameters are only ever added at the end, so sessions and automation keep working across versions.

- **Each filter:**
  - On, Style (Clean, Smooth, Warm, Monolith, Hot, Acid, Raw)
  - Type (Low Pass, Band Pass, High Pass), Slope (12, 24, 48 dB/oct)
  - Freq (20 Hz-20 kHz), Peak, Pan (±100%), MIDI key tracking
- **Global:** Routing (Serial, Parallel), Input Gain (±24 dB), Output Gain (±24 dB), Mix, Auto Mute.
- **Each LFO:**
  - Wave (Triangle, Square), Balance
  - Sync (Free, Straight, Dotted, Triplet), Rate (0.01-100 Hz), Division (16 bars-1/64)
  - MIDI Retrigger
- **Envelope:**
  - Attack, Decay, Sustain, Release
  - Threshold (-60 to 0 dB), MIDI Trigger
- **Modulation:** 18 bipolar amounts, from LFO 1, LFO 2 and Env to each filter's Freq, Peak and Pan.
- **MIDI:** on/off.

Caldera also reports a few output values that hosts may list (envelope level, the live cutoffs and a MIDI Learn message); they are for the interface.

---

## 16. Troubleshooting

- **Caldera doesn't appear in my DAW.** Check the install folders above and rescan plugins. On macOS, run the `xattr` command from section 1.
- **KEY, MIDI TRIG or RETRIG do nothing.** The track must send MIDI to Caldera (in Logic, insert it as an AU MIDI-controlled effect), and the MIDI button must be on.
- **The LFO doesn't follow the tempo.** Choose STR, DOT or TRIP instead of FREE.
- **A filter whistles in silence.** Turn on AUTO MUTE, or lower PEAK in the self-oscillating styles.
- **It's suddenly loud.** The saturating styles add level with high INPUT and PEAK; lower INPUT or OUTPUT.
- **Hover tips get in the way.** Turn them off with the switch at the bottom of the HELP window.
- **Latency:** Caldera has none.
