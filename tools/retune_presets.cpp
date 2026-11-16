// Comp Suite — preset retune tool
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Prints, for every factory preset, the current gain reduction on a reference
// signal and the drive value that would hit the target for its category.
//
// After a DSP change that alters how a control maps to reduction (the feedback
// pass did exactly that), presets drift.  This measures instead of guessing.
//
//   comp_retune_presets                 table of suggestions
//   comp_retune_presets --targets FILE  use per-preset targets ("name<TAB>gr")
//
// The targets file is how the 0.3.0 retune preserved the intent of presets
// that were tuned before the feedback pass: measure each preset's reduction on
// the previous release, then solve for the drive that reproduces it here.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "dsp/engine.hpp"
#include "params.hpp"
#include "presets.hpp"

namespace {

constexpr float kSampleRate = 48000.0f;
constexpr float kReferenceHz = 500.0f;
constexpr float kReferenceDbfs = -12.0f;

// Target gain reduction on the reference signal, by preset category.
float targetGr(const char* category) {
  if (category == nullptr) return 4.0f;
  if (!std::strcmp(category, "Vocals")) return 5.0f;
  if (!std::strcmp(category, "Drums")) return 4.0f;
  if (!std::strcmp(category, "Bass")) return 4.0f;
  if (!std::strcmp(category, "Guitars")) return 3.0f;
  if (!std::strcmp(category, "Parallel")) return 7.0f;
  if (!std::strcmp(category, "Mix Bus")) return 3.0f;
  return 4.0f;
}

// Which control sets the drive for a model, and which way it moves.
// Returns P_COUNT when the model has no usable drive control.
comp::ParamId driveParam(int model, bool& increasesWithValue) {
  switch (model) {
    case 0:
    case 1:
    case 4:
      increasesWithValue = true;
      return comp::P_INPUT;
    case 2:
    case 3:
      increasesWithValue = true;  // peak reduction
      return comp::P_THRESHOLD;
    case 5:
      increasesWithValue = false;  // VCA threshold: lower dB is more reduction
      return comp::P_THRESHOLD;
    case 6:
    case 7:
      increasesWithValue = false;  // Clean/Molot threshold, same direction
      return comp::P_THRESHOLD;
    default:
      return comp::P_COUNT;
  }
}

comp::Controls buildControls(const comp::Preset& preset, float driveValue) {
  comp::Shared shared;
  shared.set(comp::P_MODEL, static_cast<double>(preset.model));
  for (const comp::PresetValue& value : preset.values) {
    if (value.param >= 0 && value.param < static_cast<int>(comp::P_COUNT)) {
      shared.set(static_cast<uint32_t>(value.param),
                 static_cast<double>(value.value));
    }
  }
  bool increases = true;
  const comp::ParamId drive = driveParam(preset.model, increases);
  if (drive != comp::P_COUNT) {
    shared.set(drive, static_cast<double>(driveValue));
  }
  return comp::controlsFrom(shared);
}

float measureGr(const comp::Preset& preset, float driveValue) {
  comp::Engine engine;
  engine.prepare(kSampleRate);
  engine.setControls(buildControls(preset, driveValue));

  comp::Meters meters;
  std::vector<float> left(256), right(256);
  const float amp = std::pow(10.0f, kReferenceDbfs / 20.0f);
  const int blocks = static_cast<int>(6.0f * kSampleRate / 256);
  const int settle = static_cast<int>(4.0f * kSampleRate / 256);
  float peak = 0.0f;
  for (int b = 0; b < blocks; ++b) {
    for (int i = 0; i < 256; ++i) {
      const float t = static_cast<float>(b * 256 + i) / kSampleRate;
      const float s = amp * std::sin(2.0f * 3.14159265f * kReferenceHz * t);
      left[i] = s;
      right[i] = s;
    }
    engine.process(left.data(), right.data(), 256, meters);
    if (b >= settle) {
      peak = std::max(peak, meters.grDb);
    }
  }
  return peak;
}

// Binary search on the drive control for the target reduction.
float solveDrive(const comp::Preset& preset, float target) {
  bool increases = true;
  driveParam(preset.model, increases);
  float lo = 0.0f;
  float hi = 1.0f;
  for (int i = 0; i < 24; ++i) {
    const float mid = 0.5f * (lo + hi);
    const float gr = measureGr(preset, mid);
    const bool more = increases ? (gr < target) : (gr > target);
    if (more) {
      lo = mid;  // need more reduction
    } else {
      hi = mid;
    }
  }
  return 0.5f * (lo + hi);
}

} // namespace

int main(int argc, char** argv) {
  std::map<std::string, float> targets;
  for (int i = 1; i < argc; ++i) {
    if (!std::strcmp(argv[i], "--targets") && i + 1 < argc) {
      std::ifstream in(argv[++i]);
      std::string name;
      float gr = 0.0f;
      while (std::getline(in, name, '\t') && (in >> gr)) {
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        targets[name] = gr;
      }
    }
  }

  std::printf("%-24s %-10s %6s %6s %6s %6s\n", "preset", "category", "old",
              "GRold", "new", "GRnew");
  for (int i = 0; i < comp::kPresetCount; ++i) {
    const comp::Preset& preset = comp::kPresets[i];
    bool increases = true;
    const comp::ParamId drive = driveParam(preset.model, increases);
    if (drive == comp::P_COUNT) {
      continue;
    }
    // Current drive value, or the parameter default when the preset omits it.
    float current = static_cast<float>(comp::kParamSpecs[drive].def);
    for (const comp::PresetValue& value : preset.values) {
      if (value.param == static_cast<int>(drive)) {
        current = value.value;
      }
    }
    const auto targetIt = targets.find(preset.name);
    const float target = targetIt != targets.end() ? targetIt->second
                                                   : targetGr(preset.category);
    const float oldGr = measureGr(preset, current);
    const float solved = solveDrive(preset, target);
    const float newGr = measureGr(preset, solved);
    char name[32];
    std::snprintf(name, sizeof(name), "%s", preset.name);
    std::printf("%-24s %-10s %6.3f %6.2f %6.3f %6.2f\n", name,
                preset.category ? preset.category : "", current, oldGr, solved,
                newGr);
  }
  return 0;
}
