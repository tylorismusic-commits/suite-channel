// Suite Channel — panel.cpp
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/panel.hpp"

#include "presets.hpp"

#include <cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace suite {

namespace {

struct Rgb {
  double r, g, b;
};

// One neutral Comp Suite faceplate, with the selected Patina colour used as an
// accent for the tabs and a few indicators.
struct Skin {
  Rgb bodyTop, bodyBottom;
  Rgb legend, legendDim;
  Rgb headerBg;
  Rgb tabOff, tabOn, tabTextOff, tabTextOn;
  Rgb knobCap, knobCapEdge, knobSkirt, knobIndicator;
  Rgb btnOff, btnOn, btnText, btnTextOn;
  Rgb screw;
};

Rgb colourAccent(int colour) {
  switch (colour) {
    case 0: return {0.90, 0.55, 0.22};  // Valve
    case 1: return {0.30, 0.76, 0.66};  // Console
    case 2: return {0.40, 0.62, 0.92};  // Bus
    case 3: return {0.72, 0.48, 0.88};  // Tape
    default: return {0.38, 0.66, 0.54}; // Transformer
  }
}

Skin skinFor(int colour) {
  Skin s;
  s.bodyTop = {0.20, 0.22, 0.26};
  s.bodyBottom = {0.10, 0.11, 0.14};
  s.legend = {0.84, 0.87, 0.91};
  s.legendDim = {0.48, 0.52, 0.58};
  s.headerBg = {0.09, 0.10, 0.12};
  s.tabOff = {0.16, 0.18, 0.21};
  s.tabOn = colourAccent(colour);
  s.tabTextOff = {0.62, 0.66, 0.71};
  s.tabTextOn = {1.0, 1.0, 1.0};
  s.knobCap = {0.20, 0.52, 0.82};
  s.knobCapEdge = {0.07, 0.27, 0.46};
  s.knobSkirt = {0.10, 0.12, 0.14};
  s.knobIndicator = {0.92, 0.96, 1.0};
  s.btnOff = {0.15, 0.17, 0.20};
  s.btnOn = colourAccent(colour);
  s.btnText = {0.72, 0.76, 0.80};
  s.btnTextOn = {1.0, 1.0, 1.0};
  s.screw = {0.38, 0.40, 0.44};
  return s;
}

Rgb bandAccent(uint32_t param) {
  if (param == P_HF_GAIN || param == P_HF_FREQ || param == P_HF_BELL) {
    return {0.90, 0.36, 0.30};  // HF red
  }
  if (param == P_HM_GAIN || param == P_HM_FREQ || param == P_HM_Q) {
    return {0.35, 0.78, 0.42};  // HMF green
  }
  if (param == P_LM_GAIN || param == P_LM_FREQ || param == P_LM_Q) {
    return {0.36, 0.62, 0.92};  // LMF blue
  }
  if (param == P_LF_GAIN || param == P_LF_FREQ || param == P_LF_BELL) {
    return {0.86, 0.58, 0.28};  // LF amber
  }
  return {0.92, 0.96, 1.0};
}

const char* const kScaleInput[] = {"-18", "0", "+18"};
const char* const kScaleAmount[] = {"0", "", "10"};
const char* const kScaleHPF[] = {"OUT", "40", "90", "190", "350"};
const char* const kScaleLPF[] = {"OUT", "3k", "6k", "10k", "15.2k"};
const char* const kScaleGain[] = {"-15", "0", "+15"};
const char* const kScaleFreqLF[] = {"30", "100", "200", "450"};
const char* const kScaleFreqLM[] = {"200", "500", "1k", "2.5k"};
const char* const kScaleFreqHM[] = {"600", "1.5k", "3k", "7k"};
const char* const kScaleFreqHF[] = {"1.5k", "4k", "8k", "16k"};
const char* const kScaleQ[] = {"0.5", "1", "2", "3"};
const char* const kScaleCompThresh[] = {"+10", "0", "-10", "-20"};
const char* const kScaleMakeup[] = {"-9", "0", "+9"};
const char* const kScaleGateThresh[] = {"-30", "-15", "0", "+10"};
const char* const kScaleRange[] = {"0", "10", "20", "30", "40"};
const char* const kScaleRelease[] = {"0.1", "0.3", "1", "2", "4"};
const char* const kScaleOutput[] = {"-18", "0", "+18"};

template <size_t N>
std::vector<const char*> sc(const char* const (&labels)[N]) {
  return std::vector<const char*>(labels, labels + N);
}

constexpr double kPi = 3.14159265358979323846;

constexpr double kPresetX = 64.0;
constexpr double kPresetY = 14.0;
constexpr double kPresetW = 240.0;
constexpr double kPresetH = 32.0;
constexpr double kListY = 50.0;
constexpr double kListRow = 20.0;
constexpr double kListVisible = 16.0;

double knobAngle(double f) {
  return (135.0 + 270.0 * f) * kPi / 180.0;
}

double clamp01(double v) {
  return std::min(std::max(v, 0.0), 1.0);
}

void roundRect(cairo_t* cr, double x, double y, double w, double h, double r) {
  cairo_new_sub_path(cr);
  cairo_arc(cr, x + w - r, y + r, r, -kPi / 2, 0);
  cairo_arc(cr, x + w - r, y + h - r, r, 0, kPi / 2);
  cairo_arc(cr, x + r, y + h - r, r, kPi / 2, kPi);
  cairo_arc(cr, x + r, y + r, r, kPi, 3 * kPi / 2);
  cairo_close_path(cr);
}

void setRgb(cairo_t* cr, Rgb c, double a = 1.0) {
  cairo_set_source_rgba(cr, c.r, c.g, c.b, a);
}

Rgb mix(Rgb a, Rgb b, double t) {
  return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t,
          a.b + (b.b - a.b) * t};
}

const std::vector<int>& presetOrder() {
  static const std::vector<int> order = [] {
    std::vector<int> v;
    for (int i = 0; i < kPresetCount; ++i) {
      v.push_back(i);
    }
    std::stable_sort(v.begin(), v.end(), [](int a, int b) {
      const int c = std::strcmp(kPresets[a].category, kPresets[b].category);
      if (c != 0) {
        return c < 0;
      }
      return std::strcmp(kPresets[a].name, kPresets[b].name) < 0;
    });
    return v;
  }();
  return order;
}

