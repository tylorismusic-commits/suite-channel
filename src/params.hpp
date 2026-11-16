// Suite Channel — params.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

#include "dsp/controls.hpp"

namespace suite {

// Parameter ids are the CLAP parameter ids and the index into Shared::values.
// They are also the ordering of the LV2 control ports (port = 4 + id), so this
// enum must only ever be appended to.
enum ParamId : uint32_t {
  P_ENABLED = 0,
  P_INPUT,
  P_POLARITY,
  P_COLOR_MODEL,
  P_COLOR_AMOUNT,
  P_HPF,
  P_LPF,
  P_EQ_TYPE,
  P_EQ_IN,
  P_LF_GAIN,
  P_LF_FREQ,
  P_LF_BELL,
  P_LM_GAIN,
  P_LM_FREQ,
  P_LM_Q,
  P_HM_GAIN,
  P_HM_FREQ,
  P_HM_Q,
  P_HF_GAIN,
  P_HF_FREQ,
  P_HF_BELL,
  P_COMP_TYPE,
  P_COMP_IN,
  P_COMP_THRESHOLD,
  P_COMP_MAKEUP,
  P_COMP_FAST,
  P_COMP_AUTO,
  P_GATE_IN,
  P_GATE_THRESHOLD,
  P_GATE_RANGE,
  P_GATE_RELEASE,
  P_GATE_FAST,
  P_GATE_MODE,
  P_DYN_POST,
  P_OUTPUT,
  P_OVERSAMPLE,
  P_GATE_HOLD,
  P_LIMITER_ON,
  P_COUNT
};

struct Labels {
  const char* const* names = nullptr;
  int count = 0;
};

template <size_t N>
constexpr Labels labels(const char* const (&arr)[N]) noexcept {
  return Labels{arr, static_cast<int>(N)};
}

// `symbol` is the LV2 port symbol and part of the saved-session format.
struct ParamSpec {
  const char* symbol;
  const char* name;
  const char* module;
  double min;
  double max;
  double def;
  bool stepped;
  bool hidden;
  bool toggled;
  const char* unit;
  Labels labels;
};

inline constexpr const char* kColourLabels[] = {"Valve", "Console", "Bus",
                                                "Tape", "Transformer"};
inline constexpr const char* kCompTypeLabels[] = {"VCA", "76Blue"};
inline constexpr const char* kGateModeLabels[] = {"Expander", "Gate", "Ducker"};
inline constexpr const char* kOversampleLabels[] = {"Off", "2x", "4x"};
inline constexpr const char* kEqTypeLabels[] = {"Brown", "Black"};

inline constexpr ParamSpec kParamSpecs[P_COUNT] = {
    {"power", "Power", "Global", 0.0, 1.0, 1.0, true, false, true, nullptr, {}},
    {"input", "Input", "Input", 0.0, 1.0, 0.5, false, false, false, "dB", {}},
    {"polarity", "Polarity", "Input", 0.0, 1.0, 0.0, true, false, true, nullptr, {}},
    {"color_model", "Colour", "Colour", 0.0, 4.0, 1.0, true, false, false,
     nullptr, labels(kColourLabels)},
    {"color_amount", "Colour Amount", "Colour", 0.0, 1.0, 0.35, false, false,
     false, nullptr, {}},
    {"hpf", "HPF", "Filters", 0.0, 1.0, 0.0, false, false, false, "Hz", {}},
    {"lpf", "LPF", "Filters", 0.0, 1.0, 0.0, false, false, false, "Hz", {}},
    {"eq_type", "EQ Type", "EQ", 0.0, 1.0, 0.0, true, false, false, nullptr,
     labels(kEqTypeLabels)},
    {"eq_in", "EQ In", "EQ", 0.0, 1.0, 1.0, true, false, true, nullptr, {}},
    {"lf_gain", "LF Gain", "EQ", 0.0, 1.0, 0.5, false, false, false, "dB", {}},
    {"lf_freq", "LF Frequency", "EQ", 0.0, 1.0, 0.70, false, false, false, "Hz",
     {}},
    {"lf_bell", "LF Bell", "EQ", 0.0, 1.0, 0.0, true, false, true, nullptr, {}},
    {"lm_gain", "LMF Gain", "EQ", 0.0, 1.0, 0.5, false, false, false, "dB", {}},
    {"lm_freq", "LMF Frequency", "EQ", 0.0, 1.0, 0.637, false, false, false,
     "Hz", {}},
    {"lm_q", "LMF Q", "EQ", 0.0, 1.0, 0.4, false, false, false, nullptr, {}},
    {"hm_gain", "HMF Gain", "EQ", 0.0, 1.0, 0.5, false, false, false, "dB", {}},
    {"hm_freq", "HMF Frequency", "EQ", 0.0, 1.0, 0.655, false, false, false,
     "Hz", {}},
    {"hm_q", "HMF Q", "EQ", 0.0, 1.0, 0.4, false, false, false, nullptr, {}},
    {"hf_gain", "HF Gain", "EQ", 0.0, 1.0, 0.5, false, false, false, "dB", {}},
    {"hf_freq", "HF Frequency", "EQ", 0.0, 1.0, 0.707, false, false, false,
     "Hz", {}},
    {"hf_bell", "HF Bell", "EQ", 0.0, 1.0, 0.0, true, false, true, nullptr, {}},
    {"comp_type", "Compressor Type", "Dynamics", 0.0, 1.0, 0.0, true, false,
     false, nullptr, labels(kCompTypeLabels)},
    {"comp_in", "Compressor In", "Dynamics", 0.0, 1.0, 0.0, true, false, true,
     nullptr, {}},
    {"comp_threshold", "Compressor Threshold", "Dynamics", 0.0, 1.0, 0.73,
     false, false, false, "dB", {}},
    {"comp_makeup", "Compressor Makeup", "Dynamics", 0.0, 1.0, 0.5, false,
     false, false, "dB", {}},
    {"comp_fast", "Compressor Fast Attack", "Dynamics", 0.0, 1.0, 0.0, true,
     false, true, nullptr, {}},
    {"comp_auto", "Compressor Auto Makeup", "Dynamics", 0.0, 1.0, 1.0, true,
     false, true, nullptr, {}},
    {"gate_in", "Gate In", "Dynamics", 0.0, 1.0, 0.0, true, false, true, nullptr,
     {}},
    {"gate_threshold", "Gate Threshold", "Dynamics", 0.0, 1.0, 0.25, false,
     false, false, "dB", {}},
    {"gate_range", "Gate Range", "Dynamics", 0.0, 1.0, 0.6, false, false, false,
     "dB", {}},
    {"gate_release", "Gate Release", "Dynamics", 0.0, 1.0, 0.5, false, false,
     false, "s", {}},
    {"gate_fast", "Gate Fast Attack", "Dynamics", 0.0, 1.0, 0.0, true, false,
     true, nullptr, {}},
    {"gate_mode", "Gate Mode", "Dynamics", 0.0, 2.0, 1.0, true, false, false,
     nullptr, labels(kGateModeLabels)},
    {"dyn_post", "Dynamics Post EQ", "Dynamics", 0.0, 1.0, 0.0, true, false,
     true, nullptr, {}},
    {"output", "Output", "Output", 0.0, 1.0, 0.75, false, false, false, "dB", {}},
    {"oversample", "Oversampling", "Global", 0.0, 2.0, 2.0, true, false, false,
     nullptr, labels(kOversampleLabels)},
    {"gate_hold", "Gate Hold", "Dynamics", 0.0, 1.0, 0.0, false, false, false,
     "ms", {}},
    {"limiter", "Output Limiter", "Output", 0.0, 1.0, 1.0, true, false, true,
     nullptr, {}},
};

static_assert(sizeof(kParamSpecs) / sizeof(kParamSpecs[0]) == P_COUNT,
              "kParamSpecs must cover every parameter id");
static_assert(P_COUNT == 38, "parameter table and enum must stay in sync");

class Shared {
public:
  Shared() {
    for (uint32_t p = 0; p < P_COUNT; ++p) {
      values_[p].store(kParamSpecs[p].def, std::memory_order_relaxed);
      dirty_[p].store(false, std::memory_order_relaxed);
    }
  }

