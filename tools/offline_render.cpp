// Comp Suite — offline renderer for measurements.
//
// Reads a 24-bit stereo WAV, runs it through one model with the given
// controls, and writes a 24-bit stereo WAV.  Used by tools/measure.py so our
// models can be compared against reference plugins on identical signals.
//
// Usage: comp_offline_render in.wav out.wav model=0 input=0.5 threshold=0.5 ...
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "dsp/engine.hpp"

namespace {

// Raw interleaved stereo float32 files: no RIFF parsing here, so the Python
// harness owns all WAV handling and there is only one implementation of it.
struct Audio {
  std::vector<float> left;
  std::vector<float> right;
};

bool readRaw(const char* path, Audio& out) {
  FILE* f = std::fopen(path, "rb");
  if (!f) {
    return false;
  }
  std::fseek(f, 0, SEEK_END);
  const long bytes = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  const size_t frames = static_cast<size_t>(bytes) / (2 * sizeof(float));
  std::vector<float> interleaved(frames * 2);
  if (std::fread(interleaved.data(), sizeof(float), interleaved.size(), f) !=
      interleaved.size()) {
    std::fclose(f);
    return false;
  }
  std::fclose(f);
  out.left.resize(frames);
  out.right.resize(frames);
  for (size_t i = 0; i < frames; ++i) {
    out.left[i] = interleaved[2 * i];
    out.right[i] = interleaved[2 * i + 1];
  }
  return true;
}

bool writeRaw(const char* path, const Audio& a) {
  FILE* f = std::fopen(path, "wb");
  if (!f) {
    return false;
  }
  for (size_t i = 0; i < a.left.size(); ++i) {
    const float frame[2] = {a.left[i], a.right[i]};
    std::fwrite(frame, sizeof(float), 2, f);
  }
  std::fclose(f);
  return true;
}

comp::Controls defaults() {
  comp::Controls c;
  c.model = 0;
  c.enabled = true;
  c.input = 0.25f;
  c.threshold = 0.5f;
  c.ratio = 0.5f;
  c.attack = 0.5f;
  c.release = 0.5f;
  c.makeup = 0.5f;
  c.output = 0.5f;
  c.mix = 1.0f;
  c.sidechainHpf = 0.0f;
  c.distortion = 0.0f;
  return c;
}

} // namespace

int main(int argc, char** argv) {
  if (argc < 4) {
    std::fprintf(stderr, "usage: %s in.f32 out.f32 sampleRate [key=value ...]\n", argv[0]);
    return 2;
  }
  Audio in;
  if (!readRaw(argv[1], in)) {
    std::fprintf(stderr, "cannot read %s\n", argv[1]);
    return 1;
  }
  const float sampleRate = std::strtof(argv[3], nullptr);
  comp::Controls c = defaults();
  for (int i = 4; i < argc; ++i) {
    const char* eq = std::strchr(argv[i], '=');
    if (!eq) {
      continue;
    }
    const std::string key(argv[i], static_cast<size_t>(eq - argv[i]));
    const float v = std::strtof(eq + 1, nullptr);
    if (key == "model") c.model = static_cast<int>(v);
    else if (key == "input") c.input = v;
    else if (key == "threshold") c.threshold = v;
    else if (key == "ratio") c.ratio = v;
    else if (key == "attack") c.attack = v;
    else if (key == "release") c.release = v;
    else if (key == "makeup") c.makeup = v;
    else if (key == "output") c.output = v;
    else if (key == "mix") c.mix = v;
    else if (key == "sc") c.sidechainHpf = v;
    else if (key == "distortion") c.distortion = v;
    else if (key == "limiter") c.limiterOn = v > 0.5f;
    else if (key == "limitthresh") c.limitThreshold = v;
    else if (key == "enabled") c.enabled = v > 0.5f;
  }

  comp::Engine engine;
  engine.prepare(sampleRate);
  engine.setControls(c);

  Audio out = in;
  comp::Meters meters;
  const int frames = static_cast<int>(out.left.size());
  for (int start = 0; start < frames; start += 256) {
    const int len = std::min(256, frames - start);
    engine.process(out.left.data() + start, out.right.data() + start, len, meters);
  }
  if (!writeRaw(argv[2], out)) {
    return 1;
  }
  return 0;
}