int currentColour(const Shared& shared) {
  return std::clamp(static_cast<int>(shared.get(P_COLOR_MODEL) + 0.5), 0, 4);
}

} // namespace

Panel::Panel() {
  grain_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 64, 64);
  unsigned char* data = cairo_image_surface_get_data(grain_);
  const int stride = cairo_image_surface_get_stride(grain_);
  unsigned int seed = 0x5u;
  for (int y = 0; y < 64; ++y) {
    for (int x = 0; x < 64; ++x) {
      seed = seed * 1103515245u + 12345u;
      const unsigned v = (seed >> 16) & 0xff;
      const unsigned a = (v < 128) ? 16u : 0u;
      const unsigned c = (v < 128) ? 0u : 255u;
      unsigned char* px = data + y * stride + x * 4;
      px[0] = static_cast<unsigned char>(c & 0xff);
      px[1] = static_cast<unsigned char>(c & 0xff);
      px[2] = static_cast<unsigned char>(c & 0xff);
      px[3] = static_cast<unsigned char>(a);
    }
  }
  cairo_surface_mark_dirty(grain_);
  fallbackFont_ = cairo_toy_font_face_create(
      "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  font_ = fallbackFont_;
}

Panel::~Panel() {
  if (grain_) {
    cairo_surface_destroy(grain_);
  }
  if (font_ && font_ != fallbackFont_) {
    cairo_font_face_destroy(font_);
  }
  if (fallbackFont_) {
    cairo_font_face_destroy(fallbackFont_);
  }
  if (ftFace_) {
    FT_Done_Face(static_cast<FT_Face>(ftFace_));
  }
  if (ft_) {
    FT_Done_FreeType(static_cast<FT_Library>(ft_));
  }
}

void Panel::loadFont(const std::string& dir) {
  const std::string path = dir + "/Jost-ExtraBold.ttf";
  FT_Library lib = nullptr;
  if (FT_Init_FreeType(&lib) != 0) {
    return;
  }
  FT_Face face = nullptr;
  if (FT_New_Face(lib, path.c_str(), 0, &face) != 0) {
    FT_Done_FreeType(lib);
    return;
  }
  cairo_font_face_t* cf = cairo_ft_font_face_create_for_ft_face(face, 0);
  if (font_ && font_ != fallbackFont_) {
    cairo_font_face_destroy(font_);
  }
  ft_ = lib;
  ftFace_ = face;
  font_ = cf;
}

