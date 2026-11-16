# Changelog

## 0.2.0 — 2026-11-16

- Gate **HOLD**: 0 / 50 / 100 / 200 / 400 / 500 ms, shown as a compact pill
  that cycles on click and accepts typed values.
- Gate **DUCK** mode: attenuates material above the threshold, with the same
  range, release and fast-attack controls. A patched external sidechain keys
  gate and ducker detection.
- Gate detector now uses a fast peak envelope, fixing zero-crossing chatter and
  making ducker gain reduction stable.
- Output **LIMIT** pill switches the −0.2 dBFS safety ceiling on or off.
- Typed value entry: right-click any knob, fader or the HOLD pill; Enter
  applies, Esc cancels, Backspace edits. Accepts units and `on`/`off`.
- Double-click a knob or fader to restore its default.
- Dragging a control shows an exact value bubble.
- **A / B / COPY** compare controls in the header (editor-local, not saved in
  host state).
- CLAP and LV2 editors route key events for text entry.
- CLAP state loading accepts states saved with the original 36 parameters.
- Parameter count is now 38; LV2 meter port indices follow automatically.

## 0.1.6 — 2026-11-16

- Output fader unity moved to **3/4 travel**. Top is **+12 dB**, bottom is
  **OFF**, and the lower range uses the cubic gain taper.
- Fader marks updated to +12 / +6 / 0 / −12 / −24 / OFF.
- Factory preset output trims remapped to preserve their previous dB values;
  the default output is now 0.75 (0 dB).
- Note: a saved session with the old 0.5 output position now plays about
  10.5 dB quieter. This is the intentional fader-law change.

## 0.1.5 — 2026-11-16

- Output fader is now **Off at the bottom**, **0 dB at centre** and **+12 dB at
  the top**. The lower half uses a cubic gain taper to reach true silence.
- Fader marks updated to +12 / +6 / 0 / −12 / −24 / OFF.
- The 27×53 Reapertips-style thumb is inset at both ends so it stays inside the
  track at full boost and full attenuation.
- Existing presets around 0.5 remain at 0 dB; no preset retune needed.

## 0.1.4 — 2026-11-16

- Output fader thumb resized to the Reapertips MCP volume-thumb proportions
  (**27×53 logical px**) with a portrait light ribbed grip.
- Gate subheader/control-row cleanup from 0.1.3 retained.

## 0.1.3 — 2026-11-16

- Gate subheader now matches the compressor subheader: title, underline rule
  and mode label (GATE / EXPANDER).
- Gate header moved down clear of the compressor block, including the silver
  76Blue panel.
- Gate controls rearranged into a single threshold/range/release row, removing
  the label collision at the bottom of the dynamics section.

## 0.1.2 — 2026-11-16

- Corrected the input meter tap: it now measures **after the input trim** and
  before the colour stage, instead of before the trim.
- Verified: −20.0 dB at 0 dB trim and −2.0 dB at +18 dB trim for a −20 dBFS
  constant input.

## 0.1.1 — 2026-11-16

- Added a stacked stereo **input level meter** above the output meter, with
  the same LED ladder and dB marks.
- Input metering is pre-trim; output remains post-fader/clip.
- Input and output meters both receive LV2 UI port notifications.

## 0.1.0 — 2026-11-15

First working vertical channel strip.

- New `suite-channel` CLAP + LV2 plugin with the Comp Suite visual language.
- 700×920 three-column panel adapted from the SSL 4000 E layout reference.
- Opens at 1.0× on a 1080p display and scales down on smaller screens.
- Console-coloured EQ fills: HF red, HMF green, LMF blue, LF amber; white
  filter knobs and green gate knobs. Black EQ mode flips the LF fill to black
  and the LF band accent to steel grey, reversibly and in sync with the DSP.
- Brushed-silver compressor panel when 76Blue is engaged.
- LED-ladder GR/output meters and a Reapertips-inspired long output fader.
- Removed the unused lower panel area; the strip now ends with the fader.
- Bug pass: corrected EQ oversampling latency at activation (0 / 23 / 27
  samples for 1x / 2x / 4x); the LV2 UI now requests control-port
  notifications so host presets and automation redraw the panel.
- Regression tests now cover Brown/Black LF colour and DSP difference,
  reversible silver 76Blue panel, every factory preset, expander mode, power
  bypass, all oversampling modes, and VCA↔76 switching. 
- Patina colour section: Valve, Console, Bus, Tape, Transformer, one Amount
  control with auto-gain.
- SSL-style expander/gate with variable hysteresis, Range, Release, Fast Attack
  and Gate/Expander modes.
- Switchable compressor: fixed 3.5:1 VCA and fixed 4:1 76Blue, same control
  surface, 20 ms type crossfade.
- Vendored Dusk 4K EQ 2 core: four bands, Brown/Black, shelf/bell, Q, HPF/LPF,
  native console colour, internal 1x/2x/4x oversampling.
- Constant-latency EQ IN crossfade.
- Vertical output fader, comp/gate GR meters and stereo output meter.
- 23 factory presets across Vocal, Drums, Bass, Guitars, Mix Bus and Utility.
- LV2 metadata generated from `params.hpp` / `presets.hpp`; `--check` test.
- Minimal CLAP and LV2 host smoke tests.
