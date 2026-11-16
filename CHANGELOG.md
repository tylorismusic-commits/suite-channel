# Changelog

## 0.1.0 — 2026-11-15

First working vertical channel strip.

- New `suite-channel` CLAP + LV2 plugin with the Comp Suite visual language.
- 700×920 three-column panel adapted from the SSL 4000 E layout reference.
- Opens at 1.0× on a 1080p display and scales down on smaller screens.
- Console-coloured EQ fills: HF red, HMF green, LMF blue, LF amber; white
  filter knobs and green gate knobs.
- Brushed-silver compressor panel when 76Blue is engaged.
- LED-ladder GR/output meters and a Reapertips-inspired long output fader.
- Removed the unused lower panel area; the strip now ends with the fader. 
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
