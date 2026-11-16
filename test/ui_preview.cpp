// Suite Channel — UI preview renderer
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cairo.h>

#include <cstdio>

#include "ui/panel.hpp"

namespace {

void render(const char* path, double scale) {
  const int w = static_cast<int>(suite::kPanelWidth * scale);
  const int h = static_cast<int>(suite::kPanelHeight * scale);
  cairo_surface_t* surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
  cairo_t* cr = cairo_create(surface);
  cairo_scale(cr, scale, scale);

  suite::Shared shared;
  shared.grDb.store(8.0f);
  shared.gateGrDb.store(4.0f);
  shared.outPeakLDb.store(-6.0f);
  shared.outPeakRDb.store(-9.0f);
  shared.inPeakLDb.store(-12.0f);
  shared.inPeakRDb.store(-14.0f);
  suite::PanelState state;
  suite::Panel panel;
  panel.tick(shared, state);
  panel.draw(cr, shared, state);

  cairo_destroy(cr);
  cairo_surface_write_to_png(surface, path);
  cairo_surface_destroy(surface);
  std::printf("wrote %s\n", path);
}

} // namespace

int main() {
  render("suite-channel.png", 1.0);
  render("suite-channel@2x.png", 2.0);
  return 0;
}
