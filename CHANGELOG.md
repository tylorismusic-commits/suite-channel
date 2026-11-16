# Changelog

## 0.1.0 — 2026-11-15

First working vertical channel strip.

- New `suite-channel` CLAP + LV2 plugin with the Comp Suite visual language.
- 560×1040 three-column panel adapted from the SSL 4000 E layout reference.
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