  double get(uint32_t p) const noexcept {
    return values_[p].load(std::memory_order_relaxed);
  }

  void set(uint32_t p, double v) noexcept {
    if (!std::isfinite(v)) {
      return;
    }
    const ParamSpec& s = kParamSpecs[p];
    v = std::min(std::max(v, s.min), s.max);
    if (s.stepped) {
      v = static_cast<double>(static_cast<int>(v + (v >= 0 ? 0.5 : -0.5)));
    }
    values_[p].store(v, std::memory_order_relaxed);
    dirty_[p].store(true, std::memory_order_release);
  }

  void setFromHost(uint32_t p, double v) noexcept {
    if (!std::isfinite(v)) {
      return;
    }
    const ParamSpec& s = kParamSpecs[p];
    v = std::min(std::max(v, s.min), s.max);
    if (s.stepped) {
      v = static_cast<double>(static_cast<int>(v + (v >= 0 ? 0.5 : -0.5)));
    }
    values_[p].store(v, std::memory_order_relaxed);
  }

  bool takeDirty(uint32_t p) noexcept {
    return dirty_[p].exchange(false, std::memory_order_acquire);
  }

  bool isDirty(uint32_t p) const noexcept {
    return dirty_[p].load(std::memory_order_acquire);
  }

  bool anyDirty() const noexcept {
    for (uint32_t p = 0; p < P_COUNT; ++p) {
      if (dirty_[p].load(std::memory_order_acquire)) {
        return true;
      }
    }
    return false;
  }