void Panel::text(cairo_t* cr, const char* s, double x, double y, double size,
                 int align, double r, double g, double b) const {
  cairo_set_font_face(cr, font_);
  cairo_set_font_size(cr, size);
  cairo_text_extents_t ext;
  cairo_text_extents(cr, s, &ext);
  double tx = x;
  if (align == 1) {
    tx -= (ext.width * 0.5 + ext.x_bearing);
  } else if (align == 2) {
    tx -= (ext.width + ext.x_bearing);
  }
  cairo_set_source_rgb(cr, r, g, b);
  cairo_move_to(cr, tx, y);
  cairo_show_text(cr, s);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void Panel::layout(const Shared& shared, const PanelState& state) {
  (void)state;
  widgets_.clear();
  const int colour = currentColour(shared);
  const int compType = std::clamp(static_cast<int>(shared.get(P_COMP_TYPE) + 0.5), 0, 1);

  auto knob = [&](uint32_t param, double x, double y, double r,
                  const char* label, std::vector<const char*> scale,
                  bool below = false) {
    Widget w;
    w.kind = Kind::Knob;
    w.param = param;
    w.x = x;
    w.y = y;
    w.r = r;
    w.label = label;
    w.scale = std::move(scale);
    w.labelBelow = below;
    widgets_.push_back(w);
  };

  auto pill = [&](uint32_t param, double x, double y, double w, double h,
                  const char* label) {
    Widget b;
    b.kind = Kind::Pill;
    b.param = param;
    b.x = x;
    b.y = y;
    b.w = w;
    b.h = h;
    b.label = label;
    widgets_.push_back(b);
  };

  auto toggle = [&](uint32_t param, double x, double y, double value,
                    const char* top, const char* bottom) {
    Widget t;
    t.kind = Kind::Toggle;
    t.param = param;
    t.x = x;
    t.y = y;
    t.w = 30;
    t.h = 44;
    t.value = value;
    t.label = top;
    t.buttonText = {top, bottom};
    widgets_.push_back(t);
  };

  // Header.
  {
    Widget p;
    p.kind = Kind::Power;
    p.param = P_ENABLED;
    p.x = 34;
    p.y = 31;
    p.r = 14;
    widgets_.push_back(p);
  }
  {
    Widget os;
    os.kind = Kind::Cycle;
    os.param = P_OVERSAMPLE;
    os.x = 448;
    os.y = 18;
    os.w = 96;
    os.h = 26;
    widgets_.push_back(os);
  }

  // Colour tabs.
  static const char* kTabNames[5] = {"VALVE", "CONSOLE", "BUS", "TAPE",
                                     "XFMR"};
  for (int i = 0; i < 5; ++i) {
    Widget t;
    t.kind = Kind::ModelTab;
    t.param = P_COLOR_MODEL;
    t.value = i;
    t.x = 24 + i * 104;
    t.y = 66;
    t.w = 98;
    t.h = 38;
    t.label = kTabNames[i];
    widgets_.push_back(t);
  }

  // Input / colour.
  knob(P_INPUT, 70, 180, 26, "INPUT", sc(kScaleInput), true);
  knob(P_COLOR_AMOUNT, 160, 180, 22, "COLOUR", sc(kScaleAmount), true);

  // Dynamics.
  toggle(P_COMP_TYPE, 170, 286, 1.0, "76", "VCA");
  pill(P_DYN_POST, 104, 288, 58, 22, "POST EQ");

  knob(P_COMP_THRESHOLD, 70, 366, 24, "THRESH",
       sc(kScaleCompThresh), true);
  knob(P_COMP_MAKEUP, 160, 366, 22, "MAKEUP", sc(kScaleMakeup), true);
  toggle(P_COMP_FAST, 44, 442, 1.0, "FAST", "SLOW");
  pill(P_COMP_AUTO, 96, 442, 54, 22, "AUTO");
  pill(P_COMP_IN, 96, 468, 54, 22, "COMP");

  knob(P_GATE_THRESHOLD, 70, 542, 24, "THRESH",
       sc(kScaleGateThresh), true);
  knob(P_GATE_RANGE, 160, 542, 22, "RANGE", sc(kScaleRange), true);
  knob(P_GATE_RELEASE, 70, 620, 22, "RELEASE", sc(kScaleRelease), true);
  toggle(P_GATE_FAST, 150, 610, 1.0, "FAST", "SLOW");
  toggle(P_GATE_MODE, 190, 610, 1.0, "GATE", "EXP");
  pill(P_GATE_IN, 96, 674, 54, 22, "GATE");

  // Filters.
  knob(P_HPF, 70, 776, 24, "HPF", sc(kScaleHPF), true);
  knob(P_LPF, 160, 776, 24, "LPF", sc(kScaleLPF), true);

  // EQ.
  toggle(P_EQ_TYPE, 390, 124, 0.0, "BLACK", "BROWN");
  widgets_.back().h = 34;
  pill(P_EQ_IN, 330, 136, 48, 22, "EQ IN");

  // HF.
  knob(P_HF_GAIN, 300, 208, 24, "HF", sc(kScaleGain), true);
  knob(P_HF_FREQ, 370, 208, 20, "FREQ", sc(kScaleFreqHF), true);
  toggle(P_HF_BELL, 400, 272, 1.0, "BELL", "SHELF");
  widgets_.back().h = 30;

  // HMF.
  knob(P_HM_GAIN, 300, 360, 24, "HMF", sc(kScaleGain), true);
  knob(P_HM_FREQ, 370, 360, 20, "FREQ", sc(kScaleFreqHM), true);
  knob(P_HM_Q, 400, 414, 20, "Q", sc(kScaleQ), true);

  // LMF.
  knob(P_LM_GAIN, 300, 512, 24, "LMF", sc(kScaleGain), true);
  knob(P_LM_FREQ, 370, 512, 20, "FREQ", sc(kScaleFreqLM), true);
  knob(P_LM_Q, 400, 566, 20, "Q", sc(kScaleQ), true);

  // LF.
  knob(P_LF_GAIN, 300, 664, 24, "LF", sc(kScaleGain), true);
  knob(P_LF_FREQ, 370, 664, 20, "FREQ", sc(kScaleFreqLF), true);
  toggle(P_LF_BELL, 400, 724, 1.0, "BELL", "SHELF");

  // Output fader.
  {
    Widget f;
    f.kind = Kind::Fader;
    f.param = P_OUTPUT;
    f.x = 444;
    f.y = 744;
    f.w = 72;
    f.h = 176;
    f.label = "OUTPUT";
    widgets_.push_back(f);
  }

  (void)compType;
  (void)colour;
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void Panel::draw(cairo_t* cr, const Shared& shared, const PanelState& state) {
  layout(shared, state);
  cairo_set_source_rgb(cr, 0.055, 0.060, 0.070);
  cairo_paint(cr);

  drawHeader(cr, shared, state);
  drawBody(cr, shared, state);

  for (const Widget& w : widgets_) {
    drawWidget(cr, w, shared, state);
  }

  if (state.presetOpen) {
    drawPresetList(cr, shared, state);
  }
}

void Panel::drawHeader(cairo_t* cr, const Shared& shared,
                       const PanelState& state) const {
  cairo_pattern_t* bar = cairo_pattern_create_linear(0, 0, 0, 62);
  cairo_pattern_add_color_stop_rgb(bar, 0.0, 0.12, 0.13, 0.15);
  cairo_pattern_add_color_stop_rgb(bar, 1.0, 0.07, 0.08, 0.09);
  cairo_rectangle(cr, 0, 0, kPanelWidth, 62);
  cairo_set_source(cr, bar);
  cairo_fill(cr);
  cairo_pattern_destroy(bar);

  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.08);
  cairo_set_line_width(cr, 1.0);
  cairo_move_to(cr, 0, 0.5);
  cairo_line_to(cr, kPanelWidth, 0.5);
  cairo_stroke(cr);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.55);
  cairo_move_to(cr, 0, 61.5);
  cairo_line_to(cr, kPanelWidth, 61.5);
  cairo_stroke(cr);

  // Tab strip background.
  cairo_rectangle(cr, 0, 62, kPanelWidth, 46);
  cairo_set_source_rgba(cr, 0.11, 0.12, 0.14, 1.0);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.06);
  cairo_move_to(cr, 0, 107.5);
  cairo_line_to(cr, kPanelWidth, 107.5);
  cairo_stroke(cr);

  text(cr, "SUITE CHANNEL", 378, 38, 14, 1, 0.82, 0.85, 0.88);
  drawPresetBar(cr, shared, state);
}

void Panel::drawPresetBar(cairo_t* cr, const Shared& shared,
                          const PanelState& state) const {
  (void)shared;
  roundRect(cr, kPresetX, kPresetY, kPresetW, kPresetH, 4);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.35);
  cairo_fill(cr);
  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.12);
  roundRect(cr, kPresetX + 0.5, kPresetY + 0.5, kPresetW - 1, kPresetH - 1,
            4);
  cairo_stroke(cr);

  text(cr, "\u2039", kPresetX + 18, kPresetY + 22, 18, 1, 0.72, 0.75, 0.78);
  text(cr, "\u203a", kPresetX + kPresetW - 18, kPresetY + 22, 18, 1, 0.72,
       0.75, 0.78);

  char label[96];
  if (state.presetIndex >= 0 && state.presetIndex < kPresetCount) {
    std::snprintf(label, sizeof(label), "%s%s",
                  kPresets[state.presetIndex].name,
                  state.presetDirty ? " *" : "");
  } else {
    std::snprintf(label, sizeof(label), "Init%s",
                  state.presetDirty ? " *" : "");
  }
  text(cr, label, kPresetX + kPresetW * 0.5, kPresetY + 21, 13, 1, 0.90,
       0.92, 0.94);
}

