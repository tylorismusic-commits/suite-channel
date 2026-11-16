// Suite Channel — panel.cpp
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/panel.hpp"

#include "presets.hpp"

#include <cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

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

struct KnobPalette {
  Rgb cap;
  Rgb edge;
  Rgb indicator;
};

// Console-style fill colours: HF red, HMF green, LMF blue, LF amber; the
// gate is green and the filters are the usual light grey.
KnobPalette paletteFor(uint32_t param, bool blackEq) {
  switch (param) {
    case P_HF_GAIN:
    case P_HF_FREQ:
    case P_HF_BELL:
      return {{0.72, 0.20, 0.16}, {0.34, 0.06, 0.05}, {1.0, 0.90, 0.84}};
    case P_HM_GAIN:
    case P_HM_FREQ:
    case P_HM_Q:
      return {{0.24, 0.58, 0.28}, {0.06, 0.24, 0.09}, {0.90, 1.0, 0.88}};
    case P_LM_GAIN:
    case P_LM_FREQ:
    case P_LM_Q:
      return {{0.22, 0.42, 0.72}, {0.06, 0.16, 0.34}, {0.86, 0.93, 1.0}};
    case P_LF_GAIN:
    case P_LF_FREQ:
    case P_LF_BELL:
      if (blackEq) {
        return {{0.13, 0.13, 0.14}, {0.035, 0.035, 0.045}, {0.88, 0.88, 0.86}};
      }
      return {{0.64, 0.42, 0.16}, {0.28, 0.16, 0.05}, {1.0, 0.92, 0.78}};
    case P_GATE_THRESHOLD:
    case P_GATE_RANGE:
    case P_GATE_RELEASE:
      return {{0.26, 0.56, 0.30}, {0.06, 0.22, 0.09}, {0.90, 1.0, 0.88}};
    case P_HPF:
    case P_LPF:
      return {{0.72, 0.74, 0.76}, {0.40, 0.42, 0.45}, {0.10, 0.11, 0.12}};
    default:
      return {{0.20, 0.52, 0.82}, {0.07, 0.27, 0.46}, {0.92, 0.96, 1.0}};
  }
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

template <size_t N>
std::vector<const char*> sc(const char* const (&labels)[N]) {
  return std::vector<const char*>(labels, labels + N);
}

constexpr double kPi = 3.14159265358979323846;
constexpr double kFaderInset = 34.0;

constexpr double kPresetX = 64.0;
constexpr double kPresetY = 14.0;
constexpr double kPresetW = 260.0;
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

std::string lowerText(std::string s) {
  for (char& c : s) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return s;
}

std::string trimText(const std::string& s) {
  const size_t a = s.find_first_not_of(" \t");
  if (a == std::string::npos) {
    return {};
  }
  const size_t b = s.find_last_not_of(" \t");
  return s.substr(a, b - a + 1);
}

std::string formatHz(double hz) {
  char buf[48];
  if (hz >= 1000.0) {
    std::snprintf(buf, sizeof(buf), "%.2f kHz", hz / 1000.0);
  } else {
    std::snprintf(buf, sizeof(buf), "%.0f Hz", hz);
  }
  return buf;
}

std::string formatDb(double db) {
  char buf[48];
  std::snprintf(buf, sizeof(buf), "%+.1f dB", db);
  return buf;
}

std::string formatMs(double ms) {
  char buf[48];
  if (ms >= 1000.0) {
    std::snprintf(buf, sizeof(buf), "%.2f s", ms / 1000.0);
  } else {
    std::snprintf(buf, sizeof(buf), "%.0f ms", ms);
  }
  return buf;
}

std::string formatParamValue(uint32_t p, double v) {
  const ParamSpec& s = kParamSpecs[p];
  if (s.labels.count > 0) {
    const int i = std::clamp(static_cast<int>(v + 0.5), 0, s.labels.count - 1);
    return s.labels.names[i];
  }
  char buf[64];
  switch (p) {
    case P_ENABLED:
    case P_POLARITY:
    case P_EQ_IN:
    case P_LF_BELL:
    case P_HF_BELL:
    case P_COMP_IN:
    case P_COMP_FAST:
    case P_COMP_AUTO:
    case P_GATE_IN:
    case P_GATE_FAST:
    case P_DYN_POST:
    case P_LIMITER_ON:
      return v > 0.5 ? "On" : "Off";
    case P_INPUT:
      return formatDb(inputGainDb(static_cast<float>(v)));
    case P_OUTPUT:
      return v <= 0.0005
                 ? "OFF"
                 : formatDb(outputGainDb(static_cast<float>(v)));
    case P_COMP_THRESHOLD:
      return formatDb(compThresholdDb(static_cast<float>(v)));
    case P_COMP_MAKEUP:
      return formatDb(compMakeupDb(static_cast<float>(v)));
    case P_HPF:
      return v <= 0.001 ? "OUT" : formatHz(hpfHz(static_cast<float>(v)));
    case P_LPF:
      return v <= 0.001 ? "OUT" : formatHz(lpfHz(static_cast<float>(v)));
    case P_LF_GAIN:
    case P_LM_GAIN:
    case P_HM_GAIN:
    case P_HF_GAIN:
      return formatDb(bandGainDb(static_cast<float>(v)));
    case P_LF_FREQ:
      return formatHz(lfHz(static_cast<float>(v)));
    case P_LM_FREQ:
      return formatHz(lmHz(static_cast<float>(v)));
    case P_HM_FREQ:
      return formatHz(hmHz(static_cast<float>(v)));
    case P_HF_FREQ:
      return formatHz(hfHz(static_cast<float>(v)));
    case P_LM_Q:
    case P_HM_Q:
      std::snprintf(buf, sizeof(buf), "%.2f", bandQ(static_cast<float>(v)));
      return buf;
    case P_GATE_THRESHOLD:
      return formatDb(gateThresholdDb(static_cast<float>(v)));
    case P_GATE_RANGE:
      std::snprintf(buf, sizeof(buf), "%.0f dB",
                    gateRangeDb(static_cast<float>(v)));
      return buf;
    case P_GATE_RELEASE:
      return formatMs(gateReleaseMs(static_cast<float>(v)));
    case P_GATE_HOLD:
      return formatMs(gateHoldMs(static_cast<float>(v)));
    case P_COLOR_AMOUNT:
      std::snprintf(buf, sizeof(buf), "%.1f", v * 10.0);
      return buf;
    default:
      std::snprintf(buf, sizeof(buf), "%.2f", v);
      return buf;
  }
}

bool parseParamValue(uint32_t p, const std::string& raw, double* out) {
  const ParamSpec& s = kParamSpecs[p];
  const std::string text = lowerText(trimText(raw));
  if (text.empty()) {
    return false;
  }
  for (int i = 0; i < s.labels.count; ++i) {
    if (text == lowerText(s.labels.names[i])) {
      *out = i;
      return true;
    }
  }
  if (text == "on" || text == "true" || text == "yes") {
    *out = 1.0;
    return true;
  }
  if (text == "off" || text == "false" || text == "no") {
    *out = 0.0;
    return true;
  }
  if (text == "out") {
    *out = 0.0;
    return true;
  }

  char* end = nullptr;
  const double value = std::strtod(text.c_str(), &end);
  if (end == text.c_str()) {
    return false;
  }
  const std::string suffix = trimText(std::string(end));

  auto freq = [&]() {
    double hz = value;
    if (suffix == "khz" || suffix == "k") {
      hz *= 1000.0;
    }
    return hz;
  };
  auto ms = [&]() {
    double t = value;
    if (suffix == "s" || suffix == "sec") {
      t *= 1000.0;
    }
    return t;
  };

  switch (p) {
    case P_INPUT:
      *out = (value + 18.0) / 36.0;
      return true;
    case P_OUTPUT: {
      if (value >= 0.0) {
        *out = 0.75 + value / 48.0;
      } else {
        *out = 0.75 * std::pow(10.0, value / 60.0);
      }
      return true;
    }
    case P_COMP_THRESHOLD:
      *out = (10.0 - value) / 30.0;
      return true;
    case P_COMP_MAKEUP:
      *out = (value + 9.0) / 18.0;
      return true;
    case P_HPF: {
      const double hz = freq();
      *out = hz <= 16.0 ? 0.0
                        : std::log(hz / 16.0) / std::log(350.0 / 16.0);
      return true;
    }
    case P_LPF: {
      const double hz = freq();
      *out = hz <= 0.0
                 ? 0.0
                 : std::log(std::clamp(hz, 3000.0, 15200.0) / 3000.0) /
                       std::log(15200.0 / 3000.0);
      return true;
    }
    case P_LF_GAIN:
    case P_LM_GAIN:
    case P_HM_GAIN:
    case P_HF_GAIN:
      *out = (value + 15.0) / 30.0;
      return true;
    case P_LF_FREQ: {
      const double hz = freq();
      *out = std::log(std::clamp(hz, 30.0, 450.0) / 30.0) /
             std::log(450.0 / 30.0);
      return true;
    }
    case P_LM_FREQ: {
      const double hz = freq();
      *out = std::log(std::clamp(hz, 200.0, 2500.0) / 200.0) /
             std::log(2500.0 / 200.0);
      return true;
    }
    case P_HM_FREQ: {
      const double hz = freq();
      *out = std::log(std::clamp(hz, 600.0, 7000.0) / 600.0) /
             std::log(7000.0 / 600.0);
      return true;
    }
    case P_HF_FREQ: {
      const double hz = freq();
      *out = std::log(std::clamp(hz, 1500.0, 16000.0) / 1500.0) /
             std::log(16000.0 / 1500.0);
      return true;
    }
    case P_LM_Q:
    case P_HM_Q:
      *out = std::log(std::clamp(value, 0.5, 3.0) / 0.5) /
             std::log(3.0 / 0.5);
      return true;
    case P_GATE_THRESHOLD:
      *out = (value + 30.0) / 40.0;
      return true;
    case P_GATE_RANGE:
      *out = value / 40.0;
      return true;
    case P_GATE_RELEASE:
      *out = std::log(std::clamp(ms(), 100.0, 4000.0) / 100.0) /
             std::log(40.0);
      return true;
    case P_GATE_HOLD:
      *out = ms() / 500.0;
      return true;
    case P_COLOR_AMOUNT:
      *out = value / 10.0;
      return true;
    default:
      *out = value;
      return true;
  }
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
  const bool silverComp = compType == 1;
  const bool blackEq =
      std::clamp(static_cast<int>(shared.get(P_EQ_TYPE) + 0.5), 0, 1) == 1;

  auto knob = [&](uint32_t param, double x, double y, double r,
                  const char* label, std::vector<const char*> scale,
                  bool below = false, double scaleRadius = 1.44) {
    Widget w;
    w.kind = Kind::Knob;
    w.param = param;
    w.x = x;
    w.y = y;
    w.r = r;
    w.label = label;
    w.scale = std::move(scale);
    w.labelBelow = below;
    w.scaleRadius = scaleRadius;
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

  auto button = [&](uint32_t param, double x, double y, double w, double h,
                    double value, const char* label) {
    Widget b;
    b.kind = Kind::Button;
    b.param = param;
    b.x = x;
    b.y = y;
    b.w = w;
    b.h = h;
    b.value = value;
    b.label = label;
    widgets_.push_back(b);
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
    os.x = 580;
    os.y = 18;
    os.w = 100;
    os.h = 26;
    widgets_.push_back(os);
  }
  {
    Widget a;
    a.kind = Kind::AB;
    a.value = 0.0;
    a.x = 332;
    a.y = 18;
    a.w = 32;
    a.h = 26;
    a.label = "A";
    widgets_.push_back(a);
  }
  {
    Widget b;
    b.kind = Kind::AB;
    b.value = 1.0;
    b.x = 368;
    b.y = 18;
    b.w = 32;
    b.h = 26;
    b.label = "B";
    widgets_.push_back(b);
  }
  {
    Widget c;
    c.kind = Kind::Copy;
    c.x = 404;
    c.y = 18;
    c.w = 40;
    c.h = 26;
    c.label = "COPY";
    widgets_.push_back(c);
  }

  // Colour tabs.
  static const char* kTabNames[5] = {"VALVE", "CONSOLE", "BUS", "TAPE",
                                     "XFMR"};
  for (int i = 0; i < 5; ++i) {
    Widget t;
    t.kind = Kind::ModelTab;
    t.param = P_COLOR_MODEL;
    t.value = i;
    t.x = 24 + i * 132;
    t.y = 66;
    t.w = 124;
    t.h = 38;
    t.label = kTabNames[i];
    widgets_.push_back(t);
  }

  // Input / colour.
  knob(P_INPUT, 80, 176, 28, "INPUT", sc(kScaleInput), true);
  knob(P_COLOR_AMOUNT, 180, 176, 24, "COLOUR", sc(kScaleAmount), true);
  pill(P_POLARITY, 165, 132, 64, 20, "\u00d8");

  // Dynamics.
  pill(P_DYN_POST, 60, 286, 56, 22, "POST EQ");
  widgets_.back().onSilver = silverComp;
  button(P_COMP_TYPE, 120, 286, 44, 22, 0.0, "VCA");
  widgets_.back().onSilver = silverComp;
  button(P_COMP_TYPE, 168, 286, 44, 22, 1.0, "76");
  widgets_.back().onSilver = silverComp;

  knob(P_COMP_THRESHOLD, 80, 372, 26, "THRESH",
       sc(kScaleCompThresh), true);
  widgets_.back().onSilver = silverComp;
  knob(P_COMP_MAKEUP, 180, 372, 24, "MAKEUP", sc(kScaleMakeup), true);
  widgets_.back().onSilver = silverComp;
  pill(P_COMP_FAST, 44, 430, 58, 22, "F.ATK");
  widgets_.back().onSilver = silverComp;
  pill(P_COMP_AUTO, 112, 430, 56, 22, "AUTO");
  widgets_.back().onSilver = silverComp;
  pill(P_COMP_IN, 112, 456, 56, 22, "COMP");
  widgets_.back().onSilver = silverComp;

  // Gate.
  knob(P_GATE_THRESHOLD, 60, 550, 24, "THRESH",
       sc(kScaleGateThresh), true);
  knob(P_GATE_RANGE, 130, 550, 22, "RANGE", sc(kScaleRange), true);
  knob(P_GATE_RELEASE, 200, 550, 22, "RELEASE", sc(kScaleRelease), true);
  pill(P_GATE_FAST, 40, 620, 58, 22, "F.ATK");
  button(P_GATE_MODE, 104, 620, 44, 22, 1.0, "GATE");
  button(P_GATE_MODE, 152, 620, 44, 22, 0.0, "EXP");
  button(P_GATE_MODE, 200, 620, 44, 22, 2.0, "DUCK");
  pill(P_GATE_IN, 80, 660, 56, 22, "GATE IN");
  {
    Widget h;
    h.kind = Kind::Hold;
    h.param = P_GATE_HOLD;
    h.x = 150;
    h.y = 660;
    h.w = 86;
    h.h = 22;
    h.label = "HOLD";
    widgets_.push_back(h);
  }

  // Filters.
  knob(P_HPF, 80, 776, 26, "HPF", sc(kScaleHPF), true);
  knob(P_LPF, 180, 776, 26, "LPF", sc(kScaleLPF), true);

  // EQ.
  pill(P_EQ_IN, 350, 136, 52, 22, "EQ IN");
  button(P_EQ_TYPE, 316, 788, 62, 22, 0.0, "BROWN");
  button(P_EQ_TYPE, 384, 788, 62, 22, 1.0, "BLACK");

  // HF.
  knob(P_HF_GAIN, 330, 208, 28, "HF", sc(kScaleGain), true, 1.35);
  knob(P_HF_FREQ, 405, 208, 22, "FREQ", sc(kScaleFreqHF), true, 1.30);
  pill(P_HF_BELL, 450, 272, 52, 22, "BELL");

  // HMF.
  knob(P_HM_GAIN, 330, 360, 28, "HMF", sc(kScaleGain), true, 1.35);
  knob(P_HM_FREQ, 405, 360, 22, "FREQ", sc(kScaleFreqHM), true, 1.30);
  knob(P_HM_Q, 452, 410, 22, "Q", sc(kScaleQ), true, 1.30);

  // LMF.
  knob(P_LM_GAIN, 330, 512, 28, "LMF", sc(kScaleGain), true, 1.35);
  knob(P_LM_FREQ, 405, 512, 22, "FREQ", sc(kScaleFreqLM), true, 1.30);
  knob(P_LM_Q, 452, 562, 22, "Q", sc(kScaleQ), true, 1.30);

  // LF.
  knob(P_LF_GAIN, 330, 664, 28, "LF", sc(kScaleGain), true, 1.35);
  widgets_.back().blackEq = blackEq;
  knob(P_LF_FREQ, 405, 664, 22, "FREQ", sc(kScaleFreqLF), true, 1.30);
  widgets_.back().blackEq = blackEq;
  pill(P_LF_BELL, 450, 728, 52, 22, "BELL");
  widgets_.back().blackEq = blackEq;

  // Output limiter pill.
  pill(P_LIMITER_ON, 528, 720, 44, 22, "LIMIT");

  // Output fader.
  {
    Widget f;
    f.kind = Kind::Fader;
    f.param = P_OUTPUT;
    f.x = 590;
    f.y = 168;
    f.w = 72;
    f.h = 660;
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

  if (dragParam_ >= 0 && !editing_) {
    const uint32_t param = static_cast<uint32_t>(dragParam_);
    drawValueBubble(cr, param, shared.get(param));
  }
  if (editing_) {
    drawTextEntry(cr, shared);
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

  text(cr, "SUITE CHANNEL", 505, 38, 13.5, 1, 0.82, 0.85, 0.88);
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

void Panel::applyAB(Shared& shared, PanelState& state, int target) const {
  auto snapshot = [&](double* dst) {
    for (uint32_t p = 0; p < P_COUNT; ++p) {
      dst[p] = shared.get(p);
    }
  };
  auto restore = [&](const double* src) {
    for (uint32_t p = 0; p < P_COUNT; ++p) {
      shared.set(p, src[p]);
    }
  };

  if (!state.aValid) {
    snapshot(state.aValues);
    state.aValid = true;
    state.activeAB = 0;
    state.presetDirty = true;
    return;
  }
  if (state.activeAB == 0) {
    snapshot(state.aValues);
  } else {
    snapshot(state.bValues);
  }

  if (target == 0) {
    restore(state.aValues);
    state.activeAB = 0;
  } else {
    if (!state.bValid) {
      snapshot(state.bValues);
      state.bValid = true;
    } else {
      restore(state.bValues);
    }
    state.activeAB = 1;
  }
  state.presetDirty = true;
}

void Panel::copyAB(Shared& shared, PanelState& state) const {
  if (!state.aValid) {
    for (uint32_t p = 0; p < P_COUNT; ++p) {
      state.aValues[p] = shared.get(p);
    }
    state.aValid = true;
    state.activeAB = 0;
  }
  if (state.activeAB == 0) {
    for (uint32_t p = 0; p < P_COUNT; ++p) {
      state.aValues[p] = shared.get(p);
      state.bValues[p] = state.aValues[p];
    }
    state.bValid = true;
  } else {
    for (uint32_t p = 0; p < P_COUNT; ++p) {
      state.bValues[p] = shared.get(p);
      state.aValues[p] = state.bValues[p];
    }
    state.aValid = true;
  }
  state.presetDirty = true;
}

void Panel::beginTextEntry(Shared& shared, PanelState& state, double x,
                           double y) {
  (void)shared;
  mouseX_ = x;
  mouseY_ = y;
  state.presetOpen = 0;
  for (const Widget& w : widgets_) {
    if (!hitWidget(w, x, y)) {
      continue;
    }
    if (w.kind != Kind::Knob && w.kind != Kind::Fader &&
        w.kind != Kind::Hold) {
      return;
    }
    editing_ = true;
    editParam_ = w.param;
    editText_.clear();
    return;
  }
}

bool Panel::textKey(Shared& shared, PanelState& state, int key, char ch) {
  if (!editing_) {
    return false;
  }
  if (key == kTextEnter) {
    return commitTextEntry(shared, state);
  }
  if (key == kTextEscape) {
    editing_ = false;
    editText_.clear();
    return true;
  }
  if (key == kTextBackspace) {
    if (!editText_.empty()) {
      editText_.pop_back();
    }
    return true;
  }
  if (ch >= 32 && ch < 127 && editText_.size() < 24) {
    editText_.push_back(ch);
  }
  return true;
}

bool Panel::commitTextEntry(Shared& shared, PanelState& state) {
  if (!editing_) {
    return false;
  }
  if (editText_.empty()) {
    editing_ = false;
    return false;
  }
  double value = 0.0;
  if (!parseParamValue(editParam_, editText_, &value)) {
    return false;
  }
  shared.set(editParam_, value);
  state.presetDirty = true;
  editing_ = false;
  editText_.clear();
  return true;
}

void Panel::drawValueBubble(cairo_t* cr, uint32_t param, double value) const {
  const Skin skin = skinFor(1);
  const std::string label = formatParamValue(param, value);
  constexpr double w = 92.0;
  constexpr double h = 20.0;
  const double x =
      std::clamp(mouseX_ + 14.0, 4.0, kPanelWidth - w - 4.0);
  const double y =
      std::clamp(mouseY_ - 34.0, 4.0, kPanelHeight - h - 4.0);
  roundRect(cr, x, y, w, h, 4);
  cairo_set_source_rgba(cr, 0.045, 0.05, 0.06, 0.96);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, 1, 1, 1, 0.16);
  cairo_set_line_width(cr, 1.0);
  roundRect(cr, x + 0.5, y + 0.5, w - 1, h - 1, 4);
  cairo_stroke(cr);
  text(cr, label.c_str(), x + w * 0.5, y + 14.0, 9.5, 1, skin.legend.r,
       skin.legend.g, skin.legend.b);
}

void Panel::drawTextEntry(cairo_t* cr, const Shared& shared) const {
  if (!editing_) {
    return;
  }
  const Skin skin = skinFor(1);
  const ParamSpec& spec = kParamSpecs[editParam_];
  const std::string current =
      formatParamValue(editParam_, shared.get(editParam_));
  const std::string shown =
      editText_.empty() ? current : (editText_ + "|");
  constexpr double w = 124.0;
  constexpr double h = 26.0;
  const double x =
      std::clamp(mouseX_ + 14.0, 4.0, kPanelWidth - w - 4.0);
  const double y =
      std::clamp(mouseY_ - 42.0, 4.0, kPanelHeight - h - 4.0);

  roundRect(cr, x, y, w, h, 4);
  cairo_set_source_rgba(cr, 0.035, 0.04, 0.05, 0.98);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, skin.tabOn.r, skin.tabOn.g, skin.tabOn.b, 0.9);
  cairo_set_line_width(cr, 1.2);
  roundRect(cr, x + 0.5, y + 0.5, w - 1, h - 1, 4);
  cairo_stroke(cr);

  text(cr, spec.name, x + 7, y + 9.5, 7.5, 0, skin.legendDim.r,
       skin.legendDim.g, skin.legendDim.b);
  text(cr, shown.c_str(), x + 7, y + 21, 9.5, 0,
       editText_.empty() ? skin.legendDim.r : skin.legend.r,
       editText_.empty() ? skin.legendDim.g : skin.legend.g,
       editText_.empty() ? skin.legendDim.b : skin.legend.b);
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
  const int gateMode =
      std::clamp(static_cast<int>(shared.get(P_GATE_MODE) + 0.5), 0, 1);
  const bool blackEq =
      std::clamp(static_cast<int>(shared.get(P_EQ_TYPE) + 0.5), 0, 1) == 1;

  // Faceplate.
  cairo_pattern_t* pat = cairo_pattern_create_linear(0, 116, 0, 884);
  cairo_pattern_add_color_stop_rgb(pat, 0.0, skin.bodyTop.r, skin.bodyTop.g,
                                   skin.bodyTop.b);
  cairo_pattern_add_color_stop_rgb(pat, 1.0, skin.bodyBottom.r,
                                   skin.bodyBottom.g, skin.bodyBottom.b);
  roundRect(cr, 16, 116, 668, 768, 6);
  cairo_set_source(cr, pat);
  cairo_fill(cr);
  cairo_pattern_destroy(pat);

  if (grain_) {
    cairo_pattern_t* grain = cairo_pattern_create_for_surface(grain_);
    cairo_pattern_set_extend(grain, CAIRO_EXTEND_REPEAT);
    cairo_save(cr);
    cairo_rectangle(cr, 17, 117, 666, 766);
    cairo_clip(cr);
    cairo_set_source(cr, grain);
    cairo_paint_with_alpha(cr, 0.018);
    cairo_restore(cr);
    cairo_pattern_destroy(grain);
  }

  cairo_set_line_width(cr, 1.0);
  setRgb(cr, {1, 1, 1}, 0.16);
  cairo_move_to(cr, 18, 117.5);
  cairo_line_to(cr, 682, 117.5);
  cairo_stroke(cr);
  setRgb(cr, {0, 0, 0}, 0.22);
  cairo_move_to(cr, 18, 882.5);
  cairo_line_to(cr, 682, 882.5);
  cairo_stroke(cr);

  for (double sx : {32.0, 668.0}) {
    for (double sy : {132.0, 868.0}) {
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

  section(24, 124, 220, 112, "INPUT / COLOUR");
  section(24, 244, 220, 456, "DYNAMICS");
  section(24, 708, 220, 152, "FILTERS");
  section(256, 124, 250, 736, "EQUALISER");
  section(518, 124, 158, 736, "OUTPUT");

  // The 76Blue module gets a brushed-silver face, like the reference's
  // alternate compressor panel.
  if (compType == 1) {
    cairo_pattern_t* silver =
        cairo_pattern_create_linear(0, 278, 0, 482);
    cairo_pattern_add_color_stop_rgb(silver, 0.0, 0.84, 0.85, 0.86);
    cairo_pattern_add_color_stop_rgb(silver, 1.0, 0.56, 0.58, 0.61);
    roundRect(cr, 30, 278, 210, 204, 5);
    cairo_set_source(cr, silver);
    cairo_fill(cr);
    cairo_pattern_destroy(silver);

    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.045);
    for (double y = 282; y < 478; y += 3.0) {
      cairo_move_to(cr, 33, y);
      cairo_line_to(cr, 237, y);
      cairo_stroke(cr);
    }
    cairo_set_source_rgba(cr, 0.10, 0.11, 0.12, 0.55);
    roundRect(cr, 30.5, 278.5, 209, 203, 5);
    cairo_stroke(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.35);
    cairo_move_to(cr, 34, 280.5);
    cairo_line_to(cr, 236, 280.5);
    cairo_stroke(cr);
  }

  text(cr, "VOICING", 316, 780, 9.0, 0, skin.legendDim.r, skin.legendDim.g,
       skin.legendDim.b);

  // Dynamics sub-headers. The compressor labels sit on silver when the
  // 76Blue panel is active; both subheaders use the same title + rule.
  const Rgb compLegend =
      compType == 1 ? Rgb{0.12, 0.13, 0.14} : skin.legendDim;
  auto subHeader = [&](double y, const char* title, const char* right,
                       bool silver) {
    const Rgb ink = silver ? compLegend : skin.legendDim;
    text(cr, title, 34, y, 10.0, 0, ink.r, ink.g, ink.b);
    if (right && *right) {
      text(cr, right, 236, y, 10.0, 2, ink.r, ink.g, ink.b);
    }
    if (silver) {
      cairo_set_source_rgba(cr, 0.10, 0.11, 0.12, 0.35);
    } else {
      cairo_set_source_rgba(cr, 1, 1, 1, 0.10);
    }
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, 34, y + 8);
    cairo_line_to(cr, 236, y + 8);
    cairo_stroke(cr);
  };
  subHeader(322, "COMP", compType == 0 ? "3.5:1" : "4:1", compType == 1);
  subHeader(498, "GATE / EXPANDER",
            gateMode == 0 ? "EXPANDER"
                          : (gateMode == 1 ? "GATE" : "DUCKER"),
            false);

  // EQ band accents and labels.
  const char* bandNames[4] = {"HF", "HMF", "LMF", "LF"};
  Rgb bandCols[4] = {{0.90, 0.36, 0.30},
                     {0.35, 0.78, 0.42},
                     {0.36, 0.62, 0.92},
                     {0.86, 0.58, 0.28}};
  if (blackEq) {
    bandCols[3] = {0.55, 0.56, 0.58};
  }
  const double bandY[4] = {164.0, 316.0, 468.0, 620.0};
  for (int i = 0; i < 4; ++i) {
    setRgb(cr, bandCols[i], 0.9);
    cairo_rectangle(cr, 264, bandY[i], 3, 140);
    cairo_fill(cr);
    text(cr, bandNames[i], 276, bandY[i] + 16, 12, 0, bandCols[i].r,
         bandCols[i].g, bandCols[i].b);
  }

  // GR meters: a short LED ladder, like the reference's meter column.
  auto grMeter = [&](double x, double y, double w, double h, float gr,
                     double maxGr, Rgb colour, const char* label) {
    roundRect(cr, x, y, w, h, 3);
    cairo_set_source_rgba(cr, 0.02, 0.02, 0.03, 0.95);
    cairo_fill(cr);
    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.10);
    roundRect(cr, x + 0.5, y + 0.5, w - 1, h - 1, 3);
    cairo_stroke(cr);

    constexpr int kSegments = 14;
    const double top = y + 5.0;
    const double bottom = y + h - 5.0;
    const double segH = (bottom - top) / kSegments;
    const double f = clamp01(static_cast<double>(gr) / maxGr);
    for (int i = 0; i < kSegments; ++i) {
      const double sy = top + i * segH;
      const bool lit = f * kSegments > static_cast<double>(i);
      roundRect(cr, x + 4, sy + 1, w - 8, segH - 2, 1.5);
      if (lit) {
        setRgb(cr, colour, 0.95);
      } else {
        setRgb(cr, mix(colour, {0, 0, 0}, 0.78), 0.65);
      }
      cairo_fill(cr);
    }
    // Scale ticks at 0, half and full.
    cairo_set_source_rgba(cr, 1, 1, 1, 0.20);
    for (double frac : {0.0, 0.5, 1.0}) {
      const double ty = top + (bottom - top) * frac;
      cairo_move_to(cr, x + 1, ty);
      cairo_line_to(cr, x + 4, ty);
      cairo_stroke(cr);
    }
    text(cr, label, x + w * 0.5, y + h + 12, 8.5, 1, skin.legendDim.r,
         skin.legendDim.g, skin.legendDim.b);
  };
  grMeter(528, 168, 28, 180, state.grDb, 24.0, {0.95, 0.55, 0.20}, "COMP");
  grMeter(562, 168, 28, 180, state.gateGrDb, 40.0, {0.35, 0.80, 0.45},
          "GATE");

  // Input and output level meters, stacked to the left of the long fader.
  drawLevelMeter(cr, 528, 380, 44, 140, state.inLevelL, state.inLevelR,
                 -60.0, 6.0, true);
  text(cr, "IN", 550, 530, 8.5, 1, skin.legendDim.r, skin.legendDim.g,
       skin.legendDim.b);

  drawLevelMeter(cr, 528, 548, 44, 140, state.outLevelL, state.outLevelR,
                 -60.0, 6.0, true);
  text(cr, "OUT", 550, 698, 8.5, 1, skin.legendDim.r, skin.legendDim.g,
       skin.legendDim.b);

  // Footer.
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.55);
  cairo_rectangle(cr, 0, 884, kPanelWidth, 36);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.10);
  cairo_set_line_width(cr, 1.0);
  cairo_move_to(cr, 0, 884.5);
  cairo_line_to(cr, kPanelWidth, 884.5);
  cairo_stroke(cr);

  char caption[96];
  std::snprintf(caption, sizeof(caption), "%s  ·  %s channel strip",
                kColourLabels[currentColour(shared)],
                compType == 0 ? "VCA" : "76Blue");
  text(cr, caption, kPanelWidth * 0.5, 908, 12, 1, 0.86, 0.88, 0.90);
}