  std::atomic<float> grDb{0.0f};
  std::atomic<float> gateGrDb{0.0f};
  std::atomic<float> inPeakDb{-120.0f};
  std::atomic<float> outPeakDb{-120.0f};
  std::atomic<float> outVuDb{-120.0f};
  std::atomic<float> inPeakLDb{-120.0f};
  std::atomic<float> inPeakRDb{-120.0f};
  std::atomic<float> outPeakLDb{-120.0f};
  std::atomic<float> outPeakRDb{-120.0f};

private:
  std::atomic<double> values_[P_COUNT];
  std::atomic<bool> dirty_[P_COUNT];
};

inline Controls controlsFrom(const Shared& s) noexcept {
  Controls c;
  c.enabled = s.get(P_ENABLED) > 0.5;
  c.input = static_cast<float>(s.get(P_INPUT));
  c.polarity = s.get(P_POLARITY) > 0.5;
  c.colour = static_cast<int>(s.get(P_COLOR_MODEL) + 0.5);
  c.colourAmount = static_cast<float>(s.get(P_COLOR_AMOUNT));
  c.hpf = static_cast<float>(s.get(P_HPF));
  c.lpf = static_cast<float>(s.get(P_LPF));
  c.eqType = static_cast<int>(s.get(P_EQ_TYPE) + 0.5);
  c.eqIn = s.get(P_EQ_IN) > 0.5;
  c.lfGain = static_cast<float>(s.get(P_LF_GAIN));
  c.lfFreq = static_cast<float>(s.get(P_LF_FREQ));
  c.lfBell = s.get(P_LF_BELL) > 0.5;
  c.lmGain = static_cast<float>(s.get(P_LM_GAIN));
  c.lmFreq = static_cast<float>(s.get(P_LM_FREQ));
  c.lmQ = static_cast<float>(s.get(P_LM_Q));
  c.hmGain = static_cast<float>(s.get(P_HM_GAIN));
  c.hmFreq = static_cast<float>(s.get(P_HM_FREQ));
  c.hmQ = static_cast<float>(s.get(P_HM_Q));
  c.hfGain = static_cast<float>(s.get(P_HF_GAIN));
  c.hfFreq = static_cast<float>(s.get(P_HF_FREQ));
  c.hfBell = s.get(P_HF_BELL) > 0.5;
  c.compType = static_cast<int>(s.get(P_COMP_TYPE) + 0.5);
  c.compIn = s.get(P_COMP_IN) > 0.5;
  c.compThreshold = static_cast<float>(s.get(P_COMP_THRESHOLD));
  c.compMakeup = static_cast<float>(s.get(P_COMP_MAKEUP));
  c.compFast = s.get(P_COMP_FAST) > 0.5;
  c.compAuto = s.get(P_COMP_AUTO) > 0.5;
  c.gateIn = s.get(P_GATE_IN) > 0.5;
  c.gateThreshold = static_cast<float>(s.get(P_GATE_THRESHOLD));
  c.gateRange = static_cast<float>(s.get(P_GATE_RANGE));
  c.gateRelease = static_cast<float>(s.get(P_GATE_RELEASE));
  c.gateHold = static_cast<float>(s.get(P_GATE_HOLD));
  c.gateFast = s.get(P_GATE_FAST) > 0.5;
  c.gateMode = static_cast<int>(s.get(P_GATE_MODE) + 0.5);
  c.dynPost = s.get(P_DYN_POST) > 0.5;
  c.output = static_cast<float>(s.get(P_OUTPUT));
  c.oversample = static_cast<float>(s.get(P_OVERSAMPLE));
  c.limiterOn = s.get(P_LIMITER_ON) > 0.5;
  return c;
}

} // namespace suite