void Panel::drawPresetList(cairo_t* cr, const Shared& shared,
                           const PanelState& state) const {
  (void)shared;
  const double h = kListVisible * kListRow + 10;
  roundRect(cr, kPresetX, kListY, kPresetW, h, 5);
  cairo_set_source_rgba(cr, 0.07, 0.08, 0.09, 0.97);
  cairo_fill(cr);
  cairo_set_line_width(cr, 1.0);
  setRgb(cr, skinFor(currentColour(shared)).tabOn, 0.7);
  roundRect(cr, kPresetX + 0.5, kListY + 0.5, kPresetW - 1, h - 1, 5);
  cairo_stroke(cr);

  cairo_save(cr);
  roundRect(cr, kPresetX + 1, kListY + 1, kPresetW - 2, h - 2, 4);
  cairo_clip(cr);

  const char* lastCategory = nullptr;
  double y = kListY + 6 - state.presetScroll * kListRow;
  for (int i : presetOrder()) {
    const Preset& p = kPresets[i];
    if (!lastCategory || std::strcmp(lastCategory, p.category) != 0) {
      lastCategory = p.category;
      text(cr, p.category, kPresetX + 14, y + 14, 10.5, 0, 0.52, 0.66, 0.80);
      y += kListRow;
      if (y > kListY + h) break;
    }
    if (y + kListRow > kListY && y < kListY + h) {
      if (i == state.presetIndex) {
        roundRect(cr, kPresetX + 6, y + 1, kPresetW - 12, kListRow - 2, 3);
        setRgb(cr, skinFor(currentColour(shared)).tabOn, 0.85);
        cairo_fill(cr);
      }
      text(cr, p.name, kPresetX + 26, y + 14, 12, 0,
           i == state.presetIndex ? 1.0 : 0.86,
           i == state.presetIndex ? 1.0 : 0.88,
           i == state.presetIndex ? 1.0 : 0.90);
    }
    y += kListRow;
  }
  cairo_restore(cr);
}

void Panel::applyPreset(Shared& shared, PanelState& state, int index) const {
  if (index < 0 || index >= kPresetCount) {
    return;
  }
  // Deterministic recall: reset every sound-shaping parameter first, leaving
  // power and oversampling alone.
  for (uint32_t p = 0; p < P_COUNT; ++p) {
    if (p == P_ENABLED || p == P_OVERSAMPLE) {
      continue;
    }
    shared.set(p, kParamSpecs[p].def);
  }
  const Preset& p = kPresets[index];
  for (const PresetValue& v : p.values) {
    if (v.param >= 0 && v.param < static_cast<int>(P_COUNT)) {
      shared.set(static_cast<uint32_t>(v.param), v.value);
    }
  }
  state.presetIndex = index;
  state.presetDirty = false;
  state.presetOpen = 0;
}

bool Panel::handlePresetClick(Shared& shared, PanelState& state, double x,
                              double y) {
  if (y >= kPresetY && y <= kPresetY + kPresetH) {
    if (x >= kPresetX && x <= kPresetX + 36) {
      const int next = state.presetIndex <= 0 ? 0 : state.presetIndex - 1;
      applyPreset(shared, state, next);
      return true;
    }
    if (x >= kPresetX + kPresetW - 36 && x <= kPresetX + kPresetW) {
      const int next = std::min(kPresetCount - 1, state.presetIndex + 1);
      applyPreset(shared, state, next);
      return true;
    }
    if (x > kPresetX + 36 && x < kPresetX + kPresetW - 36) {
      state.presetOpen = state.presetOpen ? 0 : 1;
      if (state.presetOpen) {
        state.presetScroll = 0;
      }
      return true;
    }
  }

  if (!state.presetOpen) {
    return false;
  }
  const double h = kListVisible * kListRow + 10;
  if (x < kPresetX || x > kPresetX + kPresetW || y < kListY ||
      y > kListY + h) {
    state.presetOpen = 0;
    return true;
  }

  double rowY = kListY + 6 - state.presetScroll * kListRow;
  const char* lastCategory = nullptr;
  for (int i : presetOrder()) {
    if (!lastCategory ||
        std::strcmp(lastCategory, kPresets[i].category) != 0) {
      lastCategory = kPresets[i].category;
      rowY += kListRow;
    }
    if (y >= rowY && y < rowY + kListRow) {
      applyPreset(shared, state, i);
      return true;
    }
    rowY += kListRow;
  }
  return true;
}

