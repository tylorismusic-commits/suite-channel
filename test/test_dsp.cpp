// Suite Channel — DSP smoke and static-curve tests
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

#include "dsp/engine.hpp"
#include "params.hpp"

namespace {

constexpr float kPi = 3.14159265f;

bool finite(const std::vector<float>& v) {
  for (float x : v) {
    if (!std::isfinite(x)) {
      return false;
    }
  }
  return true;
}

void fillSine(std::vector<float>& l, std::vector<float>& r, float dbfs,
              float hz, int start) {
  const float amp = std::pow(10.0f, dbfs / 20.0f);
  for (size_t i = 0; i < l.size(); ++i) {
    const float s = amp * std::sin(2.0f * kPi * hz * (start + i) / 48000.0f);
    l[i] = s;
    r[i] = s;
  }
}

void fillConst(std::vector<float>& l, std::vector<float>& r, float dbfs) {
  const float amp = std::pow(10.0f, dbfs / 20.0f);
  for (size_t i = 0; i < l.size(); ++i) {
    l[i] = amp;
    r[i] = amp;
  }
}

} // namespace

int main() {
  suite::Shared shared;
  suite::Engine engine;
  suite::Meters meters;
  engine.prepare(48000.0f, 2.0f);
  engine.setControls(suite::controlsFrom(shared));

  std::vector<float> l(512);
  std::vector<float> r(512);

  // --- VCA static curve: 3.5:1 at threshold -12 dB -------------------------
  shared.set(suite::P_EQ_IN, 0.0);     // keep the EQ out of the measurement
  shared.set(suite::P_COLOR_AMOUNT, 0.0);
  shared.set(suite::P_COMP_IN, 1.0);
  shared.set(suite::P_COMP_AUTO, 0.0);
  shared.set(suite::P_COMP_MAKEUP, 0.5);
  shared.set(suite::P_COMP_THRESHOLD, (10.0 + 12.0) / 30.0);  // -12 dB
  engine.setControls(suite::controlsFrom(shared));
  engine.prepare(48000.0f, 2.0f);

  for (int block = 0; block < 100; ++block) {
    fillConst(l, r, -6.0f);
    engine.process(l.data(), r.data(), 512, meters);
    assert(finite(l));
    assert(finite(r));
  }
  // 6 dB over threshold at 3.5:1 = 6 * (1 - 1/3.5) = 4.29 dB.
  const float expected = 6.0f * (1.0f - 1.0f / 3.5f);
  std::printf("VCA GR at -6 dBFS / -12 dB thr: %.2f dB (expected %.2f)\n",
              meters.grDb, expected);
  assert(std::fabs(meters.grDb - expected) < 0.8f);

  // --- 76Blue stays finite and compresses ----------------------------------
  shared.set(suite::P_COMP_TYPE, 1.0);
  engine.setControls(suite::controlsFrom(shared));
  for (int block = 0; block < 20; ++block) {
    fillSine(l, r, -6.0f, 1000.0f, block * 512);
    engine.process(l.data(), r.data(), 512, meters);
    assert(finite(l));
    assert(finite(r));
  }
  std::printf("76Blue GR at -6 dBFS: %.2f dB\n", meters.grDb);
  assert(meters.grDb > 0.5f);

  // --- Gate: closed on a quiet signal, open on a loud one ------------------
  shared.set(suite::P_COMP_IN, 0.0);
  shared.set(suite::P_GATE_IN, 1.0);
  shared.set(suite::P_GATE_MODE, 1.0);
  shared.set(suite::P_GATE_THRESHOLD, (-20.0 + 30.0) / 40.0);  // -20 dB
  shared.set(suite::P_GATE_RANGE, 1.0);                        // 40 dB
  shared.set(suite::P_GATE_RELEASE, 0.3);
  engine.setControls(suite::controlsFrom(shared));

  for (int block = 0; block < 100; ++block) {
    fillSine(l, r, -45.0f, 1000.0f, block * 512);
    engine.process(l.data(), r.data(), 512, meters);
  }
  std::printf("gate GR on -45 dBFS: %.2f dB\n", meters.gateGrDb);
  assert(meters.gateGrDb > 30.0f);

  for (int block = 0; block < 20; ++block) {
    fillSine(l, r, -6.0f, 1000.0f, block * 512);
    engine.process(l.data(), r.data(), 512, meters);
  }
  std::printf("gate GR on -6 dBFS: %.2f dB\n", meters.gateGrDb);
  assert(meters.gateGrDb < 1.0f);

  // --- EQ IN is a constant-latency crossfade -------------------------------
  const int latencyBefore = engine.latencySamples();
  shared.set(suite::P_EQ_IN, 1.0);
  engine.setControls(suite::controlsFrom(shared));
  for (int block = 0; block < 8; ++block) {
    fillSine(l, r, -20.0f, 1000.0f, block * 512);
    engine.process(l.data(), r.data(), 512, meters);
    assert(finite(l));
    assert(finite(r));
  }
  assert(engine.latencySamples() == latencyBefore);

  // --- Colour amount 0 is dry; amount 1 stays finite -----------------------
  shared.set(suite::P_GATE_IN, 0.0);
  shared.set(suite::P_COLOR_AMOUNT, 0.0);
  engine.setControls(suite::controlsFrom(shared));
  for (int block = 0; block < 4; ++block) {
    fillSine(l, r, -20.0f, 1000.0f, block * 512);
    engine.process(l.data(), r.data(), 512, meters);
    assert(finite(l));
    assert(finite(r));
  }
  shared.set(suite::P_COLOR_AMOUNT, 1.0);
  engine.setControls(suite::controlsFrom(shared));
  for (int block = 0; block < 20; ++block) {
    fillSine(l, r, -20.0f, 1000.0f, block * 512);
    engine.process(l.data(), r.data(), 512, meters);
    assert(finite(l));
    assert(finite(r));
  }

  assert(engine.latencySamples() >= 0);
  std::printf("suite_dsp: ok (latency %d samples)\n", engine.latencySamples());
  return 0;
}