void Panel::drawWidget(cairo_t* cr, const Widget& w, const Shared& shared,
                       const PanelState& state) const {
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
    case Kind::AB:
      drawButton(cr, w, state.activeAB == static_cast<int>(w.value));
      break;
    case Kind::Copy:
      drawButton(cr, w, false);
      break;
    case Kind::Hold: {
      std::string label =
          "HOLD " + formatParamValue(P_GATE_HOLD, shared.get(P_GATE_HOLD));
      Widget copy = w;
      copy.label = label.c_str();
      drawPill(cr, copy, shared.get(P_GATE_HOLD) > 0.001);
      break;
    }
  }
}

void Panel::drawKnob(cairo_t* cr, const Widget& w, double value) const {
  const Skin skin = skinFor(1);
  const KnobPalette pal = paletteFor(w.param, w.blackEq);
  const Rgb legend = w.onSilver ? Rgb{0.13, 0.14, 0.15} : skin.legend;
  const Rgb legendDim = w.onSilver ? Rgb{0.30, 0.31, 0.33} : skin.legendDim;
  const double cx = w.x;
  const double cy = w.y;
  const double r = w.r;

  setRgb(cr, legendDim, 0.55);
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
         cy + std::sin(a0) * (r * 1.44) + 4, 12, 1, legendDim.r,
         legendDim.g, legendDim.b);
    text(cr, "+", cx + std::cos(a1) * (r * 1.44),
         cy + std::sin(a1) * (r * 1.44) + 4, 12, 1, legendDim.r,
         legendDim.g, legendDim.b);
  }

  const size_t n = w.scale.size();
  for (size_t i = 0; i < n; ++i) {
    const double f = n > 1 ? static_cast<double>(i) / (n - 1) : 0.0;
    const double a = knobAngle(f);
    const double lx = cx + std::cos(a) * (r * w.scaleRadius);
    const double ly = cy + std::sin(a) * (r * w.scaleRadius);
    text(cr, w.scale[i], lx, ly + 4, 9.0, 1, legend.r, legend.g,
         legend.b);
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
  const Rgb hi = mix(pal.cap, {1, 1, 1}, 0.24);
  cairo_pattern_add_color_stop_rgb(cap, 0.0, hi.r, hi.g, hi.b);
  cairo_pattern_add_color_stop_rgb(cap, 0.75, pal.cap.r, pal.cap.g,
                                   pal.cap.b);
  cairo_pattern_add_color_stop_rgb(cap, 1.0, pal.edge.r,
                                   pal.edge.g, pal.edge.b);
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
  setRgb(cr, pal.indicator);
  cairo_move_to(cr, cx + std::cos(a) * r * 0.26,
                cy + std::sin(a) * r * 0.26);
  cairo_line_to(cr, cx + std::cos(a) * r * 0.78,
                cy + std::sin(a) * r * 0.78);
  cairo_stroke(cr);

  const double labelY = w.labelBelow ? cy + r * 1.44 + 18 : cy - r * 1.44 - 16;
  text(cr, w.label, cx, labelY, 10.0, 1, legend.r, legend.g,
       legend.b);
}