void Panel::drawBody(cairo_t* cr, const Shared& shared,
                     const PanelState& state) const {
  const Skin skin = skinFor(currentColour(shared));
  const int compType =
      std::clamp(static_cast<int>(shared.get(P_COMP_TYPE) + 0.5), 0, 1);

  // Faceplate.
  cairo_pattern_t* pat = cairo_pattern_create_linear(0, 116, 0, 1004);
  cairo_pattern_add_color_stop_rgb(pat, 0.0, skin.bodyTop.r, skin.bodyTop.g,
                                   skin.bodyTop.b);
  cairo_pattern_add_color_stop_rgb(pat, 1.0, skin.bodyBottom.r,
                                   skin.bodyBottom.g, skin.bodyBottom.b);
  roundRect(cr, 16, 116, 528, 888, 6);
  cairo_set_source(cr, pat);
  cairo_fill(cr);
  cairo_pattern_destroy(pat);

  if (grain_) {
    cairo_pattern_t* grain = cairo_pattern_create_for_surface(grain_);
    cairo_pattern_set_extend(grain, CAIRO_EXTEND_REPEAT);
    cairo_save(cr);
    cairo_rectangle(cr, 17, 117, 526, 886);
    cairo_clip(cr);
    cairo_set_source(cr, grain);
    cairo_paint_with_alpha(cr, 0.018);
    cairo_restore(cr);
    cairo_pattern_destroy(grain);
  }

  cairo_set_line_width(cr, 1.0);
  setRgb(cr, {1, 1, 1}, 0.16);
  cairo_move_to(cr, 18, 117.5);
  cairo_line_to(cr, 542, 117.5);
  cairo_stroke(cr);
  setRgb(cr, {0, 0, 0}, 0.22);
  cairo_move_to(cr, 18, 1002.5);
  cairo_line_to(cr, 542, 1002.5);
  cairo_stroke(cr);

  for (double sx : {32.0, 528.0}) {
    for (double sy : {132.0, 988.0}) {
      cairo_arc(cr, sx, sy, 4.5, 0, 2 * kPi);
      setRgb(cr, skin.screw, 0.9);
      cairo_fill(cr);
    }
  }

  auto section = [&](double x, double y, double w, double h,
                     const char* title) {
    roundRect(cr, x, y, w, h, 5);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.16);
    cairo_fill(cr);
    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.07);
    roundRect(cr, x + 0.5, y + 0.5, w - 1, h - 1, 5);
    cairo_stroke(cr);
    text(cr, title, x + 10, y + 18, 10.5, 0, skin.legend.r, skin.legend.g,
         skin.legend.b);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.10);
    cairo_move_to(cr, x + 10, y + 26);
    cairo_line_to(cr, x + w - 10, y + 26);
    cairo_stroke(cr);
  };

  section(24, 124, 192, 112, "INPUT / COLOUR");
  section(24, 244, 192, 456, "DYNAMICS");
  section(24, 708, 192, 152, "FILTERS");
  section(228, 124, 196, 800, "EQUALISER");
  section(436, 124, 100, 800, "OUTPUT");

  // Dynamics sub-headers.
  text(cr, "COMP", 34, 322, 10.0, 0, skin.legendDim.r, skin.legendDim.g,
       skin.legendDim.b);
  text(cr, compType == 0 ? "3.5:1" : "4:1", 206, 322, 10.0, 2,
       skin.legendDim.r, skin.legendDim.g, skin.legendDim.b);
  text(cr, "GATE / EXPANDER", 34, 500, 10.0, 0, skin.legendDim.r,
       skin.legendDim.g, skin.legendDim.b);

  // EQ band accents and labels.
  const char* bandNames[4] = {"HF", "HMF", "LMF", "LF"};
  const Rgb bandCols[4] = {{0.90, 0.36, 0.30},
                           {0.35, 0.78, 0.42},
                           {0.36, 0.62, 0.92},
                           {0.86, 0.58, 0.28}};
  const double bandY[4] = {164.0, 316.0, 468.0, 620.0};
  for (int i = 0; i < 4; ++i) {
    setRgb(cr, bandCols[i], 0.9);
    cairo_rectangle(cr, 236, bandY[i], 3, 140);
    cairo_fill(cr);
    text(cr, bandNames[i], 248, bandY[i] + 16, 12, 0, bandCols[i].r,
         bandCols[i].g, bandCols[i].b);
  }

  // GR meters.
  auto grMeter = [&](double x, double y, double w, double h, float gr,
                     double maxGr, Rgb colour, const char* label) {
    roundRect(cr, x, y, w, h, 3);
    cairo_set_source_rgba(cr, 0.03, 0.03, 0.04, 0.9);
    cairo_fill(cr);
    const double f = clamp01(static_cast<double>(gr) / maxGr);
    if (f > 0.0) {
      roundRect(cr, x + 2, y + 2, w - 4, (h - 4) * f, 2);
      setRgb(cr, colour, 0.95);
      cairo_fill(cr);
    }
    text(cr, label, x + w * 0.5, y + h + 12, 8.5, 1, skin.legendDim.r,
         skin.legendDim.g, skin.legendDim.b);
  };
  grMeter(446, 168, 22, 150, state.grDb, 24.0, {0.95, 0.55, 0.20}, "COMP");
  grMeter(478, 168, 22, 150, state.gateGrDb, 40.0, {0.35, 0.80, 0.45},
          "GATE");

  // Output level meter.
  drawLevelMeter(cr, 446, 360, 56, 260, state.outLevelL, state.outLevelR,
                 -60.0, 6.0, false);
  text(cr, "OUT", 474, 636, 8.5, 1, skin.legendDim.r, skin.legendDim.g,
       skin.legendDim.b);

  // Footer.
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.55);
  cairo_rectangle(cr, 0, 1004, kPanelWidth, 36);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.10);
  cairo_set_line_width(cr, 1.0);
  cairo_move_to(cr, 0, 1004.5);
  cairo_line_to(cr, kPanelWidth, 1004.5);
  cairo_stroke(cr);

  char caption[96];
  std::snprintf(caption, sizeof(caption), "%s  ·  %s channel strip",
                kColourLabels[currentColour(shared)],
                compType == 0 ? "VCA" : "76Blue");
  text(cr, caption, kPanelWidth * 0.5, 1028, 12, 1, 0.86, 0.88, 0.90);
}

void Panel::drawWidget(cairo_t* cr, const Widget& w, const Shared& shared,
                       const PanelState& state) const {
  (void)state;
  const int colour = currentColour(shared);
  const Skin skin = skinFor(colour);
  switch (w.kind) {
    case Kind::Power: {
      const bool on = shared.get(P_ENABLED) > 0.5;
      cairo_arc(cr, w.x, w.y, w.r, 0, 2 * kPi);
      setRgb(cr, {0.12, 0.12, 0.12});
      cairo_fill(cr);
      cairo_arc(cr, w.x, w.y, w.r - 1.5, 0, 2 * kPi);
      setRgb(cr, on ? Rgb{0.36, 0.86, 0.46} : Rgb{0.42, 0.42, 0.42});
      cairo_set_line_width(cr, 2.2);
      cairo_new_path(cr);
      cairo_arc(cr, w.x, w.y, 7, -kPi / 2 + 0.6, -kPi / 2 + 2 * kPi - 0.6);
      cairo_stroke(cr);
      cairo_move_to(cr, w.x, w.y - 9);
      cairo_line_to(cr, w.x, w.y - 1);
      cairo_stroke(cr);
      break;
    }
    case Kind::ModelTab: {
      const bool selected =
          std::fabs(shared.get(P_COLOR_MODEL) - w.value) < 0.5;
      roundRect(cr, w.x, w.y, w.w, w.h, 4);
      setRgb(cr, selected ? skin.tabOn : skin.tabOff);
      cairo_fill(cr);
      const Rgb tc = selected ? skin.tabTextOn : skin.tabTextOff;
      text(cr, w.label, w.x + w.w * 0.5, w.y + w.h * 0.5 + 4.5, 10.5, 1,
           tc.r, tc.g, tc.b);
      break;
    }
    case Kind::Knob:
      drawKnob(cr, w, knobValue(shared, w));
      break;
    case Kind::Button:
      drawButton(cr, w, isLit(shared, w));
      break;
    case Kind::Toggle:
      drawToggle(cr, w, shared.get(w.param) > 0.5);
      break;
    case Kind::Pill:
      drawPill(cr, w, shared.get(w.param) > 0.5);
      break;
    case Kind::Cycle: {
      static const char* const kOsNames[] = {"OS OFF", "OS 2X", "OS 4X"};
      const int v = std::clamp(static_cast<int>(shared.get(w.param) + 0.5),
                               0, 2);
      Widget copy = w;
      copy.label = kOsNames[v];
      drawPill(cr, copy, v != 0);
      break;
    }
    case Kind::Fader:
      drawFader(cr, w, knobValue(shared, w));
      break;
  }
}

