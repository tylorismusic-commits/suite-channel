// Suite Channel — panel smoke test
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cairo.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdio>

#include "presets.hpp"
#include "ui/panel.hpp"

int main() {
  cairo_surface_t* surface = cairo_image_surface_create(
      CAIRO_FORMAT_ARGB32, suite::kPanelWidth, suite::kPanelHeight);
  cairo_t* cr = cairo_create(surface);

  suite::Shared shared;
  suite::PanelState state;
  suite::Panel panel;
  panel.tick(shared, state);
  panel.draw(cr, shared, state);

  const auto rects = panel.widgetRects();
  assert(rects.size() > 20);
  for (const auto& r : rects) {
    assert(r.x + r.w >= 0.0);
    assert(r.y + r.h >= 0.0);
  }

  // The LF gain knob is at (330, 664); its cap centre reports the fill
  // colour.  Brown is tan; Black must be dark, and switching back must
  // restore the tan fill.
  auto capRed = [&]() {
    cairo_surface_flush(surface);
    unsigned char* data = cairo_image_surface_get_data(surface);
    const int stride = cairo_image_surface_get_stride(surface);
    return data[664 * stride + 330 * 4 + 2];
  };
  const int brownRed = capRed();
  assert(brownRed > 120);

  assert(panel.pointerDown(shared, state, 415, 799, false));
  assert(shared.get(suite::P_EQ_TYPE) == 1.0);
  panel.pointerUp();
  panel.draw(cr, shared, state);
  const int blackRed = capRed();
  assert(blackRed < brownRed - 40);

  assert(panel.pointerDown(shared, state, 347, 799, false));
  assert(shared.get(suite::P_EQ_TYPE) == 0.0);
  panel.pointerUp();
  panel.draw(cr, shared, state);
  const int brownAgain = capRed();
  assert(brownAgain > blackRed + 40);

  // 76Blue must paint a silver compressor block, and VCA must restore dark.
  auto redAt = [&](int x, int y) {
    cairo_surface_flush(surface);
    unsigned char* data = cairo_image_surface_get_data(surface);
    const int stride = cairo_image_surface_get_stride(surface);
    return data[y * stride + x * 4 + 2];
  };
  const int vcaBlock = redAt(40, 290);
  assert(vcaBlock < 100);
  assert(panel.pointerDown(shared, state, 190, 297, false));
  assert(shared.get(suite::P_COMP_TYPE) == 1.0);
  panel.pointerUp();
  panel.draw(cr, shared, state);
  const int silverBlock = redAt(40, 290);
  assert(silverBlock > 150);
  assert(panel.pointerDown(shared, state, 140, 297, false));
  assert(shared.get(suite::P_COMP_TYPE) == 0.0);
  panel.pointerUp();
  panel.draw(cr, shared, state);
  const int vcaAgain = redAt(40, 290);
  assert(vcaAgain < 100);

  // A/B compare: A captures the first state, B the second, switching restores.
  {
    shared.set(suite::P_OUTPUT, 0.9);
    assert(panel.pointerDown(shared, state, 345, 31, false));  // A
    panel.pointerUp();
    assert(panel.pointerDown(shared, state, 375, 31, false));  // B
    panel.pointerUp();
    shared.set(suite::P_OUTPUT, 0.5);
    assert(std::fabs(shared.get(suite::P_OUTPUT) - 0.5) < 1.0e-9);
    assert(panel.pointerDown(shared, state, 345, 31, false));  // back to A
    panel.pointerUp();
    assert(std::fabs(shared.get(suite::P_OUTPUT) - 0.9) < 1.0e-9);
    assert(panel.pointerDown(shared, state, 375, 31, false));  // back to B
    panel.pointerUp();
    assert(std::fabs(shared.get(suite::P_OUTPUT) - 0.5) < 1.0e-9);
  }

  // COPY captures the live state; PASTE applies it to the other slot.
  {
    assert(panel.pointerDown(shared, state, 345, 31, false));  // A = 0.9
    panel.pointerUp();
    assert(panel.pointerDown(shared, state, 409, 31, false));  // COPY
    panel.pointerUp();
    assert(panel.pointerDown(shared, state, 375, 31, false));  // B = 0.5
    panel.pointerUp();
    assert(std::fabs(shared.get(suite::P_OUTPUT) - 0.5) < 1.0e-9);
    assert(panel.pointerDown(shared, state, 451, 31, false));  // PASTE
    panel.pointerUp();
    assert(std::fabs(shared.get(suite::P_OUTPUT) - 0.9) < 1.0e-9);
  }

  // Typed value entry accepts real units and normalizes them.
  {
    shared.set(suite::P_INPUT, 0.5);
    panel.beginTextEntry(shared, state, 85, 190);
    for (const char* c = "6.0 dB"; *c != '\0'; ++c) {
      assert(panel.textKey(shared, state, 0, *c));
    }
    assert(panel.textKey(shared, state, suite::Panel::kTextEnter, 0));
    const double expected = (6.0 + 18.0) / 36.0;
    assert(std::fabs(shared.get(suite::P_INPUT) - expected) < 1.0e-6);

    // Clicking inside the field keeps the edit alive, then Enter applies.
    panel.beginTextEntry(shared, state, 85, 190);
    assert(panel.pointerDown(shared, state, 110, 158, false));
    panel.pointerUp();
    panel.textKey(shared, state, 0, '3');
    assert(panel.textKey(shared, state, suite::Panel::kTextEnter, 0));
    assert(std::fabs(shared.get(suite::P_INPUT) - (3.0 + 18.0) / 36.0) <
           1.0e-6);

    // Escape abandons an edit.
    panel.beginTextEntry(shared, state, 85, 190);
    panel.textKey(shared, state, 0, '9');
    assert(panel.textKey(shared, state, suite::Panel::kTextEscape, 0));
    assert(std::fabs(shared.get(suite::P_INPUT) - (3.0 + 18.0) / 36.0) <
           1.0e-6);
  }

  // Double-click resets a knob to its default.
  {
    shared.set(suite::P_INPUT, 0.9);
    panel.pointerDown(shared, state, 85, 190, false);
    panel.pointerUp();
    panel.pointerDown(shared, state, 85, 190, false);
    panel.pointerUp();
    assert(std::fabs(shared.get(suite::P_INPUT) - 0.5) < 1.0e-9);
  }

  // The Hold pill cycles through the fixed hold times.
  {
    shared.set(suite::P_GATE_HOLD, 0.0);
    const double expected[] = {0.1, 0.2, 0.4, 0.8, 1.0, 0.0};
    for (double v : expected) {
      assert(panel.pointerDown(shared, state, 166, 671, false));
      panel.pointerUp();
      assert(std::fabs(shared.get(suite::P_GATE_HOLD) - v) < 1.0e-9);
    }
  }

  // Every factory preset must recall a coherent state and draw.
  for (int i = 0; i < suite::kPresetCount; ++i) {
    panel.applyPreset(shared, state, i);
    const int ct = static_cast<int>(shared.get(suite::P_COMP_TYPE) + 0.5);
    const int cm = static_cast<int>(shared.get(suite::P_COLOR_MODEL) + 0.5);
    const int et = static_cast<int>(shared.get(suite::P_EQ_TYPE) + 0.5);
    const int gm = static_cast<int>(shared.get(suite::P_GATE_MODE) + 0.5);
    assert(ct >= 0 && ct <= 1);
    assert(cm >= 0 && cm <= 4);
    assert(et >= 0 && et <= 1);
    assert(gm >= 0 && gm <= 2);
    panel.draw(cr, shared, state);
  }

  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  std::printf("suite_panel: ok (%zu widgets, LF fill %d/%d/%d)\n",
              rects.size(), brownRed, blackRed, brownAgain);
  return 0;
}
