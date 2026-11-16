// Suite Channel — panel smoke test
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cairo.h>

#include <cassert>
#include <cstdio>

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

  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  std::printf("suite_panel: ok (%zu widgets)\n", rects.size());
  return 0;
}