void Panel::drawKnob(cairo_t* cr, const Widget& w, double value) const {
  const Skin skin = skinFor(1);
  const double cx = w.x;
  const double cy = w.y;
  const double r = w.r;

  setRgb(cr, skin.legendDim, 0.55);
  cairo_set_line_width(cr, 1.0);
  for (int i = 0; i <= 10; ++i) {
    const double a = knobAngle(i / 10.0);
    cairo_move_to(cr, cx + std::cos(a) * r * 1.10,
                  cy + std::sin(a) * r * 1.10);
    cairo_line_to(cr, cx + std::cos(a) * r * 1.19,
                  cy + std::sin(a) * r * 1.19);
    cairo_stroke(cr);
  }

  const ParamSpec& spec = kParamSpecs[w.param];
  if (w.scale.empty() && !spec.stepped) {
    const double a0 = knobAngle(0.0);
    const double a1 = knobAngle(1.0);
    text(cr, "\u2212", cx + std::cos(a0) * (r * 1.44),
         cy + std::sin(a0) * (r * 1.44) + 4, 12, 1, skin.legendDim.r,
         skin.legendDim.g, skin.legendDim.b);
    text(cr, "+", cx + std::cos(a1) * (r * 1.44),
         cy + std::sin(a1) * (r * 1.44) + 4, 12, 1, skin.legendDim.r,
         skin.legendDim.g, skin.legendDim.b);
  }

  const size_t n = w.scale.size();
  for (size_t i = 0; i < n; ++i) {
    const double f = n > 1 ? static_cast<double>(i) / (n - 1) : 0.0;
    const double a = knobAngle(f);
    const double lx = cx + std::cos(a) * (r * w.scaleRadius);
    const double ly = cy + std::sin(a) * (r * w.scaleRadius);
    text(cr, w.scale[i], lx, ly + 4, 9.0, 1, skin.legend.r, skin.legend.g,
         skin.legend.b);
  }

  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.18);
  cairo_arc(cr, cx + 0.5, cy + 1.5, r * 1.03, 0, 2 * kPi);
  cairo_fill(cr);

  cairo_pattern_t* body = cairo_pattern_create_linear(0, cy - r, 0, cy + r);
  const Rgb top = mix(skin.knobSkirt, {1, 1, 1}, 0.16);
  const Rgb bot = mix(skin.knobSkirt, {0, 0, 0}, 0.30);
  cairo_pattern_add_color_stop_rgb(body, 0.0, top.r, top.g, top.b);
  cairo_pattern_add_color_stop_rgb(body, 1.0, bot.r, bot.g, bot.b);
  cairo_arc(cr, cx, cy, r, 0, 2 * kPi);
  cairo_set_source(cr, body);
  cairo_fill(cr);
  cairo_pattern_destroy(body);
  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.45);
  cairo_arc(cr, cx, cy, r - 0.5, 0, 2 * kPi);
  cairo_stroke(cr);

  cairo_pattern_t* cap = cairo_pattern_create_radial(
      cx - r * 0.35, cy - r * 0.40, r * 0.05, cx, cy, r * 0.98);
  const Rgb hi = mix(skin.knobCap, {1, 1, 1}, 0.24);
  cairo_pattern_add_color_stop_rgb(cap, 0.0, hi.r, hi.g, hi.b);
  cairo_pattern_add_color_stop_rgb(cap, 0.75, skin.knobCap.r, skin.knobCap.g,
                                   skin.knobCap.b);
  cairo_pattern_add_color_stop_rgb(cap, 1.0, skin.knobCapEdge.r,
                                   skin.knobCapEdge.g, skin.knobCapEdge.b);
  cairo_arc(cr, cx, cy, r * 0.82, 0, 2 * kPi);
  cairo_set_source(cr, cap);
  cairo_fill(cr);
  cairo_pattern_destroy(cap);

  const ParamSpec& spec2 = kParamSpecs[w.param];
  const double vmax = w.steps > 1 ? w.steps - 1 : spec2.max;
  const double span = vmax - spec2.min;
  const double frac =
      span > 0.0 ? (std::min(value, vmax) - spec2.min) / span : 0.0;
  const double a = knobAngle(clamp01(frac));
  cairo_set_line_width(cr, 2.0);
  setRgb(cr, bandAccent(w.param));
  cairo_move_to(cr, cx + std::cos(a) * r * 0.26,
                cy + std::sin(a) * r * 0.26);
  cairo_line_to(cr, cx + std::cos(a) * r * 0.78,
                cy + std::sin(a) * r * 0.78);
  cairo_stroke(cr);

  const double labelY = w.labelBelow ? cy + r * 1.44 + 18 : cy - r * 1.44 - 16;
  text(cr, w.label, cx, labelY, 10.0, 1, skin.legend.r, skin.legend.g,
       skin.legend.b);
}

void Panel::drawPill(cairo_t* cr, const Widget& w, bool on) const {
  const Skin skin = skinFor(1);
  roundRect(cr, w.x, w.y, w.w, w.h, 5);
  setRgb(cr, on ? skin.btnOn : skin.btnOff);
  cairo_fill(cr);
  cairo_set_line_width(cr, 1.0);
  setRgb(cr, {0.0, 0.0, 0.0}, 0.35);
  roundRect(cr, w.x + 0.5, w.y + 0.5, w.w - 1.0, w.h - 1.0, 5);
  cairo_stroke(cr);
  const Rgb tc = on ? skin.btnTextOn : skin.btnText;
  text(cr, w.label, w.x + w.w * 0.5, w.y + w.h * 0.5 + 3.5, 9.0, 1, tc.r,
       tc.g, tc.b);
}