void Panel::drawPill(cairo_t* cr, const Widget& w, bool on) const {
  const Skin skin = skinFor(1);
  const Rgb off = w.onSilver ? Rgb{0.40, 0.42, 0.45} : skin.btnOff;
  const Rgb offText = w.onSilver ? Rgb{0.12, 0.13, 0.14} : skin.btnText;
  roundRect(cr, w.x, w.y, w.w, w.h, 5);
  setRgb(cr, on ? skin.btnOn : off);
  cairo_fill(cr);
  cairo_set_line_width(cr, 1.0);
  setRgb(cr, {0.0, 0.0, 0.0}, 0.35);
  roundRect(cr, w.x + 0.5, w.y + 0.5, w.w - 1.0, w.h - 1.0, 5);
  cairo_stroke(cr);
  const Rgb tc = on ? skin.btnTextOn : offText;
  text(cr, w.label, w.x + w.w * 0.5, w.y + w.h * 0.5 + 3.5, 9.0, 1, tc.r,
       tc.g, tc.b);
}

void Panel::drawButton(cairo_t* cr, const Widget& w, bool lit) const {
  const Skin skin = skinFor(1);
  const Rgb base =
      lit ? skin.btnOn : (w.onSilver ? Rgb{0.40, 0.42, 0.45} : skin.btnOff);
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
  const Rgb tc =
      lit ? skin.btnTextOn
          : (w.onSilver ? Rgb{0.12, 0.13, 0.14} : skin.btnText);
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
  const double top = w.y + kFaderInset;
  const double bottom = w.y + w.h - kFaderInset;
  const double f = clamp01(value);
  const double hy = bottom - f * (bottom - top);

  // Long fader scale, like the reference's centre-section fader.
  cairo_set_source_rgba(cr, 1, 1, 1, 0.14);
  cairo_set_line_width(cr, 1.0);
  cairo_move_to(cr, cx - 34, top);
  cairo_line_to(cr, cx - 34, bottom);
  cairo_stroke(cr);
  for (int i = 0; i <= 8; ++i) {
    const double frac = i / 8.0;
    const double ty = bottom - frac * (bottom - top);
    const double len = (i == 4) ? 11.0 : 5.0;
    cairo_move_to(cr, cx - 34, ty);
    cairo_line_to(cr, cx - 34 + len, ty);
    cairo_stroke(cr);
  }
  cairo_set_source_rgba(cr, 1, 1, 1, 0.16);
  cairo_move_to(cr, cx - 36, (top + bottom) * 0.5);
  cairo_line_to(cr, cx + 30, (top + bottom) * 0.5);
  cairo_stroke(cr);

  // Dark groove with a thin centre line; the Reapertips-style thumb is a
  // dark cap with a light, ribbed grip rather than a plastic knob.
  roundRect(cr, cx - 7, top, 14, bottom - top, 4);
  setRgb(cr, {0.035, 0.038, 0.045}, 0.98);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, 1, 1, 1, 0.08);
  cairo_move_to(cr, cx - 6.5, top + 4);
  cairo_line_to(cr, cx - 6.5, bottom - 4);
  cairo_stroke(cr);
  cairo_set_source_rgba(cr, 0, 0, 0, 0.9);
  cairo_move_to(cr, cx, top + 4);
  cairo_line_to(cr, cx, bottom - 4);
  cairo_stroke(cr);

  // Thumb: 27x53, matching the Reapertips MCP volume thumb. Dark outer cap,
  // light inset grip and horizontal ribs.
  constexpr double kHandleW = 27.0;
  constexpr double kHandleH = 53.0;
  roundRect(cr, cx - kHandleW * 0.5, hy - kHandleH * 0.5, kHandleW,
            kHandleH, 4);
  cairo_pattern_t* cap =
      cairo_pattern_create_linear(0, hy - kHandleH * 0.5, 0,
                                  hy + kHandleH * 0.5);
  cairo_pattern_add_color_stop_rgb(cap, 0.0, 0.22, 0.23, 0.25);
  cairo_pattern_add_color_stop_rgb(cap, 1.0, 0.075, 0.08, 0.09);
  cairo_set_source(cr, cap);
  cairo_fill(cr);
  cairo_pattern_destroy(cap);
  cairo_set_source_rgba(cr, 1, 1, 1, 0.20);
  roundRect(cr, cx - kHandleW * 0.5 + 0.5, hy - kHandleH * 0.5 + 0.5,
            kHandleW - 1.0, kHandleH - 1.0, 4);
  cairo_stroke(cr);

  roundRect(cr, cx - 7.5, hy - 18, 15, 36, 3);
  cairo_set_source_rgba(cr, 0.80, 0.82, 0.84, 0.98);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, 0.12, 0.13, 0.14, 0.70);
  cairo_set_line_width(cr, 1.0);
  for (int i = -5; i <= 5; ++i) {
    const double ly = hy + i * 3.0;
    cairo_move_to(cr, cx - 5, ly);
    cairo_line_to(cr, cx + 5, ly);
    cairo_stroke(cr);
  }

  struct FaderMark {
    const char* label;
    double value;
  };
  const FaderMark marks[] = {{"+12", 1.0},    {"+6", 0.875}, {"0", 0.75},
                             {"-12", 0.4732}, {"-24", 0.2986}, {"OFF", 0.0}};
  for (const FaderMark& mark : marks) {
    const double ty = bottom - mark.value * (bottom - top);
    text(cr, mark.label, cx + 28, ty + 3, 7.5, 0, skin.legendDim.r,
         skin.legendDim.g, skin.legendDim.b);
  }
}

