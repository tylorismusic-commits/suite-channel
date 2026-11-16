// Suite Channel — panel.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cairo.h>

#include <chrono>
#include <string>
#include <vector>

#include "params.hpp"

namespace suite {

constexpr int kPanelWidth = 700;
constexpr int kPanelHeight = 920;

struct PanelState {
  int presetIndex = -1;
  bool presetDirty = false;
  int presetOpen = 0;
  int presetScroll = 0;
  float inLevel = 0.0f;
  float outLevel = 0.0f;
  float inLevelL = 0.0f;
  float inLevelR = 0.0f;
  float outLevelL = 0.0f;
  float outLevelR = 0.0f;
  float outPeakHold = 0.0f;
  float grDb = 0.0f;
  float gateGrDb = 0.0f;

  // Editor-local A/B compare slots and copy/paste clipboard.
  bool aValid = false;
  bool bValid = false;
  int activeAB = 0;
  double aValues[P_COUNT] = {};
  double bValues[P_COUNT] = {};
  bool clipboardValid = false;
  double clipboardValues[P_COUNT] = {};
};

class Panel {
public:
  Panel();
  ~Panel();
  Panel(const Panel&) = delete;
  Panel& operator=(const Panel&) = delete;

  void loadFont(const std::string& dir);
  void tick(const Shared& shared, PanelState& state) const;
  void draw(cairo_t* cr, const Shared& shared, const PanelState& state);

  bool pointerDown(Shared& shared, PanelState& state, double x, double y,
                   bool fine);
  bool pointerMove(Shared& shared, PanelState& state, double x, double y,
                   bool fine);
  void pointerUp();
  bool handlePresetClick(Shared& shared, PanelState& state, double x, double y);
  void applyPreset(Shared& shared, PanelState& state, int index) const;
  void drawPresetBar(cairo_t* cr, const Shared& shared,
                     const PanelState& state) const;
  void drawPresetList(cairo_t* cr, const Shared& shared,
                      const PanelState& state) const;
  bool wheel(Shared& shared, PanelState& state, double x, double y, double dy);

  // Right-click opens a typed-value editor; Enter applies, Escape cancels.
  void beginTextEntry(Shared& shared, PanelState& state, double x, double y);
  static constexpr int kTextEnter = 1;
  static constexpr int kTextEscape = 2;
  static constexpr int kTextBackspace = 3;
  bool textKey(Shared& shared, PanelState& state, int key, char ch);

  bool dragging() const { return dragParam_ >= 0; }
  void markEdited(PanelState& state) const { state.presetDirty = true; }

  struct WidgetRect {
    uint32_t param = 0;
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
    double r = 0.0;
    const char* label = "";
    bool knob = false;
    bool toggle = false;
  };
  std::vector<WidgetRect> widgetRects() const;

private:
  enum class Kind {
    Knob,
    Button,
    Toggle,
    ModelTab,
    Power,
    Pill,
    Cycle,
    Fader,
    AB,
    Copy,
    Paste,
    Hold,
  };

  struct Widget {
    Kind kind = Kind::Knob;
    uint32_t param = 0;
    double value = 0.0;
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
    double r = 0.0;
    int steps = 0;
    double scaleRadius = 1.44;
    const char* label = "";
    std::vector<const char*> scale;
    bool labelBelow = false;
    bool onSilver = false;
    bool blackEq = false;
    std::vector<const char*> buttonText;
  };

  void layout(const Shared& shared, const PanelState& state);
  void drawHeader(cairo_t* cr, const Shared& shared,
                  const PanelState& state) const;
  void drawBody(cairo_t* cr, const Shared& shared,
                const PanelState& state) const;
  void drawWidget(cairo_t* cr, const Widget& w, const Shared& shared,
                  const PanelState& state) const;
  void drawKnob(cairo_t* cr, const Widget& w, double value) const;
  void drawButton(cairo_t* cr, const Widget& w, bool lit) const;
  void drawToggle(cairo_t* cr, const Widget& w, bool on) const;
  void drawPill(cairo_t* cr, const Widget& w, bool on) const;
  void drawFader(cairo_t* cr, const Widget& w, double value) const;
  void drawLevelMeter(cairo_t* cr, double x, double y, double w, double h,
                      double dbL, double dbR, double minDb, double maxDb,
                      bool labelsRight) const;
  void drawValueBubble(cairo_t* cr, uint32_t param, double value) const;
  void drawTextEntry(cairo_t* cr, const Shared& shared) const;
  void text(cairo_t* cr, const char* s, double x, double y, double size,
            int align, double r, double g, double b) const;

  void applyAB(Shared& shared, PanelState& state, int target) const;
  void copyAB(Shared& shared, PanelState& state) const;
  bool commitTextEntry(Shared& shared, PanelState& state);
  bool hitTextEntry(double x, double y) const;
  void pasteClipboard(Shared& shared, PanelState& state) const;

  double knobValue(const Shared& shared, const Widget& w) const;
  bool isLit(const Shared& shared, const Widget& w) const;
  bool hitWidget(const Widget& w, double x, double y) const;

  std::vector<Widget> widgets_;
  int dragParam_ = -1;
  bool dragFader_ = false;
  double dragStartY_ = 0.0;
  double dragStartValue_ = 0.0;
  int dragSteps_ = 0;

  double mouseX_ = 0.0;
  double mouseY_ = 0.0;
  std::chrono::steady_clock::time_point lastClickTime_{};
  uint32_t lastClickParam_ = 0xffffffffu;

  bool editing_ = false;
  uint32_t editParam_ = 0;
  std::string editText_;
  double editBoxX_ = 0.0;
  double editBoxY_ = 0.0;
  static constexpr double kEditW = 140.0;
  static constexpr double kEditH = 30.0;

  cairo_surface_t* grain_ = nullptr;
  void* ft_ = nullptr;
  void* ftFace_ = nullptr;
  cairo_font_face_t* font_ = nullptr;
  cairo_font_face_t* fallbackFont_ = nullptr;
};

} // namespace suite