void Panel::drawButton(cairo_t* cr, const Widget& w, bool lit) const {
  const Skin skin = skinFor(1);
  const Rgb base = lit ? skin.btnOn : skin.btnOff;
  cairo_pattern_t* pat = cairo_pattern_create_linear(0, w.y, 0, w.y + w.h);
  const Rgb hi = mix(base, {1, 1, 1}, lit ? 0.10 : 0.07);
  const Rgb lo = mix(base, {0, 0, 0}, 0.10);
  cairo_pattern_add_color_stop_rgb(pat, 0.0, hi.r, hi.g, hi.b);
  cairo_pattern_add_color_stop_rgb(pat, 1.0, lo.r, lo.g, lo.b);
  roundRect(cr, w.x, w.y, w.w, w.h, 3);
  cairo_set_source(cr, pat);
  cairo_fill(cr);
  cairo_pattern_destroy(pat);
  setRgb(cr, mix(base, lit ? Rgb{1, 1, 1} : Rgb{0, 0, 0}, 0.35), 0.9);
  cairo_set_line_width(cr, 1.0);
  roundRect(cr, w.x + 0.5, w.y + 0.5, w.w - 1, w.h - 1, 3);
  cairo_stroke(cr);
  const Rgb tc = lit ? skin.btnTextOn : skin.btnText;
  text(cr, w.label, w.x + w.w * 0.5, w.y + w.h * 0.5 + 4, 10.0, 1, tc.r,
       tc.g, tc.b);
}

void Panel::drawToggle(cairo_t* cr, const Widget& w, bool on) const {
  const Skin skin = skinFor(1);
  roundRect(cr, w.x - w.w * 0.5, w.y, w.w, w.h, 4);
  setRgb(cr, mix(skin.knobSkirt, {0, 0, 0}, 0.3));
  cairo_fill(cr);
  const double leverY = on ? w.y + 12 : w.y + w.h - 12;
  cairo_arc(cr, w.x, leverY, 8, 0, 2 * kPi);
  setRgb(cr, skin.knobCap);
  cairo_fill(cr);
  if (w.buttonText.size() >= 2) {
    text(cr, w.buttonText[0], w.x, w.y - 6, 9.0, 1, skin.legend.r,
         skin.legend.g, skin.legend.b);
    text(cr, w.buttonText[1], w.x, w.y + w.h + 13, 9.0, 1, skin.legend.r,
         skin.legend.g, skin.legend.b);
  }
}

void Panel::drawFader(cairo_t* cr, const Widget& w, double value) const {
  const Skin skin = skinFor(1);
  const double cx = w.x + w.w * 0.5;
  const double top = w.y + 12;
  const double bottom = w.y + w.h - 12;

  roundRect(cr, cx - 3, top, 6, bottom - top, 3);
  setRgb(cr, {0.03, 0.03, 0.04}, 0.9);
  cairo_fill(cr);

  const double f = clamp01(value);
  const double hy = bottom - f * (bottom - top);
  roundRect(cr, cx - 2, hy, 4, bottom - hy, 2);
  setRgb(cr, skin.tabOn, 0.95);
  cairo_fill(cr);

  roundRect(cr, cx - 18, hy - 9, 36, 18, 3);
  cairo_pattern_t* handle = cairo_pattern_create_linear(0, hy - 9, 0, hy + 9);
  cairo_pattern_add_color_stop_rgb(handle, 0.0, 0.72, 0.74, 0.77);
  cairo_pattern_add_color_stop_rgb(handle, 1.0, 0.42, 0.44, 0.47);
  cairo_set_source(cr, handle);
  cairo_fill(cr);
  cairo_pattern_destroy(handle);

  cairo_set_source_rgba(cr, 1, 1, 1, 0.35);
  cairo_set_line_width(cr, 1.0);
  cairo_move_to(cr, cx - 14, hy);
  cairo_line_to(cr, cx + 14, hy);
  cairo_stroke(cr);

  text(cr, w.label, cx, w.y - 8, 9.5, 1, skin.legend.r, skin.legend.g,
       skin.legend.b);

  text(cr, "+18", cx + 26, top + 4, 8.0, 0, skin.legendDim.r,
       skin.legendDim.g, skin.legendDim.b);
  text(cr, "0", cx + 26, (top + bottom) * 0.5 + 3, 8.0, 0,
       skin.legendDim.r, skin.legendDim.g, skin.legendDim.b);
  text(cr, "-18", cx + 26, bottom + 2, 8.0, 0, skin.legendDim.r,
       skin.legendDim.g, skin.legendDim.b);
}

void Panel::drawLevelMeter(cairo_t* cr, double x, double y, double w, double h,
                           double dbL, double dbR, double minDb,
                           double maxDb, bool labelsRight) const {
  (void)labelsRight;
  roundRect(cr, x, y, w, h, 3);
  cairo_set_source_rgba(cr, 0.03, 0.03, 0.04, 0.9);
  cairo_fill(cr);

  auto bar = [&](double bx, double db) {
    const double f = clamp01((db - minDb) / (maxDb - minDb));
    const double bh = (h - 4) * f;
    if (bh <= 0.0) {
      return;
    }
    const double by = y + h - 2 - bh;
    cairo_save(cr);
    roundRect(cr, bx, by, 16, bh, 1);
    cairo_clip(cr);
    if (db >= -1.0) {
      setRgb(cr, {0.95, 0.30, 0.22});
    } else if (db >= -6.0) {
      setRgb(cr, {0.95, 0.78, 0.25});
    } else {
      setRgb(cr, {0.35, 0.82, 0.42});
    }
    cairo_paint(cr);
    cairo_restore(cr);
  };
  bar(x + 5, dbL);
  bar(x + w - 21, dbR);
}