void Panel::drawLevelMeter(cairo_t* cr, double x, double y, double w, double h,
                           double dbL, double dbR, double minDb,
                           double maxDb, bool labelsRight) const {
  const Skin skin = skinFor(1);
  roundRect(cr, x, y, w, h, 3);
  cairo_set_source_rgba(cr, 0.02, 0.02, 0.03, 0.95);
  cairo_fill(cr);
  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.10);
  roundRect(cr, x + 0.5, y + 0.5, w - 1, h - 1, 3);
  cairo_stroke(cr);

  constexpr int kSegments = 18;
  const double top = y + 4.0;
  const double bottom = y + h - 4.0;
  const double segH = (bottom - top) / kSegments;

  auto bar = [&](double bx, double db) {
    for (int i = 0; i < kSegments; ++i) {
      const double sy = bottom - (i + 1) * segH;
      const double segDb =
          minDb + (maxDb - minDb) * (i + 1) / kSegments;
      const bool lit = db >= segDb;
      roundRect(cr, bx, sy + 1, 16, segH - 2, 1.5);
      if (lit) {
        if (segDb >= -1.0) {
          setRgb(cr, {0.96, 0.30, 0.22}, 0.98);
        } else if (segDb >= -6.0) {
          setRgb(cr, {0.96, 0.78, 0.25}, 0.98);
        } else {
          setRgb(cr, {0.35, 0.84, 0.42}, 0.98);
        }
      } else {
        setRgb(cr, {0.07, 0.08, 0.09}, 0.9);
      }
      cairo_fill(cr);
    }
  };
  bar(x + 5, dbL);
  bar(x + w - 21, dbR);

  if (labelsRight) {
    const double marks[] = {0.0, -6.0, -12.0, -24.0, -48.0};
    for (double marked : marks) {
      if (marked < minDb || marked > maxDb) {
        continue;
      }
      const double f = (marked - minDb) / (maxDb - minDb);
      const double ly = bottom - f * (bottom - top);
      cairo_set_source_rgba(cr, 1, 1, 1, 0.16);
      cairo_move_to(cr, x + 2, ly);
      cairo_line_to(cr, x + w - 2, ly);
      cairo_stroke(cr);
      char label[16];
      std::snprintf(label, sizeof(label), "%.0f", marked);
      text(cr, label, x + w + 2, ly + 3, 7.0, 0, skin.legendDim.r,
           skin.legendDim.g, skin.legendDim.b);
    }
  }
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
  mouseX_ = x;
  mouseY_ = y;
  if (editing_) {
    editing_ = false;
    editText_.clear();
  }
  if (handlePresetClick(shared, state, x, y)) {
    return true;
  }
  auto doubleClicked = [&](uint32_t param) {
    const auto now = std::chrono::steady_clock::now();
    const bool twice =
        param == lastClickParam_ &&
        now - lastClickTime_ < std::chrono::milliseconds(350);
    lastClickParam_ = twice ? 0xffffffffu : param;
    lastClickTime_ = now;
    if (twice) {
      shared.set(param, kParamSpecs[param].def);
      state.presetDirty = true;
      return true;
    }
    return false;
  };

  for (const Widget& w : widgets_) {
    if (!hitWidget(w, x, y)) {
      continue;
    }
    switch (w.kind) {
      case Kind::Knob:
        if (doubleClicked(w.param)) {
          return true;
        }
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
        if (doubleClicked(w.param)) {
          return true;
        }
        dragParam_ = static_cast<int>(w.param);
        dragFader_ = true;
        dragStartY_ = y;
        dragStartValue_ = shared.get(w.param);
        {
          const double top = w.y + kFaderInset;
          const double bottom = w.y + w.h - kFaderInset;
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
      case Kind::AB:
        applyAB(shared, state, static_cast<int>(w.value));
        return true;
      case Kind::Copy:
        copyAB(shared, state);
        return true;
      case Kind::Hold: {
        static const double steps[] = {0.0, 0.1, 0.2, 0.4, 0.8, 1.0};
        const double v = shared.get(P_GATE_HOLD);
        int idx = 0;
        for (int i = 0; i < 6; ++i) {
          if (std::fabs(v - steps[i]) < 0.05) {
            idx = i;
            break;
          }
        }
        idx = (idx + 1) % 6;
        shared.set(P_GATE_HOLD, steps[idx]);
        state.presetDirty = true;
        return true;
      }
    }
  }
  return false;
}

bool Panel::pointerMove(Shared& shared, PanelState& state, double x, double y,
                        bool fine) {
  mouseX_ = x;
  mouseY_ = y;
  if (dragParam_ < 0) {
    return false;
  }
  const uint32_t param = static_cast<uint32_t>(dragParam_);
  const ParamSpec& spec = kParamSpecs[param];

  if (dragFader_) {
    for (const Widget& w : widgets_) {
      if (w.kind == Kind::Fader && w.param == param) {
        const double top = w.y + kFaderInset;
        const double bottom = w.y + w.h - kFaderInset;
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
