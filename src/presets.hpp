// Suite Channel — factory presets
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "params.hpp"

namespace suite {

// A preset sets the parameters it cares about; -1 means "leave as is".
struct PresetValue {
  int16_t param = -1;
  float value = 0.0f;
};

struct Preset {
  const char* name;
  const char* category;
  PresetValue values[32];
};

inline constexpr Preset kPresets[] = {
    {"Init", "Utility",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 1.f}, {P_COLOR_AMOUNT, 0.35f},
      {P_COMP_IN, 0.f}, {P_GATE_IN, 0.f}, {P_OUTPUT, 0.75f}, {-1, 0}}},

    // --- Vocals -------------------------------------------------------------
    {"Clean Vocal", "Vocals",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 2.f}, {P_COLOR_AMOUNT, 0.30f},
      {P_HPF, 0.522f}, {P_LF_GAIN, 0.533f}, {P_LF_FREQ, 0.445f},
      {P_LM_GAIN, 0.467f}, {P_LM_FREQ, 0.160f}, {P_LM_Q, 0.533f},
      {P_HM_GAIN, 0.567f}, {P_HM_FREQ, 0.718f}, {P_HM_Q, 0.188f},
      {P_HF_GAIN, 0.533f}, {P_HF_FREQ, 0.801f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.667f}, {P_COMP_AUTO, 1.f},
      {P_OUTPUT, 0.765f}, {-1, 0}}},
    {"Forward Vocal", "Vocals",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 1.f}, {P_COLOR_AMOUNT, 0.38f},
      {P_HPF, 0.594f}, {P_LF_GAIN, 0.55f}, {P_LF_FREQ, 0.445f},
      {P_LM_GAIN, 0.433f}, {P_LM_FREQ, 0.160f}, {P_LM_Q, 0.533f},
      {P_HM_GAIN, 0.60f}, {P_HM_FREQ, 0.718f}, {P_HM_Q, 0.188f},
      {P_HF_GAIN, 0.55f}, {P_HF_FREQ, 0.801f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.733f}, {P_COMP_AUTO, 1.f},
      {P_OUTPUT, 0.75f}, {-1, 0}}},
    {"Rock Vocal", "Vocals",
     {{P_COMP_TYPE, 1.f}, {P_COLOR_MODEL, 0.f}, {P_COLOR_AMOUNT, 0.42f},
      {P_HPF, 0.594f}, {P_LF_GAIN, 0.533f}, {P_LF_FREQ, 0.50f},
      {P_LM_GAIN, 0.433f}, {P_LM_FREQ, 0.363f}, {P_LM_Q, 0.50f},
      {P_HM_GAIN, 0.633f}, {P_HM_FREQ, 0.655f}, {P_HM_Q, 0.188f},
      {P_HF_GAIN, 0.567f}, {P_HF_FREQ, 0.878f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.80f}, {P_COMP_FAST, 1.f},
      {P_COMP_AUTO, 1.f}, {P_OUTPUT, 0.7296f}, {-1, 0}}},
    {"76 Vocal Slam", "Vocals",
     {{P_COMP_TYPE, 1.f}, {P_COLOR_MODEL, 1.f}, {P_COLOR_AMOUNT, 0.45f},
      {P_HPF, 0.522f}, {P_LF_GAIN, 0.55f}, {P_LF_FREQ, 0.445f},
      {P_LM_GAIN, 0.467f}, {P_LM_FREQ, 0.20f}, {P_LM_Q, 0.45f},
      {P_HM_GAIN, 0.60f}, {P_HM_FREQ, 0.718f}, {P_HM_Q, 0.20f},
      {P_HF_GAIN, 0.55f}, {P_HF_FREQ, 0.801f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.90f}, {P_COMP_FAST, 1.f},
      {P_COMP_AUTO, 1.f}, {P_OUTPUT, 0.7097f}, {-1, 0}}},

    // --- Drums --------------------------------------------------------------
    {"Kick Punch", "Drums",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 4.f}, {P_COLOR_AMOUNT, 0.40f},
      {P_HPF, 0.204f}, {P_LF_GAIN, 0.633f}, {P_LF_FREQ, 0.256f},
      {P_LM_GAIN, 0.40f}, {P_LM_FREQ, 0.160f}, {P_LM_Q, 0.262f},
      {P_HM_GAIN, 0.60f}, {P_HM_FREQ, 0.655f}, {P_HM_Q, 0.387f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.667f}, {P_COMP_AUTO, 1.f},
      {P_GATE_IN, 1.f}, {P_GATE_THRESHOLD, 0.05f}, {P_GATE_RANGE, 0.75f},
      {P_GATE_RELEASE, 0.188f}, {P_GATE_MODE, 1.f}, {P_OUTPUT, 0.75f},
      {-1, 0}}},
    {"Snare Crack", "Drums",
     {{P_COMP_TYPE, 1.f}, {P_COLOR_MODEL, 1.f}, {P_COLOR_AMOUNT, 0.42f},
      {P_HPF, 0.522f}, {P_LF_GAIN, 0.55f}, {P_LF_FREQ, 0.445f},
      {P_LM_GAIN, 0.433f}, {P_LM_FREQ, 0.363f}, {P_LM_Q, 0.30f},
      {P_HM_GAIN, 0.60f}, {P_HM_FREQ, 0.772f}, {P_HM_Q, 0.30f},
      {P_HF_GAIN, 0.55f}, {P_HF_FREQ, 0.878f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.80f}, {P_COMP_FAST, 1.f},
      {P_COMP_AUTO, 1.f}, {P_GATE_IN, 1.f}, {P_GATE_THRESHOLD, 0.15f},
      {P_GATE_RANGE, 0.60f}, {P_GATE_RELEASE, 0.188f},
      {P_OUTPUT, 0.7296f}, {-1, 0}}},
    {"Drum Bus", "Drums",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 2.f}, {P_COLOR_AMOUNT, 0.40f},
      {P_HPF, 0.204f}, {P_LF_GAIN, 0.60f}, {P_LF_FREQ, 0.256f},
      {P_LM_GAIN, 0.433f}, {P_LM_FREQ, 0.160f}, {P_LM_Q, 0.262f},
      {P_HM_GAIN, 0.567f}, {P_HM_FREQ, 0.772f}, {P_HM_Q, 0.387f},
      {P_HF_GAIN, 0.55f}, {P_HF_FREQ, 0.878f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.60f}, {P_COMP_AUTO, 1.f},
      {P_OUTPUT, 0.765f}, {-1, 0}}},
    {"Room Crush", "Drums",
     {{P_COMP_TYPE, 1.f}, {P_COLOR_MODEL, 3.f}, {P_COLOR_AMOUNT, 0.50f},
      {P_HPF, 0.30f}, {P_LF_GAIN, 0.55f}, {P_LF_FREQ, 0.35f},
      {P_LM_GAIN, 0.433f}, {P_LM_FREQ, 0.363f}, {P_LM_Q, 0.262f},
      {P_HM_GAIN, 0.60f}, {P_HM_FREQ, 0.772f}, {P_HM_Q, 0.30f},
      {P_HF_GAIN, 0.567f}, {P_HF_FREQ, 0.801f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.867f}, {P_COMP_FAST, 1.f},
      {P_COMP_AUTO, 1.f}, {P_OUTPUT, 0.6903f}, {-1, 0}}},

    // --- Bass ---------------------------------------------------------------
    {"Bass DI", "Bass",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 0.f}, {P_COLOR_AMOUNT, 0.38f},
      {P_HPF, 0.204f}, {P_LF_GAIN, 0.567f}, {P_LF_FREQ, 0.30f},
      {P_LM_GAIN, 0.433f}, {P_LM_FREQ, 0.363f}, {P_LM_Q, 0.262f},
      {P_HM_GAIN, 0.567f}, {P_HM_FREQ, 0.490f}, {P_HM_Q, 0.387f},
      {P_HF_GAIN, 0.533f}, {P_HF_FREQ, 0.707f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.733f}, {P_COMP_AUTO, 1.f},
      {P_OUTPUT, 0.75f}, {-1, 0}}},
    {"Bass Amp", "Bass",
     {{P_COMP_TYPE, 1.f}, {P_COLOR_MODEL, 4.f}, {P_COLOR_AMOUNT, 0.45f},
      {P_HPF, 0.30f}, {P_LF_GAIN, 0.60f}, {P_LF_FREQ, 0.256f},
      {P_LM_GAIN, 0.433f}, {P_LM_FREQ, 0.30f}, {P_LM_Q, 0.262f},
      {P_HM_GAIN, 0.60f}, {P_HM_FREQ, 0.655f}, {P_HM_Q, 0.30f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.80f}, {P_COMP_AUTO, 1.f},
      {P_GATE_IN, 1.f}, {P_GATE_THRESHOLD, 0.15f}, {P_GATE_RANGE, 0.45f},
      {P_GATE_RELEASE, 0.30f}, {P_OUTPUT, 0.7296f}, {-1, 0}}},
    {"Aggressive Bass", "Bass",
     {{P_COMP_TYPE, 1.f}, {P_COLOR_MODEL, 1.f}, {P_COLOR_AMOUNT, 0.48f},
      {P_HPF, 0.204f}, {P_LF_GAIN, 0.567f}, {P_LF_FREQ, 0.30f},
      {P_LM_GAIN, 0.40f}, {P_LM_FREQ, 0.45f}, {P_LM_Q, 0.40f},
      {P_HM_GAIN, 0.633f}, {P_HM_FREQ, 0.655f}, {P_HM_Q, 0.30f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.90f}, {P_COMP_FAST, 1.f},
      {P_COMP_AUTO, 1.f}, {P_OUTPUT, 0.7097f}, {-1, 0}}},

    // --- Guitars ------------------------------------------------------------
    {"Clean Guitar", "Guitars",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 1.f}, {P_COLOR_AMOUNT, 0.32f},
      {P_HPF, 0.522f}, {P_LF_GAIN, 0.467f}, {P_LF_FREQ, 0.35f},
      {P_LM_GAIN, 0.567f}, {P_LM_FREQ, 0.549f}, {P_LM_Q, 0.262f},
      {P_HM_GAIN, 0.60f}, {P_HM_FREQ, 0.655f}, {P_HM_Q, 0.30f},
      {P_HF_GAIN, 0.567f}, {P_HF_FREQ, 0.801f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.667f}, {P_COMP_AUTO, 1.f},
      {P_OUTPUT, 0.75f}, {-1, 0}}},
    {"Double Guitar", "Guitars",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 3.f}, {P_COLOR_AMOUNT, 0.40f},
      {P_HPF, 0.594f}, {P_LF_GAIN, 0.45f}, {P_LF_FREQ, 0.40f},
      {P_LM_GAIN, 0.567f}, {P_LM_FREQ, 0.549f}, {P_LM_Q, 0.30f},
      {P_HM_GAIN, 0.60f}, {P_HM_FREQ, 0.718f}, {P_HM_Q, 0.262f},
      {P_HF_GAIN, 0.533f}, {P_HF_FREQ, 0.878f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.667f}, {P_COMP_AUTO, 1.f},
      {P_OUTPUT, 0.75f}, {-1, 0}}},
    {"Lead Guitar", "Guitars",
     {{P_COMP_TYPE, 1.f}, {P_COLOR_MODEL, 0.f}, {P_COLOR_AMOUNT, 0.44f},
      {P_HPF, 0.594f}, {P_LF_GAIN, 0.533f}, {P_LF_FREQ, 0.35f},
      {P_LM_GAIN, 0.567f}, {P_LM_FREQ, 0.549f}, {P_LM_Q, 0.30f},
      {P_HM_GAIN, 0.633f}, {P_HM_FREQ, 0.655f}, {P_HM_Q, 0.262f},
      {P_HF_GAIN, 0.55f}, {P_HF_FREQ, 0.801f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.733f}, {P_COMP_FAST, 1.f},
      {P_COMP_AUTO, 1.f}, {P_OUTPUT, 0.7296f}, {-1, 0}}},

    // --- Mix bus ------------------------------------------------------------
    {"Mix Bus Glue", "Mix Bus",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 2.f}, {P_COLOR_AMOUNT, 0.28f},
      {P_LF_GAIN, 0.533f}, {P_LF_FREQ, 0.20f},
      {P_LM_GAIN, 0.467f}, {P_LM_FREQ, 0.363f}, {P_LM_Q, 0.188f},
      {P_HM_GAIN, 0.517f}, {P_HM_FREQ, 0.772f}, {P_HM_Q, 0.188f},
      {P_HF_GAIN, 0.55f}, {P_HF_FREQ, 0.973f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.60f}, {P_COMP_AUTO, 1.f},
      {P_OUTPUT, 0.7296f}, {-1, 0}}},
    {"Mix Bus Punch", "Mix Bus",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 4.f}, {P_COLOR_AMOUNT, 0.30f},
      {P_LF_GAIN, 0.55f}, {P_LF_FREQ, 0.256f},
      {P_LM_GAIN, 0.467f}, {P_LM_FREQ, 0.30f}, {P_LM_Q, 0.20f},
      {P_HM_GAIN, 0.533f}, {P_HM_FREQ, 0.655f}, {P_HM_Q, 0.20f},
      {P_HF_GAIN, 0.533f}, {P_HF_FREQ, 0.878f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.667f}, {P_COMP_FAST, 1.f},
      {P_COMP_AUTO, 1.f}, {P_OUTPUT, 0.7296f}, {-1, 0}}},
    {"Mix Bus Air", "Mix Bus",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 1.f}, {P_COLOR_AMOUNT, 0.25f},
      {P_LF_GAIN, 0.517f}, {P_LF_FREQ, 0.20f},
      {P_LM_GAIN, 0.467f}, {P_LM_FREQ, 0.363f}, {P_LM_Q, 0.188f},
      {P_HM_GAIN, 0.517f}, {P_HM_FREQ, 0.772f}, {P_HM_Q, 0.20f},
      {P_HF_GAIN, 0.583f}, {P_HF_FREQ, 0.973f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.55f}, {P_COMP_AUTO, 1.f},
      {P_OUTPUT, 0.7296f}, {-1, 0}}},

    // --- Utility ------------------------------------------------------------
    {"Gate Tight", "Utility",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 1.f}, {P_COLOR_AMOUNT, 0.30f},
      {P_COMP_IN, 0.f}, {P_GATE_IN, 1.f}, {P_GATE_THRESHOLD, 0.05f},
      {P_GATE_RANGE, 1.0f}, {P_GATE_RELEASE, 0.15f}, {P_GATE_FAST, 1.f},
      {P_GATE_MODE, 1.f}, {P_OUTPUT, 0.75f}, {-1, 0}}},
    {"Expander Gentle", "Utility",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 1.f}, {P_COLOR_AMOUNT, 0.30f},
      {P_COMP_IN, 0.f}, {P_GATE_IN, 1.f}, {P_GATE_THRESHOLD, 0.0f},
      {P_GATE_RANGE, 0.30f}, {P_GATE_RELEASE, 0.50f}, {P_GATE_MODE, 0.f},
      {P_OUTPUT, 0.75f}, {-1, 0}}},
    {"Colour Only", "Utility",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 3.f}, {P_COLOR_AMOUNT, 0.60f},
      {P_COMP_IN, 0.f}, {P_GATE_IN, 0.f}, {P_OUTPUT, 0.75f}, {-1, 0}}},
    {"76 Drum Smash", "Drums",
     {{P_COMP_TYPE, 1.f}, {P_COLOR_MODEL, 3.f}, {P_COLOR_AMOUNT, 0.50f},
      {P_HPF, 0.204f}, {P_LF_GAIN, 0.60f}, {P_LF_FREQ, 0.256f},
      {P_LM_GAIN, 0.433f}, {P_LM_FREQ, 0.30f}, {P_LM_Q, 0.262f},
      {P_HM_GAIN, 0.567f}, {P_HM_FREQ, 0.772f}, {P_HM_Q, 0.30f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.90f}, {P_COMP_FAST, 1.f},
      {P_COMP_AUTO, 1.f}, {P_OUTPUT, 0.6999f}, {-1, 0}}},
    {"Bass Bus", "Bass",
     {{P_COMP_TYPE, 0.f}, {P_COLOR_MODEL, 2.f}, {P_COLOR_AMOUNT, 0.32f},
      {P_HPF, 0.204f}, {P_LF_GAIN, 0.567f}, {P_LF_FREQ, 0.28f},
      {P_LM_GAIN, 0.467f}, {P_LM_FREQ, 0.363f}, {P_LM_Q, 0.262f},
      {P_HM_GAIN, 0.533f}, {P_HM_FREQ, 0.490f}, {P_HM_Q, 0.30f},
      {P_COMP_IN, 1.f}, {P_COMP_THRESHOLD, 0.667f}, {P_COMP_AUTO, 1.f},
      {P_OUTPUT, 0.75f}, {-1, 0}}},
};

inline constexpr int kPresetCount =
    static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0]));

} // namespace suite
