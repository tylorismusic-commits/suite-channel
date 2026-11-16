// Suite Channel — gate.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Stereo-linked SSL-style expander/gate.  Gate mode uses variable hysteresis;
// expander mode is a fixed 1:2 downward expansion.  Range sets the attenuation
// floor, so the section can be used as a gentle expander or a hard gate.
#pragma once

#include <algorithm>
#include <cmath>

#include <hdsp/hdsp.hpp>

#include "controls.hpp"

namespace suite {

class GateExpander {
public:
  void prepare(float sampleRate) noexcept {
    sr_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
    attackSlow_ = hdsp::timeCoeff(1.5f, sr_);
    attackFast_ = hdsp::timeCoeff(0.1f, sr_);
    reset();
    setControls(controls_);
  }

  void setControls(const Controls& c) noexcept {
    controls_ = c;
    enabled_ = c.gateIn;
    mode_ = static_cast<GateMode>(std::min(std::max(c.gateMode, 0), 1));
    openDb_ = gateThresholdDb(c.gateThreshold);
    rangeDb_ = gateRangeDb(c.gateRange);
    releaseCoeff_ = hdsp::timeCoeff(gateReleaseMs(c.gateRelease), sr_);
    attackCoeff_ = c.gateFast ? attackFast_ : attackSlow_;
    // Variable hysteresis: more as the threshold is lowered.
    hysteresisDb_ = 2.0f + 6.0f * (1.0f - clamp01(c.gateThreshold));
    closeDb_ = openDb_ - hysteresisDb_;
  }

  // Processes one stereo frame in place. Returns gate gain reduction in dB.
  float process(float& left, float& right) noexcept {
    if (!enabled_ || rangeDb_ <= 0.01f) {
      grDb_ = 0.0f;
      return 0.0f;
    }

    const float level = std::max(std::fabs(left), std::fabs(right));
    const float levelDb = hdsp::gainToDb(level + 1.0e-12f);

    float targetGr = 0.0f;
    if (mode_ == GateMode::Gate) {
      if (levelDb >= openDb_) {
        open_ = true;
      } else if (levelDb <= closeDb_) {
        open_ = false;
      }
      targetGr = open_ ? 0.0f : rangeDb_;
    } else {
      // Fixed 1:2 downward expansion, capped by Range.
      targetGr = std::min(std::max(0.5f * (openDb_ - levelDb), 0.0f), rangeDb_);
    }

    const float coeff = targetGr < grDb_ ? attackCoeff_ : releaseCoeff_;
    grDb_ = coeff * grDb_ + (1.0f - coeff) * targetGr;
    if (grDb_ < 1.0e-5f) {
      grDb_ = 0.0f;
    }

    const float gain = hdsp::dbToGain(-grDb_);
    left *= gain;
    right *= gain;
    return grDb_;
  }

  void reset() noexcept {
    grDb_ = 0.0f;
    open_ = false;
  }

private:
  float sr_ = 48000.0f;
  Controls controls_;
  bool enabled_ = false;
  GateMode mode_ = GateMode::Gate;
  bool open_ = false;
  float openDb_ = -20.0f;
  float closeDb_ = -26.0f;
  float hysteresisDb_ = 6.0f;
  float rangeDb_ = 24.0f;
  float attackSlow_ = 0.0f;
  float attackFast_ = 0.0f;
  float attackCoeff_ = 0.0f;
  float releaseCoeff_ = 0.0f;
  float grDb_ = 0.0f;
};

} // namespace suite