void Panel::tick(const Shared& shared, PanelState& state) const {
  state.grDb = shared.grDb.load(std::memory_order_relaxed);
  state.gateGrDb = shared.gateGrDb.load(std::memory_order_relaxed);
  state.inLevelL = shared.inPeakLDb.load(std::memory_order_relaxed);
  state.inLevelR = shared.inPeakRDb.load(std::memory_order_relaxed);
  state.outLevelL = shared.outPeakLDb.load(std::memory_order_relaxed);
  state.outLevelR = shared.outPeakRDb.load(std::memory_order_relaxed);
  state.inLevel = std::max(state.inLevelL, state.inLevelR);
  state.outLevel = std::max(state.outLevelL, state.outLevelR);
  state.outPeakHold = std::max(state.outPeakHold - 0.25f, state.outLevel);
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

bool Panel::hitWidget(const Widget& w, double x, double y) const {
  switch (w.kind) {
    case Kind::Knob:
    case Kind::Power: {
      const double dx = x - w.x;
      const double dy = y - w.y;
      return dx * dx + dy * dy <= w.r * w.r * 1.35;
    }
    case Kind::Toggle:
      return x >= w.x - w.w * 0.5 && x <= w.x + w.w * 0.5 && y >= w.y &&
             y <= w.y + w.h;
    default:
      return x >= w.x && x <= w.x + w.w && y >= w.y && y <= w.y + w.h;
  }
}

double Panel::knobValue(const Shared& shared, const Widget& w) const {
  return shared.get(w.param);
}

bool Panel::isLit(const Shared& shared, const Widget& w) const {
  return std::fabs(shared.get(w.param) - w.value) < 1e-3;
}

bool Panel::pointerDown(Shared& shared, PanelState& state, double x, double y,
                        bool fine) {
  (void)fine;
  if (handlePresetClick(shared, state, x, y)) {
    return true;
  }
  for (const Widget& w : widgets_) {
    if (!hitWidget(w, x, y)) {
      continue;
    }
    switch (w.kind) {
      case Kind::Knob:
        dragParam_ = static_cast<int>(w.param);
        dragFader_ = false;
        dragSteps_ = w.steps;
        dragStartY_ = y;
        dragStartValue_ = shared.get(w.param);
        if (dragSteps_ > 1) {
          dragStartValue_ = std::min(dragStartValue_, dragSteps_ - 1.0);
        }
        return false;
      case Kind::Fader:
        dragParam_ = static_cast<int>(w.param);
        dragFader_ = true;
        dragStartY_ = y;
        dragStartValue_ = shared.get(w.param);
        {
          const double top = w.y + 12.0;
          const double bottom = w.y + w.h - 12.0;
          const double v = (bottom - y) / (bottom - top);
          shared.set(w.param, v);
          state.presetDirty = true;
        }
        return true;
      case Kind::Power:
        shared.set(P_ENABLED, shared.get(P_ENABLED) > 0.5 ? 0.0 : 1.0);
        state.presetDirty = true;
        return true;
      case Kind::ModelTab:
        shared.set(w.param, w.value);
        state.presetDirty = true;
        return true;
      case Kind::Button:
        shared.set(w.param, w.value);
        state.presetDirty = true;
        return true;
      case Kind::Toggle: {
        const double f = (y - w.y) / w.h;
        shared.set(w.param, f < 0.5 ? 1.0 : 0.0);
        state.presetDirty = true;
        return true;
      }
      case Kind::Pill:
        shared.set(w.param, shared.get(w.param) > 0.5 ? 0.0 : 1.0);
        state.presetDirty = true;
        return true;
      case Kind::Cycle: {
        const int v = static_cast<int>(shared.get(w.param) + 0.5);
        shared.set(w.param, static_cast<double>((v + 1) % 3));
        state.presetDirty = true;
        return true;
      }
    }
  }
  return false;
}

bool Panel::pointerMove(Shared& shared, PanelState& state, double x, double y,
                        bool fine) {
  (void)x;
  if (dragParam_ < 0) {
    return false;
  }
  const uint32_t param = static_cast<uint32_t>(dragParam_);
  const ParamSpec& spec = kParamSpecs[param];

  if (dragFader_) {
    for (const Widget& w : widgets_) {
      if (w.kind == Kind::Fader && w.param == param) {
        const double top = w.y + 12.0;
        const double bottom = w.y + w.h - 12.0;
        shared.set(param, (bottom - y) / (bottom - top));
        state.presetDirty = true;
        return true;
      }
    }
    return false;
  }

  const double perPixel = spec.stepped
                              ? 1.0 / 18.0
                              : (spec.max - spec.min) / (fine ? 800.0 : 200.0);
  double v = dragStartValue_ + (dragStartY_ - y) * perPixel;
  if (dragSteps_ > 1) {
    v = std::min(v, dragSteps_ - 1.0);
  }
  shared.set(param, v);
  state.presetDirty = true;
  return true;
}

void Panel::pointerUp() {
  dragParam_ = -1;
  dragFader_ = false;
}

bool Panel::wheel(Shared& shared, PanelState& state, double x, double y,
                  double dy) {
  if (state.presetOpen && x >= kPresetX && x <= kPresetX + kPresetW &&
      y >= kListY && y <= kListY + kListVisible * kListRow + 10) {
    int rows = kPresetCount;
    const char* last = nullptr;
    for (int i : presetOrder()) {
      if (!last || std::strcmp(last, kPresets[i].category) != 0) {
        last = kPresets[i].category;
        ++rows;
      }
    }
    const int maxScroll =
        std::max(0, rows - static_cast<int>(kListVisible));
    state.presetScroll =
        std::min(maxScroll, std::max(0, state.presetScroll - (int)dy));
    return true;
  }
  for (const Widget& w : widgets_) {
    if ((w.kind == Kind::Knob || w.kind == Kind::Fader) &&
        hitWidget(w, x, y)) {
      const ParamSpec& spec = kParamSpecs[w.param];
      if (spec.stepped) {
        shared.set(w.param, shared.get(w.param) - (dy > 0 ? 1.0 : -1.0));
      } else {
        shared.set(w.param,
                   shared.get(w.param) - dy * (spec.max - spec.min) * 0.01);
      }
      state.presetDirty = true;
      return true;
    }
  }
  return false;
}

std::vector<Panel::WidgetRect> Panel::widgetRects() const {
  std::vector<WidgetRect> out;
  out.reserve(widgets_.size());
  for (const Widget& w : widgets_) {
    WidgetRect r;
    r.param = w.param;
    r.label = w.label;
    r.knob = w.kind == Kind::Knob;
    r.toggle = w.kind == Kind::Toggle;
    r.r = w.r;
    if (w.kind == Kind::Knob) {
      r.x = w.x - w.r;
      r.y = w.y - w.r;
      r.w = w.r * 2.0;
      r.h = w.r * 2.0;
    } else if (w.kind == Kind::Toggle) {
      r.x = w.x - w.w * 0.5;
      r.y = w.y;
      r.w = w.w;
      r.h = w.h;
    } else {
      r.x = w.x;
      r.y = w.y;
      r.w = w.w;
      r.h = w.h;
    }
    out.push_back(r);
  }
  return out;
}

} // namespace suite
