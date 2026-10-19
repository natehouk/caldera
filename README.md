# Caldera

**A stereo dual-filter effect plugin for macOS, Windows and Linux (VST3 · AU · CLAP · VST2).**

Part of **[Epsilon Audio](https://github.com/natehouk/epsilon-plugins)**, a family of free, open-source plugins: Monolith, Caldera, Obsidian, Strata, Basalt, Gneiss, Mica, Flint, Granite and Magma.

![Caldera](screenshot.png)

Caldera is a clean-room design with the feature set of FabFilter Volcano, FabFilter's second plugin (May 2005). It was written from scratch, working only from FabFilter's public press release and the Volcano user manual. No FabFilter code, binaries, presets, artwork or trademarks were used.

The filters are cascaded topology-preserving state-variable filters (Zavalishin) with Butterworth stage damping. Each style adds its own published-technique nonlinearity.

Caldera is the sibling of [Monolith](https://github.com/natehouk/monolith), and Monolith's filter is one of its seven styles.

## Download

Get the latest build from [**Releases**](../../releases):

| Platform | File | Contains |
|---|---|---|
| macOS 10.13+ (Apple Silicon + Intel) | `Caldera-macos-universal.zip` | VST3, AU (MIDI-controlled effect), CLAP |
| Windows x64 | `Caldera-windows-x64.zip` | VST3, CLAP, VST2 |
| Linux x64 | `Caldera-linux-x64.zip` | VST3, CLAP, VST2 |

### Install

| Platform | Copy | To |
|---|---|---|
| macOS | `Caldera.vst3` / `Caldera.component` / `Caldera.clap` | `~/Library/Audio/Plug-Ins/VST3` / `Components` / `CLAP` |
| Windows | `Caldera.vst3` (folder) / `Caldera.clap` | `C:\Program Files\Common Files\VST3\` / `CLAP\` |
| Linux | `Caldera.vst3` / `Caldera.clap` | `~/.vst3/` / `~/.clap/` |

**macOS:** the plugins are ad-hoc signed, not notarized. After copying, run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Caldera*
```

**Logic Pro:** to use MIDI features, insert Caldera as an **AU MIDI-controlled effect** on an instrument track. MIDI features are key tracking, MIDI-triggered envelope, LFO retrigger and MIDI Learn.

## Features

**Two filters**
- Each filter can be switched on or off independently.
- **Seven styles**, from transparent to self-oscillating:

  | Style | Character |
  |---|---|
  | **Clean** | Linear, high-Q, never self-oscillates |
  | **Smooth** | Gentle, broad resonance |
  | **Warm** | Saturated input into a linear filter |
  | **Monolith** | The Monolith synth's filter: soft-limited, self-oscillating |
  | **Hot** | Saturating integrators, screaming peak |
  | **Acid** | Resonance thins the bass, squelchy |
  | **Raw** | Hard-clipped feedback, buzzy self-oscillation |

- **Low pass / band pass / high pass** at **12, 24 or 48 dB/oct**.
- **Freq** (20 Hz–20 kHz) and **Peak** (resonance) controls.
- **Pan:** offsets the cutoff between the left and right channels (up to ±2 octaves) for phase-free stereo movement.
- **KEY:** the cutoff follows the MIDI notes you play. The Freq knob sets the cutoff at middle C.

**Routing and output**
- **Serial** (filter 1 → filter 2) or **parallel** (both filters read the input and their outputs are mixed).
- **Input** gain (±24 dB) drives the saturating styles.
- **Output** gain and dry/wet **Mix**.
- **Auto Mute:** silences self-oscillation 50 ms after the input goes quiet.

**Modulation**
- **2 LFOs:**
  - Triangle or square, with **Balance**. For triangle it skews from a falling ramp through symmetric to a rising ramp; for square it sets the duty cycle.
  - **Free** (0.01–100 Hz) or synced to host tempo and song position: **straight**, **dotted** or **triplet**, from 16 bars to 1/64.
  - **RETRIG** restarts the LFO on each MIDI note.
- **Envelope (ADSR):**
  - By default it triggers when the input level crosses **Thresh**, which makes it work as an envelope follower or gate.
  - **MIDI TRIG** triggers it from notes instead.
  - The light in the panel shows when the envelope is active.
- **Modulation grid:**
  - Each source (LFO 1, LFO 2, Env) has its own bipolar amount knob next to each filter's **Freq**, **Peak** and **Pan**.
  - Turning an amount left applies the inverted signal.
  - Freq modulation reaches ±4 octaves.

**Interactive display**
- Shows both filter curves, the combined response (serial product or parallel sum), and faint left/right curves when pan is used.
- Live markers show where modulation is moving each cutoff.
- Drag handle **1** or **2** to set freq (horizontal) and peak (vertical).
- Drag the **center handle** to move both filters together.
- **Cmd/Ctrl-drag** a filter handle to move the two filters in opposite directions.

**MIDI Learn**
1. Click **LEARN**.
2. Click any control, then move a knob or fader on your controller.
3. Right-click a control to unassign it. **Clear all assignments** is in the display.

Assignments are saved with your project. The **MIDI** button turns all MIDI handling off, for hosts that send MIDI to every effect.

**Presets**
- 41 factory presets in six categories: Sweeps, Rhythmic, Resonant, Stereo, Character, Utility.
- **Save current as…** stores your own presets as small text files, which load the same on every OS:
  - macOS: `~/Library/Audio/Presets/Epsilon Audio/Caldera/`
  - Windows: `%APPDATA%\Epsilon Audio\Caldera\Presets\`
  - Linux: `~/.config/epsilon-audio/caldera/presets/`
- To delete a user preset, right-click it, then click it again to confirm.
- A `*` after the preset name means you've edited the sound since loading it.

![Presets](screenshot-presets.png)

**Interface**

| Action | Result |
|---|---|
| Drag up/down | Adjust a knob (hold **Shift** for fine) |
| **Alt**-drag a filter knob | Move the same knob on both filters |
| Mouse wheel | Step the value |
| **Double-click** | Type a value (`1.2k`, `L40`, `-6`, `250ms`, `1/8`), then **Enter** |
| **Cmd/Ctrl-click** | Reset to default |
| Hover | The parameter name and value appear in the display |

The window is resizable and keeps its aspect ratio. Every parameter is automatable, and parameter changes are smoothed, so there's no zipper noise.

### Compared with FabFilter Volcano (2005)

| Volcano | Caldera |
|---|---|
| Two filters, serial or parallel | ✓ |
| Multiple filter characteristics, incl. FabFilter One's | ✓ 7 styles, incl. Monolith's filter |
| LP / HP / BP at 12 / 24 / 48 dB/oct | ✓ |
| Per-filter stereo pan (cutoff offset L/R) | ✓ |
| Per-filter MIDI key tracking, per-filter bypass | ✓ |
| Interactive display: drag one or both, Cmd/Ctrl for opposite directions | ✓ |
| Input gain, output gain, effect mix, auto-mute self-oscillation | ✓ |
| 2 LFOs: triangle/square + balance, free or sync (straight/dotted/triplet, 16 bars–1/64), MIDI retrigger | ✓ |
| ADSR envelope with audio threshold or MIDI trigger, activity light | ✓ |
| Per source/destination modulation amounts for freq, peak and pan (bipolar) | ✓ |
| MIDI Learn with saved assignments, MIDI on/off switch | ✓ |
| Factory presets in categories, save/delete your own, cross-platform preset files | ✓ |
| Knob rotate-mode | Vertical drag only |
| Pro Tools (RTAS/AAX) | Not supported (needs Avid's SDK and signing) |

## Build from source

The plugin is built on [DPF](https://github.com/DISTRHO/DPF):

```
git clone --recursive https://github.com/DISTRHO/DPF.git dpf
make -C plugins/Caldera                    # Linux / macOS (macOS also builds the AU)
make -C plugins/Caldera WINDOWS=true CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++
```

GitHub Actions builds all three platforms on every push. The macOS build is universal and ad-hoc signed. A `v*` tag, or a release published on GitHub, attaches the zips to that release.

## Tests

```
cd tests
g++ -O2 -std=c++11 response.cpp -o response && ./response   # all 63 style × type × slope combinations: passband and slope
g++ -O2 -std=c++11 features.cpp -o features && ./features   # self-oscillation per style, pan, routing, EG triggers, key tracking, LFO sync, auto-mute
g++ -O2 -std=c++11 fuzz.cpp -o fuzz && ./fuzz               # random params/MIDI/transport, loud input, 44.1–192 kHz: finite, ≤ 0 dBFS
g++ -O2 -std=c++11 presets.cpp -o presets && ./presets      # every factory preset on test material
```

Release checks:
- Steinberg's VST3 validator passes 47/47.
- The interface is exercised with scripted input: filter toggles, style stepper, segment switches, knob and mod-amount drags, display-handle drag, MIDI Learn mode, and preset save and reload.

## Source layout

| File | Contents |
|---|---|
| `plugins/Caldera/CalderaEngine.hpp` | All DSP: filter styles, routing, LFOs, envelope, modulation. Header-only |
| `plugins/Caldera/ParamDefs.hpp` | Parameter table, labels, value mapping |
| `plugins/Caldera/Presets.hpp` | Factory presets |
| `plugins/Caldera/UserPresets.hpp` | User preset files |
| `plugins/Caldera/CalderaPlugin.cpp` | Plugin wrapper: parameters, programs, MIDI, MIDI Learn, transport |
| `plugins/Caldera/CalderaUI.cpp` | Custom vector interface (NanoVG) |

New parameters are only ever appended, so saved sessions and automation keep working across versions.

## License

[MIT](LICENSE). Built with [DPF](https://github.com/DISTRHO/DPF) and its bundled NanoVG, which are under the ISC license. `plugins/Caldera/ResizeHandle.hpp` comes from DPF's examples and keeps its ISC notice.

"FabFilter" and "FabFilter Volcano" are trademarks of FabFilter Software Instruments. Caldera is not affiliated with or endorsed by FabFilter.
