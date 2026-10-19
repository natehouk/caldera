// Caldera — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "ParamDefs.hpp"
#include "Presets.hpp"
#include "UserPresets.hpp"
#include "ResizeHandle.hpp"
#include <cctype>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

START_NAMESPACE_DISTRHO

using namespace caldera;
using DGL_NAMESPACE::ResizeHandle;
typedef std::complex<float> cplx;

static constexpr float W = 1100.f, H = 660.f;

#define C_BG        Color(0x0f, 0x10, 0x12)
#define C_PANEL     Color(0x18, 0x1a, 0x1d)
#define C_PANEL_HI  Color(0x22, 0x25, 0x29)
#define C_LINE      Color(0x2c, 0x30, 0x35)
#define C_TEXT      Color(0xe8, 0xe4, 0xdc)
#define C_DIM       Color(0x84, 0x88, 0x8e)
#define C_FAINT     Color(0x4a, 0x4e, 0x54)
#define C_F1        Color(0xff, 0xa8, 0x3e)
#define C_F2        Color(0x5f, 0xd0, 0xc0)
#define C_F1A(a)    Color(0xff, 0xa8, 0x3e, (a) / 255.f)
#define C_F2A(a)    Color(0x5f, 0xd0, 0xc0, (a) / 255.f)

// ---------------------------------------------------------------- layout
enum WType { Knob, Mini, Toggle, Segment, Stepper };
struct Ctl { WType type; uint32_t param; float x, y, r; const char* label; };    // Segment/Stepper: r = width
struct Panel { float x, y, w, h; const char* title; };

static constexpr float DX = 16, DY = 64, DW = 1068, DH = 236;     // filter display
static constexpr float FPY = 312, FPH = 158;                        // filter panels
static constexpr float BPY = 482, BPH = 162;                        // bottom panels

static const Panel kPanels[] = {
    { 16,  FPY, 530, FPH, "" },            // filter 1 (title drawn by its toggle)
    { 554, FPY, 530, FPH, "" },            // filter 2
    { 16,  BPY, 284, BPH, "GLOBAL" },
    { 308, BPY, 252, BPH, "LFO 1" },
    { 568, BPY, 252, BPH, "LFO 2" },
    { 828, BPY, 256, BPH, "ENVELOPE" },
};

static std::vector<Ctl> buildControls() {
    std::vector<Ctl> c;
    for (int f = 0; f < 2; ++f) {
        const float x = f == 0 ? 16.f : 554.f;
        const uint32_t b = f == 0 ? kF1On : kF2On;
        c.push_back({ Toggle,  b + 0, x + 34,  FPY + 22, 0,   f == 0 ? "FILTER 1" : "FILTER 2" });
        c.push_back({ Stepper, b + 1, x + 178, FPY + 22, 124, "" });
        c.push_back({ Segment, b + 2, x + 316, FPY + 22, 120, "" });
        c.push_back({ Segment, b + 3, x + 432, FPY + 22, 96,  "" });
        c.push_back({ Toggle,  b + 7, x + 504, FPY + 22, 0,   "KEY" });
        const uint32_t mains[3] = { b + 4, b + 5, b + 6 };
        const char* names[3] = { "FREQ", "PEAK", "PAN" };
        for (int k = 0; k < 3; ++k) {
            const float cx = x + 62 + k * 172;
            c.push_back({ Knob, mains[k], cx, FPY + 98, k == 0 ? 28.f : 22.f, names[k] });
            for (int s = 0; s < 3; ++s)
                c.push_back({ Mini, modParam(s, f * 3 + k), cx + 62, FPY + 66 + s * 31, 10, s == 0 ? "LFO1" : (s == 1 ? "LFO2" : "ENV") });
        }
    }
    // global
    c.push_back({ Segment, kRouting, 158, BPY + 46, 236, "" });
    c.push_back({ Knob, kInGain,  58,  BPY + 112, 18, "INPUT" });
    c.push_back({ Knob, kOutGain, 124, BPY + 112, 18, "OUTPUT" });
    c.push_back({ Knob, kMix,     190, BPY + 112, 18, "MIX" });
    c.push_back({ Toggle, kAutoMute, 254, BPY + 112, 0, "AUTO MUTE" });
    // LFOs
    for (int l = 0; l < 2; ++l) {
        const float x = l == 0 ? 308.f : 568.f;
        const uint32_t b = l == 0 ? kL1Wave : kL2Wave;
        c.push_back({ Segment, b + 0, x + 50,  BPY + 80, 76,  "" });
        c.push_back({ Segment, b + 2, x + 172, BPY + 80, 140, "" });
        c.push_back({ Knob, b + 3, x + 46,  BPY + 128, 17, "RATE" });
        c.push_back({ Knob, b + 1, x + 112, BPY + 128, 15, "BALANCE" });
        c.push_back({ Toggle, b + 5, x + 196, BPY + 128, 0, "RETRIG" });
    }
    // envelope
    const char* eg[4] = { "A", "D", "S", "R" };
    for (int i = 0; i < 4; ++i) c.push_back({ Knob, kEgAttack + i, 858 + i * 44.f, BPY + 128, 14, eg[i] });
    c.push_back({ Knob, kEgThreshold, 1040, BPY + 128, 14, "THRESH" });
    c.push_back({ Toggle, kEgMidi, 1046, BPY + 80, 0, "MIDI TRIG" });
    return c;
}
static const std::vector<Ctl>& controls() { static std::vector<Ctl> c = buildControls(); return c; }

static const char* const* segLabels(uint32_t p, int& n) {
    static const char* const type[]  = { "LP", "BP", "HP" };
    static const char* const slope[] = { "12", "24", "48" };
    static const char* const route[] = { "SERIAL", "PARALLEL" };
    static const char* const wave[]  = { "TRI", "SQR" };
    static const char* const sync[]  = { "FREE", "STR", "DOT", "TRIP" };
    switch (p) {
    case kF1Type: case kF2Type: n = 3; return type;
    case kF1Slope: case kF2Slope: n = 3; return slope;
    case kRouting: n = 2; return route;
    case kL1Wave: case kL2Wave: n = 2; return wave;
    case kL1Sync: case kL2Sync: n = 4; return sync;
    default: n = 0; return nullptr;
    }
}

static constexpr float PS_X = 414.f, PS_Y = 20.f, PS_W = 272.f, PS_H = 28.f, PS_ARROW = 30.f;
static constexpr float LEARN_X = 712.f, LEARN_W = 72.f, MIDI_X = 792.f, MIDI_W = 56.f;
static constexpr float PM_COLW = 146.f, PM_ROW = 23.f, PM_HEAD = 22.f, PM_PAD = 8.f;

// filter magnitude for drawing (linear prototype of the engine's cascade)
static cplx filterH(const float* v, int f, float hz, float panOct) {
    const uint32_t b = f == 0 ? kF1On : kF2On;
    const int type = (int)(v[b + 2] + .5f), sl = (int)(v[b + 3] + .5f);
    const int stages = sl == 0 ? 1 : (sl == 1 ? 2 : 4);
    const float fc = v[b + 4] * std::pow(2.f, panOct);
    const float r = std::pow(clampf(v[b + 5], 0.f, 1.f), 0.6f);
    const StyleDef& sd = styleDef((int)(v[b + 1] + .5f));
    const float* kb = bwK(stages);
    const float w = hz / fc;
    cplx h(1, 0);
    for (int i = 0; i < stages; ++i) {
        float k = i == stages - 1 ? kb[i] * (1 - r) + sd.kMin * r : kb[i];
        k = std::max(k, 0.035f);
        const cplx den(1 - w * w, k * w);
        if (type == 0) h *= cplx(1, 0) / den;
        else if (type == 2) h *= cplx(-w * w, 0) / den;
        else h *= cplx(0, w * std::max(k, 0.12f)) / den;
    }
    if (sd.bassLoss > 0) h *= 1.f - sd.bassLoss * r;
    return h;
}

static Color withAlpha(const Color& c, float a) { Color r(c); r.alpha = a; return r; }

class CalderaUI : public UI {
public:
    CalderaUI() : UI((uint)W, (uint)H), resizeHandle(this) {
        loadSharedResources();
        Engine def;
        for (uint32_t i = 0; i < kParamCount; ++i) { val[i] = def.p[i]; defv[i] = def.p[i]; }
        val[kOutF1Live] = 1000; val[kOutF2Live] = 200;
        for (int& m : paramCC) m = -1;
        const double sf = getScaleFactor();
        if (sf != 1.0) setSize((uint)(W * sf), (uint)(H * sf));
        setGeometryConstraints((uint)(W * 0.7 * sf), (uint)(H * 0.7 * sf), true);
        if (isResizable()) resizeHandle.hide();
        userPresets = listUserPresets();
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }

    // ---- values -------------------------------------------------------------
    void setValue(uint32_t p, float v) {
        const ParamDef& d = paramDef(p);
        v = clampf(v, d.min, d.max);
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }
    void commit(uint32_t p, float v) { editParameter(p, true); setValue(p, v); editParameter(p, false); }

    void parameterChanged(uint32_t i, float v) override {
        if (i >= kParamCount) return;
        if (i == kOutLearn) {
            if (v != lastLearnEcho && v > 0) {
                const int code = (int)v % 10000 - 1, prm = code / 128, cc = code % 128;
                for (int& m : paramCC) if (m == cc) m = -1;
                if (prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
                learnWaiting = -1;
            }
            lastLearnEcho = v; repaint(); return;
        }
        if (val[i] != v) { val[i] = v; repaint(); }
    }
    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "midimap") != 0) return;
        for (int& m : paramCC) m = -1;
        const char* q = value;
        while (q && *q) {
            int cc = -1, prm = -1;
            if (std::sscanf(q, "%d:%d", &cc, &prm) == 2 && prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
            const char* s = std::strchr(q, ';'); if (!s) break; q = s + 1;
        }
        repaint();
    }

    uint32_t effParam(const Ctl& c) const {
        if (c.param == kL1Rate && val[kL1Sync] > 0.5f) return kL1Div;
        if (c.param == kL2Rate && val[kL2Sync] > 0.5f) return kL2Div;
        return c.param;
    }
    static int filterOf(uint32_t p) {
        if (p >= kF1On && p <= kF1Midi) return 0;
        if (p >= kF2On && p <= kF2Midi) return 1;
        if (p >= kModFirst && p <= kModLast) return ((p - kModFirst) % kNumDests) / 3;
        return -1;
    }
    bool dimmed(uint32_t p) const {
        const int f = filterOf(p);
        if (f >= 0 && p != (f == 0 ? kF1On : kF2On) && val[f == 0 ? kF1On : kF2On] < 0.5f) return true;
        if (p == kEgThreshold && val[kEgMidi] > 0.5f) return true;
        return false;
    }

    // ---- formatting ---------------------------------------------------------
    void fmt(uint32_t p, char* b, size_t n) const {
        const float v = val[p];
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        const ParamDef& d = paramDef(p);
        if (d.hints & kHintBoolean) { snprintf(b, n, "%s", v > 0.5f ? "On" : "Off"); return; }
        if (p == kF1Freq || p == kF2Freq) { if (v >= 1000) snprintf(b, n, "%.2f kHz", v / 1000); else snprintf(b, n, "%.0f Hz", v); return; }
        if (p == kF1Pan || p == kF2Pan) {
            if (std::fabs(v) < 0.005f) snprintf(b, n, "Center"); else snprintf(b, n, "%s %.0f%%", v < 0 ? "L" : "R", std::fabs(v) * 100); return; }
        if (p == kInGain || p == kOutGain || p == kEgThreshold) { snprintf(b, n, "%+.1f dB", v); return; }
        if (p == kL1Rate || p == kL2Rate) { snprintf(b, n, v < 1 ? "%.2f Hz" : "%.1f Hz", v); return; }
        if (isTimeParam(p)) { if (v < 1) snprintf(b, n, "%.0f ms", v * 1000); else snprintf(b, n, "%.2f s", v); return; }
        if (isModParam(p)) { snprintf(b, n, "%+.0f%%", v * 100); return; }
        snprintf(b, n, "%.0f%%", v * 100);
    }
    bool parse(uint32_t p, const std::string& in, float& out) const {
        std::string s; for (char ch : in) if (!std::isspace((unsigned char)ch)) s += (char)std::tolower((unsigned char)ch);
        if (s.empty()) return false;
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) {
            for (int i = 0; i < ne; ++i) {
                std::string lab; for (const char* c = l[i]; *c; ++c) if (*c != ' ') lab += (char)std::tolower((unsigned char)*c);
                if (lab.compare(0, s.size(), s) == 0) { out = (float)i; return true; }
            }
            return false;
        }
        if (p == kF1Pan || p == kF2Pan) {
            if (s[0] == 'c') { out = 0; return true; }
            const float sign = s[0] == 'l' ? -1.f : 1.f;
            const char* q = (s[0] == 'l' || s[0] == 'r') ? s.c_str() + 1 : s.c_str();
            out = sign * (float)std::atof(q) / 100.f; return true;
        }
        char* end = nullptr; double x = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return false;
        std::string suf(end);
        if (suf == "k" || suf == "khz") x *= 1000;
        if (p == kF1Freq || p == kF2Freq || p == kL1Rate || p == kL2Rate || p == kInGain || p == kOutGain || p == kEgThreshold) { out = (float)x; return true; }
        if (isTimeParam(p)) { out = (float)(suf == "ms" ? x / 1000 : x); return true; }
        out = (float)(x / 100.0); return true;
    }

    // ---- drawing ------------------------------------------------------------
    void onNanoDisplay() override {
        const float s = uiScale();
        save(); scale(s, s);
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();
        fontFace(NANOVG_DEJAVU_SANS_TTF);

        // header
        beginPath(); moveTo(20, 47); lineTo(30, 21); lineTo(40, 47); closePath(); fillColor(C_F1); fill();
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(52, 34, "CALDERA", nullptr);
        fontSize(10); textLetterSpacing(2.5f); fillColor(C_DIM); text(232, 35, "STEREO DUAL FILTER", nullptr);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); text(W - 22, 35, "EPSILON AUDIO  ·  1.0", nullptr);
        drawPresetBar();
        drawHeaderButton(LEARN_X, LEARN_W, learnMode ? "LEARNING" : "LEARN", learnMode, hoverHeader == 1);
        drawHeaderButton(MIDI_X, MIDI_W, "MIDI", val[kMidiOn] > 0.5f, hoverHeader == 2);

        drawDisplay();
        for (const Panel& p : kPanels) drawPanel(p);
        drawLfoWave(318, BPY + 28, 232, 30, 0);
        drawLfoWave(578, BPY + 28, 232, 30, 1);
        drawEnvShape(838, BPY + 28, 160, 30);
        // envelope activity light
        const float eg = val[kOutEg];
        beginPath(); circle(1060, BPY + 43, 5); fillColor(eg > 0.02f ? Color(0xff, 0xa8, 0x3e, 0.25f + 0.75f * std::min(1.f, eg)) : C_PANEL_HI); fill();
        strokeColor(C_LINE); strokeWidth(1); stroke();

        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) drawCtl(cs[i], i == active || i == hover);
        if (editing >= 0) drawEditBox();
        drawStatus();
        if (menuOpen) drawPresetMenu();
        if (saving) drawSaveBox();
        restore();
    }

    void drawHeaderButton(float x, float w, const char* label, bool on, bool hot) {
        beginPath(); roundedRect(x, PS_Y, w, PS_H, 5);
        fillColor(on ? C_F1A(learnMode && label[0] == 'L' ? 70 : 40) : C_PANEL); fill();
        strokeColor(on ? C_F1 : (hot ? C_DIM : C_LINE)); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fillColor(on ? C_TEXT : C_DIM); text(x + w / 2, PS_Y + PS_H / 2 + 0.5f, label, nullptr);
    }

    void drawPanel(const Panel& p) {
        beginPath(); roundedRect(p.x, p.y, p.w, p.h, 6); fillColor(C_PANEL); fill();
        strokeColor(C_LINE); strokeWidth(1); stroke();
        if (p.title[0]) {
            fontSize(9.5f); textLetterSpacing(2.2f); fillColor(C_DIM);
            textAlign(ALIGN_LEFT | ALIGN_TOP); text(p.x + 14, p.y + 10, p.title, nullptr);
        }
    }

    // display mapping
    static float xOfHz(float hz) { return DX + 10 + (DW - 20) * std::log(hz / 20.f) / std::log(1000.f); }
    static float hzOfX(float x)  { return 20.f * std::pow(1000.f, (x - DX - 10) / (DW - 20)); }
    static float yOfDb(float db) { return DY + 8 + (DH - 16) * (1 - (db + 36.f) / 60.f); }
    static float yOfPeak(float pk) { return DY + DH * 0.86f - pk * DH * 0.72f; }
    static float peakOfY(float y) { return (DY + DH * 0.86f - y) / (DH * 0.72f); }

    void drawDisplay() {
        beginPath(); roundedRect(DX, DY, DW, DH, 6); fillColor(Color(0x0b, 0x0c, 0x0d)); fill();
        strokeColor(C_LINE); strokeWidth(1); stroke();
        // grid
        fontSize(8); textLetterSpacing(1); fillColor(C_FAINT); textAlign(ALIGN_CENTER | ALIGN_BOTTOM);
        const float grid[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
        const char* gl[] = { "50", "100", "200", "500", "1k", "2k", "5k", "10k" };
        beginPath();
        for (float g : grid) { const float x = xOfHz(g); moveTo(x, DY + 4); lineTo(x, DY + DH - 4); }
        for (int db = -24; db <= 12; db += 12) { const float y = yOfDb((float)db); moveTo(DX + 4, y); lineTo(DX + DW - 4, y); }
        strokeColor(Color(255, 255, 255, 8 / 255.f)); stroke();
        for (int i = 0; i < 8; ++i) text(xOfHz(grid[i]), DY + DH - 4, gl[i], nullptr);

        const int N = 300;
        std::vector<float> hz(N + 1);
        for (int i = 0; i <= N; ++i) hz[i] = hzOfX(DX + 10 + (DW - 20) * i / N);
        const bool on[2] = { val[kF1On] > 0.5f, val[kF2On] > 0.5f };
        // individual filters (with stereo pan shown as faint L/R curves)
        for (int f = 1; f >= 0; --f) {
            const uint32_t b = f == 0 ? kF1On : kF2On;
            const Color col = f == 0 ? C_F1 : C_F2;
            const float pan = val[b + 6];
            const float alpha = on[f] ? 1.f : 0.25f;
            if (std::fabs(pan) > 0.01f && on[f]) {
                for (int side = -1; side <= 1; side += 2) {
                    beginPath();
                    for (int i = 0; i <= N; ++i) {
                        const float db = 20 * std::log10(std::max(std::abs(filterH(val, f, hz[i], side * 2.f * pan)), 1e-4f));
                        const float y = clampf(yOfDb(db), DY + 2, DY + DH - 2), x = DX + 10 + (DW - 20) * i / N;
                        if (!i) moveTo(x, y); else lineTo(x, y);
                    }
                    strokeColor(withAlpha(col, 0.22f)); strokeWidth(1.f); stroke();
                }
            }
            beginPath();
            std::vector<float> ys(N + 1);
            for (int i = 0; i <= N; ++i) {
                const float db = 20 * std::log10(std::max(std::abs(filterH(val, f, hz[i], 0)), 1e-4f));
                ys[i] = clampf(yOfDb(db), DY + 2, DY + DH - 2);
            }
            moveTo(DX + 10, DY + DH - 2);
            for (int i = 0; i <= N; ++i) lineTo(DX + 10 + (DW - 20) * i / N, ys[i]);
            lineTo(DX + DW - 10, DY + DH - 2); closePath();
            fillPaint(linearGradient(0, DY, 0, DY + DH, withAlpha(col, 0.16f * alpha), withAlpha(col, 0.f))); fill();
            beginPath();
            for (int i = 0; i <= N; ++i) { const float x = DX + 10 + (DW - 20) * i / N; if (!i) moveTo(x, ys[i]); else lineTo(x, ys[i]); }
            strokeColor(withAlpha(col, 0.9f * alpha)); strokeWidth(1.6f); stroke();
        }
        // combined response
        if (on[0] && on[1]) {
            const bool par = val[kRouting] > 0.5f;
            beginPath();
            for (int i = 0; i <= N; ++i) {
                const cplx h1 = filterH(val, 0, hz[i], 0), h2 = filterH(val, 1, hz[i], 0);
                const float mag = std::abs(par ? 0.7071f * (h1 + h2) : h1 * h2);
                const float y = clampf(yOfDb(20 * std::log10(std::max(mag, 1e-4f))), DY + 2, DY + DH - 2), x = DX + 10 + (DW - 20) * i / N;
                if (!i) moveTo(x, y); else lineTo(x, y);
            }
            strokeColor(Color(0xe8, 0xe4, 0xdc, 0.7f)); strokeWidth(1.2f); stroke();
        }
        // live (modulated) cutoff markers
        for (int f = 0; f < 2; ++f) {
            if (!on[f]) continue;
            const float lx = xOfHz(clampf(val[f == 0 ? kOutF1Live : kOutF2Live], 20.f, 20000.f));
            beginPath(); moveTo(lx, DY + 6); lineTo(lx, DY + DH - 14);
            strokeColor(f == 0 ? C_F1A(60) : C_F2A(60)); strokeWidth(1); stroke();
        }
        // handles: filter 1, filter 2, link (center)
        for (int h = 0; h < 3; ++h) {
            float hx, hy; handlePos(h, hx, hy);
            const bool hot = dragHandle == h || hoverHandle == h;
            if (h < 2) {
                const Color col = h == 0 ? C_F1 : C_F2;
                beginPath(); circle(hx, hy, hot ? 11 : 9); fillColor(withAlpha(col, on[h] ? 0.95f : 0.3f)); fill();
                fontSize(10); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_BG);
                text(hx, hy + 0.5f, h == 0 ? "1" : "2", nullptr);
            } else if (on[0] && on[1]) {
                beginPath(); circle(hx, hy, hot ? 7 : 5.5f); fillColor(C_PANEL_HI); fill();
                strokeColor(C_TEXT); strokeWidth(1.2f); stroke();
            }
        }
    }
    void handlePos(int h, float& x, float& y) const {
        if (h < 2) {
            const uint32_t b = h == 0 ? kF1On : kF2On;
            x = xOfHz(val[b + 4]); y = yOfPeak(val[b + 5]); return;
        }
        float x1, y1, x2, y2; handlePos(0, x1, y1); handlePos(1, x2, y2);
        x = (x1 + x2) / 2; y = (y1 + y2) / 2;
    }

    void drawLfoWave(float x, float y, float w, float h, int l) {
        const uint32_t b = l == 0 ? kL1Wave : kL2Wave;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(Color(0x0b, 0x0c, 0x0d)); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        const int wave = (int)(val[b] + .5f);
        beginPath();
        for (int i = 0; i <= 160; ++i) {
            const float t = (float)i / 160, ph = std::fmod(t * 2.f, 1.f);
            const float py = y + h / 2 - LFO::shape(ph, wave, val[b + 1]) * h * 0.34f;
            if (!i) moveTo(x + 4 + (w - 8) * t, py); else lineTo(x + 4 + (w - 8) * t, py);
        }
        strokeColor(C_F1); strokeWidth(1.4f); stroke();
        char buf[24]; fmt(val[b + 2] > 0.5f ? b + 4 : b + 3, buf, sizeof buf);
        fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_DIM); text(x + w - 5, y + 3, buf, nullptr);
    }
    void drawEnvShape(float x, float y, float w, float h) {
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(Color(0x0b, 0x0c, 0x0d)); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        const float a = val[kEgAttack], d = val[kEgDecay], s = val[kEgSustain], r = val[kEgRelease], hold = 0.4f;
        const float tot = std::max(a + d + hold + r, 0.3f), sx = (w - 10) / tot, x0 = x + 5, yb = y + h - 4, hh = h - 8;
        beginPath(); moveTo(x0, yb); lineTo(x0 + a * sx, yb - hh);
        for (int i = 1; i <= 16; ++i) { const float t = i / 16.f; lineTo(x0 + (a + d * t) * sx, yb - hh * (s + (1 - s) * std::exp(-4 * t))); }
        lineTo(x0 + (a + d + hold) * sx, yb - hh * s);
        for (int i = 1; i <= 16; ++i) { const float t = i / 16.f; lineTo(x0 + (a + d + hold + r * t) * sx, yb - hh * s * std::exp(-4 * t)); }
        strokeColor(C_F1); strokeWidth(1.4f); stroke();
    }

    void drawKnobBody(float x, float y, float r, float n, bool bipolar, bool hot, Color col, bool dim) {
        const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, ang = a0 + (a1 - a0) * n;
        beginPath(); arc(x, y, r, a0, a1, CW); strokeColor(C_LINE); strokeWidth(r > 20 ? 4.f : (r > 12 ? 3.f : 2.5f)); lineCap(ROUND); stroke();
        const float from = bipolar ? (a0 + a1) / 2 : a0;
        if (std::fabs(ang - from) > 0.001f) {
            beginPath(); arc(x, y, r, std::min(from, ang), std::max(from, ang), CW); strokeColor(dim ? C_FAINT : col); stroke();
        }
        const float cr = r - (r > 20 ? 8 : (r > 12 ? 6 : 4));
        beginPath(); circle(x, y, cr);
        fillPaint(radialGradient(x - cr * 0.3f, y - cr * 0.4f, 0, cr * 1.4f, hot ? Color(0x3a, 0x3e, 0x44) : C_PANEL_HI, Color(0x14, 0x15, 0x17)));
        fill(); strokeColor(Color(255, 255, 255, 18 / 255.f)); strokeWidth(1); stroke();
        beginPath();
        moveTo(x + std::cos(ang) * cr * 0.3f, y + std::sin(ang) * cr * 0.3f);
        lineTo(x + std::cos(ang) * cr * 0.9f, y + std::sin(ang) * cr * 0.9f);
        strokeColor(dim ? C_DIM : C_TEXT); strokeWidth(r > 12 ? 2.f : 1.5f); lineCap(ROUND); stroke();
    }

    void drawCtl(const Ctl& c, bool hot) {
        const uint32_t p = effParam(c);
        const bool dim = dimmed(c.param);
        const int f = filterOf(c.param);
        const Color col = f == 1 ? C_F2 : C_F1;
        char buf[40];
        if (c.type == Knob || c.type == Mini) {
            const bool bip = paramDef(p).min < 0 && p != kInGain && p != kOutGain && p != kEgThreshold;
            drawKnobBody(c.x, c.y, c.r, toNorm(p, val[p]), bip, hot, col, dim);
            if (c.type == Knob) {
                fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_TOP);
                if (hot && editing < 0 && !learnMode) { fmt(p, buf, sizeof buf); fillColor(col); text(c.x, c.y + c.r + 6, buf, nullptr); }
                else { fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + c.r + 6, c.label, nullptr); }
            } else {
                fontSize(7.5f); textLetterSpacing(1.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
                fillColor(dim ? C_FAINT : C_DIM); text(c.x + c.r + 5, c.y + 0.5f, c.label, nullptr);
            }
        } else if (c.type == Toggle) {
            const bool on = val[p] > 0.5f;
            const float w = 34, h = 18, x = c.x - w / 2, y = c.y - h / 2;
            const bool isFilterOn = p == kF1On || p == kF2On;
            beginPath(); roundedRect(x, y, w, h, h / 2); fillColor(on && !dim ? withAlpha(col, 0.25f) : C_PANEL_HI); fill();
            strokeColor(on && !dim ? col : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(on ? x + w - h / 2 : x + h / 2, c.y, 6); fillColor(dim ? C_FAINT : (on ? col : C_DIM)); fill();
            fontSize(isFilterOn ? 10.f : 8.5f); textLetterSpacing(isFilterOn ? 2.2f : 1.5f);
            if (isFilterOn) { textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(c.x + w / 2 + 8, c.y + 0.5f, c.label, nullptr); }
            else { textAlign(ALIGN_CENTER | ALIGN_TOP); fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + 14, c.label, nullptr); }
        } else if (c.type == Segment) {
            int n = 0; const char* const* names = segLabels(p, n);
            const int cur = (int)(val[p] + .5f);
            const float sw = c.r / n, sh = 20, x0 = c.x - c.r / 2;
            for (int i = 0; i < n; ++i) {
                const float x = x0 + sw * i;
                beginPath(); roundedRect(x + 1, c.y - sh / 2, sw - 2, sh, 4);
                fillColor(i == cur ? (dim ? C_FAINT : col) : C_PANEL_HI); fill();
                fontSize(8.5f); textLetterSpacing(1.4f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                fillColor(i == cur ? C_BG : (dim ? C_FAINT : C_DIM)); text(x + sw / 2, c.y + 0.5f, names[i], nullptr);
            }
        } else {   // Stepper (filter style)
            const float x = c.x - c.r / 2, y = c.y - 10, w = c.r, h = 20;
            beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_PANEL_HI); fill(); strokeColor(hot ? C_DIM : C_LINE); strokeWidth(1); stroke();
            for (int side = 0; side < 2; ++side) {
                const float ax = side == 0 ? x + 10 : x + w - 10, d = side == 0 ? -1.f : 1.f;
                beginPath(); moveTo(ax - 2.5f * d, c.y - 4); lineTo(ax + 2.5f * d, c.y); lineTo(ax - 2.5f * d, c.y + 4); closePath();
                fillColor(dim ? C_FAINT : C_DIM); fill();
            }
            fmt(p, buf, sizeof buf);
            fontSize(9.5f); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(dim ? C_FAINT : col);
            std::string up(buf); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
            text(c.x, c.y + 0.5f, up.c_str(), nullptr);
        }
        // MIDI learn overlays
        if (learnMode && (c.type == Knob || c.type == Mini || c.type == Toggle || c.type == Segment || c.type == Stepper)) {
            const int cc = paramCC[c.param];
            const bool waiting = learnWaiting == (int)c.param;
            if (cc >= 0 || waiting) {
                const float bx = c.type == Knob || c.type == Mini ? c.x : c.x, by = c.type == Knob ? c.y - c.r - 12 : c.y - 18;
                snprintf(buf, sizeof buf, waiting ? "?" : "CC%d", cc);
                fontSize(8); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                const float bw = waiting ? 14 : 30;
                beginPath(); roundedRect(bx - bw / 2, by - 6, bw, 12, 3); fillColor(waiting ? C_F1 : C_PANEL_HI); fill();
                strokeColor(C_F1); strokeWidth(1); stroke();
                fillColor(waiting ? C_BG : C_F1); text(bx, by + 0.5f, buf, nullptr);
            }
        }
    }

    void drawStatus() {
        std::string msg;
        char vb[40];
        if (learnMode) {
            msg = learnWaiting >= 0 ? std::string("Move a MIDI controller to assign: ") + paramDef(learnWaiting).name
                                    : "MIDI Learn: click a control, then move a controller.  Right-click a control to unassign.  ";
        } else if (hover >= 0) {
            const Ctl& c = controls()[hover]; const uint32_t p = effParam(c);
            fmt(p, vb, sizeof vb); msg = std::string(paramDef(p).name) + "   " + vb;
        } else if (hoverHandle >= 0 || dragHandle >= 0) {
            const int h = dragHandle >= 0 ? dragHandle : hoverHandle;
            if (h < 2) {
                const uint32_t b = h == 0 ? kF1On : kF2On; char f1[24], pk[24]; fmt(b + 4, f1, sizeof f1); fmt(b + 5, pk, sizeof pk);
                msg = std::string(h == 0 ? "Filter 1" : "Filter 2") + "   " + f1 + "   peak " + pk + "      drag: freq / peak    Cmd/Ctrl-drag: move both in opposite directions";
            } else msg = "Drag to move both filters together";
        }
        if (msg.empty()) return;
        fontSize(9.5f); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(learnMode ? C_F1 : C_DIM);
        text(DX + 14, DY + 16, msg.c_str(), nullptr);
        if (learnMode && learnWaiting < 0) {
            textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(hoverClear ? C_TEXT : C_DIM);
            text(DX + DW - 14, DY + 16, "CLEAR ALL ASSIGNMENTS", nullptr);
        }
    }

    void drawEditBox() {
        const Ctl& c = controls()[editing];
        const float w = 84, h = 20, x = c.x - w / 2, y = c.y + c.r + 3;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(Color(0x0b, 0x0c, 0x0d)); fill(); strokeColor(C_F1); strokeWidth(1); stroke();
        fontSize(11); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT);
        text(c.x, y + h / 2 + 0.5f, (editBuf + "|").c_str(), nullptr);
    }

    // ---- presets ------------------------------------------------------------
    bool matchesValues(const float* v) const {
        for (uint32_t i = 0; i < kParamCount; ++i) {
            if (paramDef(i).hints & kHintOutput) continue;
            if (std::fabs(val[i] - v[i]) > 1e-4f * std::max(1.f, std::fabs(v[i]))) return false;
        }
        return true;
    }
    void applyValues(const float* v) {
        for (uint32_t i = 0; i < kParamCount; ++i)
            if (!(paramDef(i).hints & kHintOutput) && val[i] != v[i]) commit(i, v[i]);
    }
    void applyFactory(int idx) {
        idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets;
        applyValues(presets()[idx].v);
        curFactory = idx; curUser.clear(); repaint();
    }
    void applyUser(const std::string& name) {
        float v[kParamCount];
        if (!loadUserPreset(name, v)) return;
        applyValues(v);
        std::copy(v, v + kParamCount, userVals); curUser = name; curFactory = -1; repaint();
    }
    void programLoaded(uint32_t index) override { curFactory = (int)index; curUser.clear(); repaint(); }
    void step(int d) {
        // step through factory presets, then user presets, as one list
        const int nf = (int)kNumPresets, nu = (int)userPresets.size(), total = nf + nu;
        int cur = curFactory;
        if (!curUser.empty()) for (int i = 0; i < nu; ++i) if (userPresets[i] == curUser) cur = nf + i;
        if (cur < 0) cur = 0;
        const int next = ((cur + d) % total + total) % total;
        if (next < nf) applyFactory(next); else applyUser(userPresets[next - nf]);
    }

    void drawPresetBar() {
        const float x = PS_X, y = PS_Y, w = PS_W, h = PS_H, a = PS_ARROW;
        beginPath(); roundedRect(x, y, w, h, 5); fillColor(C_PANEL); fill();
        strokeColor(menuOpen ? C_F1 : C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(x + a, y + 5); lineTo(x + a, y + h - 5); moveTo(x + w - a, y + 5); lineTo(x + w - a, y + h - 5); strokeColor(C_LINE); stroke();
        for (int side = 0; side < 2; ++side) {
            const bool hot = presetHover == (side == 0 ? 0 : 2);
            const float cx = side == 0 ? x + a / 2 : x + w - a / 2, cy = y + h / 2, d = side == 0 ? -1.f : 1.f;
            beginPath(); moveTo(cx - 3 * d, cy - 5); lineTo(cx + 3 * d, cy); lineTo(cx - 3 * d, cy + 5); closePath();
            fillColor(hot ? C_F1 : C_DIM); fill();
        }
        std::string name; bool modified = false;
        if (!curUser.empty()) { name = curUser; modified = !matchesValues(userVals); }
        else if (curFactory >= 0) { name = presets()[curFactory].name; modified = !matchesValues(presets()[curFactory].v); }
        else name = "Custom";
        if (modified) name += " *";
        fontSize(12); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fillColor(presetHover == 1 || menuOpen ? C_TEXT : Color(0xd0, 0xcc, 0xc4));
        text(x + w / 2 - 6, y + h / 2 + 0.5f, name.c_str(), nullptr);
        const float cx = x + w - a - 14, cy = y + h / 2;
        beginPath(); moveTo(cx - 4, cy - 2); lineTo(cx + 4, cy - 2); lineTo(cx, cy + 3); closePath(); fillColor(C_DIM); fill();
    }

    struct MRect { float x, y, w, h; int kind; int idx; };   // kind: 0 factory, 1 user, 2 save button
    void menuLayout(float& mx, float& my, float& mw, float& mh, std::vector<MRect>& items) const {
        const int cols = kNumCategories + 1;
        mw = PM_COLW * cols + 2 * PM_PAD;
        mx = std::min(W - mw - 8, std::max(8.f, PS_X + PS_W / 2 - mw / 2));
        my = PS_Y + PS_H + 4;
        items.clear();
        const float top = my + PM_PAD + PM_ROW + 6 + PM_HEAD;
        int rows[kNumCategories + 1] = {}; int maxRows = 0;
        for (uint32_t i = 0; i < kNumPresets; ++i) {
            const char* cat = presets()[i].category;
            if (std::strcmp(cat, "Init") == 0) { items.push_back({ mx + PM_PAD, my + PM_PAD, PM_COLW - 4, PM_ROW, 0, (int)i }); continue; }
            for (int c = 0; c < kNumCategories; ++c)
                if (std::strcmp(cat, kCategories[c]) == 0) {
                    items.push_back({ mx + PM_PAD + c * PM_COLW, top + rows[c] * PM_ROW, PM_COLW - 4, PM_ROW, 0, (int)i });
                    maxRows = std::max(maxRows, ++rows[c]);
                }
        }
        for (int u = 0; u < (int)userPresets.size() && u < 14; ++u)
            items.push_back({ mx + PM_PAD + kNumCategories * PM_COLW, top + u * PM_ROW, PM_COLW - 4, PM_ROW, 1, u });
        maxRows = std::max(maxRows, std::min(14, (int)userPresets.size()));
        maxRows = std::max(maxRows, 1);
        items.push_back({ mx + mw - PM_PAD - 150, my + PM_PAD, 150, PM_ROW, 2, 0 });
        mh = PM_PAD + PM_ROW + 6 + PM_HEAD + maxRows * PM_ROW + PM_PAD;
    }
    void drawPresetMenu() {
        float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items);
        beginPath(); rect(mx - 12, my - 4, mw + 24, mh + 20);
        fillPaint(boxGradient(mx, my + 2, mw, mh, 6, 14, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(mx, my, mw, mh, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + PM_PAD, my + PM_PAD + PM_ROW + 3); lineTo(mx + mw - PM_PAD, my + PM_PAD + PM_ROW + 3); strokeColor(C_LINE); stroke();
        fontSize(8.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        for (int c = 0; c <= kNumCategories; ++c) {
            std::string up = c < kNumCategories ? kCategories[c] : "User";
            for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
            fillColor(c < kNumCategories ? C_F1 : C_F2);
            text(mx + PM_PAD + c * PM_COLW + 12, my + PM_PAD + PM_ROW + 6 + PM_HEAD / 2, up.c_str(), nullptr);
        }
        if (userPresets.empty()) {
            fontSize(10); textLetterSpacing(0.4f); fillColor(C_FAINT);
            text(mx + PM_PAD + kNumCategories * PM_COLW + 12, my + PM_PAD + PM_ROW + 6 + PM_HEAD + PM_ROW / 2, "(none saved yet)", nullptr);
        }
        for (int i = 0; i < (int)items.size(); ++i) {
            const MRect& r = items[i];
            const bool hot = i == menuHover;
            if (r.kind == 2) {
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(hot ? C_F2A(60) : C_PANEL); fill(); strokeColor(C_F2); strokeWidth(1); stroke();
                fontSize(9.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT);
                text(r.x + r.w / 2, r.y + r.h / 2 + 0.5f, "SAVE CURRENT AS...", nullptr);
                continue;
            }
            const bool isCur = r.kind == 0 ? (curUser.empty() && r.idx == curFactory) : (curUser == userPresets[r.idx]);
            const bool del = r.kind == 1 && pendingDelete == r.idx;
            if (hot || del) { beginPath(); roundedRect(r.x, r.y, r.w, r.h, 4); fillColor(del ? Color(0xd0, 0x50, 0x40, 0.35f) : C_F1A(40)); fill(); }
            if (isCur) { beginPath(); circle(r.x + 6, r.y + r.h / 2, 2.5f); fillColor(r.kind == 0 ? C_F1 : C_F2); fill(); }
            fontSize(11); textLetterSpacing(0.5f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            fillColor(hot || isCur || del ? C_TEXT : C_DIM);
            const std::string label = del ? "Delete? click again" : (r.kind == 0 ? std::string(presets()[r.idx].name) : userPresets[r.idx]);
            text(r.x + 12, r.y + r.h / 2 + 0.5f, label.c_str(), nullptr);
        }
    }
    int menuHit(float x, float y) const {
        float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items);
        for (int i = 0; i < (int)items.size(); ++i) { const MRect& r = items[i]; if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) return i; }
        return -1;
    }
    MRect menuItem(int i) const { float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items); return items[i]; }

    void drawSaveBox() {
        const float w = 360, h = 96, x = W / 2 - w / 2, y = 120;
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.45f)); fill();
        beginPath(); roundedRect(x, y, w, h, 8); fillColor(C_PANEL_HI); fill(); strokeColor(C_F2); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM);
        text(x + 16, y + 14, "SAVE PRESET AS", nullptr);
        beginPath(); roundedRect(x + 16, y + 36, w - 32, 26, 4); fillColor(Color(0x0b, 0x0c, 0x0d)); fill(); strokeColor(C_LINE); stroke();
        fontSize(13); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT);
        text(x + 26, y + 49.5f, (saveName + "|").c_str(), nullptr);
        fontSize(8.5f); textLetterSpacing(1.2f); fillColor(C_FAINT); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        text(x + w - 16, y + 72, saveError ? "COULD NOT SAVE - CHECK THE NAME" : "ENTER TO SAVE   ·   ESC TO CANCEL", nullptr);
    }

    // ---- interaction ----------------------------------------------------------
    int hit(float x, float y, int* seg = nullptr) const {
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) {
            const Ctl& c = cs[i];
            switch (c.type) {
            case Knob: case Mini: { const float rr = c.r + (c.type == Mini ? 4 : 6); if ((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y) <= rr * rr) return i; break; }
            case Toggle: if (std::fabs(x - c.x) <= 20 && std::fabs(y - c.y) <= 12) return i; break;
            case Segment: {
                int n = 0; segLabels(c.param, n); const float x0 = c.x - c.r / 2;
                if (x >= x0 && x < x0 + c.r && std::fabs(y - c.y) <= 11) { if (seg) *seg = (int)((x - x0) / (c.r / n)); return i; }
                break; }
            case Stepper:
                if (std::fabs(x - c.x) <= c.r / 2 && std::fabs(y - c.y) <= 11) { if (seg) *seg = x < c.x ? -1 : 1; return i; }
                break;
            }
        }
        return -1;
    }
    int handleHit(float x, float y) const {
        const bool both = val[kF1On] > 0.5f && val[kF2On] > 0.5f;
        if (both) { float hx, hy; handlePos(2, hx, hy); if ((x - hx) * (x - hx) + (y - hy) * (y - hy) <= 100) return 2; }
        for (int h = 0; h < 2; ++h) { float hx, hy; handlePos(h, hx, hy); if ((x - hx) * (x - hx) + (y - hy) * (y - hy) <= 196) return h; }
        return -1;
    }
    int headerHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H) return 0;
        if (x >= LEARN_X && x <= LEARN_X + LEARN_W) return 1;
        if (x >= MIDI_X && x <= MIDI_X + MIDI_W) return 2;
        return 0;
    }
    int presetBarHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H || x < PS_X || x > PS_X + PS_W) return -1;
        return x < PS_X + PS_ARROW ? 0 : (x > PS_X + PS_W - PS_ARROW ? 2 : 1);
    }
    bool clearHit(float x, float y) const { return learnMode && learnWaiting < 0 && y > DY + 8 && y < DY + 24 && x > DX + DW - 170 && x < DX + DW - 10; }

    void setLearn(bool on) {
        learnMode = on; learnWaiting = -1;
        if (!on) setState("cmd", "cancel");
        repaint();
    }
    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = effParam(controls()[editing]); float v;
        if (apply && parse(p, editBuf, v)) commit(p, v);
        editing = -1; editBuf.clear(); repaint();
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (!ev.press) return false;
        std::string* buf = saving ? &saveName : (editing >= 0 ? &editBuf : nullptr);
        if (!buf) {
            if (ev.key == kKeyEscape && learnMode) { setLearn(false); return true; }
            return false;
        }
        if (ev.key == kKeyEnter) {
            if (saving) {
                if (saveUserPreset(saveName, val)) {
                    userPresets = listUserPresets(); curUser = sanitizeName(saveName); curFactory = -1;
                    std::copy(val, val + kParamCount, userVals); saving = false; saveError = false;
                } else saveError = true;
            } else endEdit(true);
            repaint(); return true;
        }
        if (ev.key == kKeyEscape) { if (saving) saving = false; else endEdit(false); repaint(); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!buf->empty()) buf->pop_back(); repaint(); return true; }
        if (ev.key >= 32 && ev.key < 127 && buf->size() < (saving ? 40u : 12u)) {
            char ch = (char)ev.key;
            if (saving && (ev.mod & kModifierShift) && std::isalpha((unsigned char)ch)) ch = (char)std::toupper((unsigned char)ch);
            *buf += ch; repaint(); return true;
        }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) { return true; }
        // right-click: unassign MIDI (learn mode) or arm delete of a user preset (menu)
        if (ev.button == 2 && ev.press) {
            if (menuOpen) {
                const int m = menuHit(x, y);
                if (m >= 0 && menuItem(m).kind == 1) { pendingDelete = menuItem(m).idx; repaint(); }
                return true;
            }
            if (learnMode) {
                const int i = hit(x, y);
                if (i >= 0) { const uint32_t p = controls()[i].param; char b[24]; snprintf(b, sizeof b, "unmap:%u", p); setState("cmd", b); paramCC[p] = -1; repaint(); }
                return true;
            }
            return false;
        }
        if (ev.button != 1) return false;
        if (!ev.press) {
            if (active >= 0) { editParameter(effParam(controls()[active]), false); active = -1; repaint(); return true; }
            if (dragHandle >= 0) {
                const uint32_t ps[4] = { kF1Freq, kF1Peak, kF2Freq, kF2Peak };
                for (uint32_t p : ps) editParameter(p, false);
                dragHandle = -1; repaint(); return true;
            }
            return false;
        }
        if (editing >= 0) endEdit(true);

        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m >= 0) {
                const MRect r = menuItem(m);
                if (r.kind == 2) { menuOpen = false; saving = true; saveError = false; saveName = curUser.empty() ? "" : curUser; }
                else if (r.kind == 1 && pendingDelete == r.idx) { deleteUserPreset(userPresets[r.idx]); if (curUser == userPresets[r.idx]) curUser.clear(); userPresets = listUserPresets(); pendingDelete = -1; repaint(); return true; }
                else { menuOpen = false; if (r.kind == 0) applyFactory(r.idx); else applyUser(userPresets[r.idx]); }
            } else menuOpen = false;
            pendingDelete = -1; menuHover = -1; repaint(); return true;
        }
        const int hh = headerHit(x, y);
        if (hh == 1) { setLearn(!learnMode); return true; }
        if (hh == 2) { commit(kMidiOn, val[kMidiOn] > 0.5f ? 0.f : 1.f); return true; }
        const int pb = presetBarHit(x, y);
        if (pb == 0) { step(-1); return true; }
        if (pb == 2) { step(1); return true; }
        if (pb == 1) { userPresets = listUserPresets(); menuOpen = true; menuHover = -1; pendingDelete = -1; repaint(); return true; }
        if (clearHit(x, y)) { setState("cmd", "clear"); for (int& m : paramCC) m = -1; repaint(); return true; }

        int seg = 0; const int i = hit(x, y, &seg);
        if (i >= 0) {
            const Ctl& c = controls()[i];
            const uint32_t p = effParam(c);
            if (learnMode) {                                   // pick the control to assign
                learnWaiting = (int)c.param; char b[24]; snprintf(b, sizeof b, "learn:%u", c.param); setState("cmd", b); repaint(); return true;
            }
            if (c.type == Toggle) { commit(p, val[p] > 0.5f ? 0.f : 1.f); return true; }
            if (c.type == Segment) { commit(p, (float)seg); return true; }
            if (c.type == Stepper) { commit(p, (float)(((int)(val[p] + .5f) + seg + kNumStyles) % kNumStyles)); return true; }
            if (ev.mod & (kModifierControl | kModifierSuper)) { commit(p, defv[p]); return true; }
            if (i == lastClick && ev.time - lastClickTime < 350) {
                lastClick = -1; editing = i;
                char b[40]; fmt(p, b, sizeof b); editBuf = b;
                const size_t sp = editBuf.find(' '); if (sp != std::string::npos && p != kF1Pan && p != kF2Pan) editBuf.resize(sp);
                if (!editBuf.empty() && editBuf.back() == '%') editBuf.pop_back();
                if ((p == kF1Freq || p == kF2Freq) && val[p] >= 1000) { char kb[16]; snprintf(kb, sizeof kb, "%.2fk", val[p] / 1000); editBuf = kb; }
                if (p == kF1Pan || p == kF2Pan) { char pb2[16]; snprintf(pb2, sizeof pb2, "%s%.0f", val[p] < 0 ? "L" : "R", std::fabs(val[p]) * 100); editBuf = std::fabs(val[p]) < .005f ? "C" : pb2; }
                if (isTimeParam(p) && val[p] < 1) { char tb[16]; snprintf(tb, sizeof tb, "%.0fms", val[p] * 1000); editBuf = tb; }
                repaint(); return true;
            }
            lastClick = i; lastClickTime = ev.time;
            active = i; dragY = y; dragNorm = toNorm(p, val[p]); dragLinked = (ev.mod & kModifierAlt) != 0;
            linkedStart = linkedPartner(p) != (uint32_t)-1 ? toNorm(linkedPartner(p), val[linkedPartner(p)]) : 0;
            dragStartNorm = dragNorm;
            editParameter(p, true); repaint();
            return true;
        }
        if (!learnMode) {
            const int h = handleHit(x, y);
            if (h >= 0) {
                dragHandle = h; dragOpposite = (ev.mod & (kModifierControl | kModifierSuper)) != 0;
                dragX0 = x; dragY0 = y;
                for (int f = 0; f < 2; ++f) { const uint32_t b = f == 0 ? kF1On : kF2On; startOct[f] = std::log2(val[b + 4]); startPeak[f] = val[b + 5]; }
                const uint32_t ps[4] = { kF1Freq, kF1Peak, kF2Freq, kF2Peak };
                for (uint32_t p : ps) editParameter(p, true);
                return true;
            }
        }
        return false;
    }

    static uint32_t linkedPartner(uint32_t p) {
        if (p >= kF1On && p <= kF1Midi) return p - kF1On + kF2On;
        if (p >= kF2On && p <= kF2Midi) return p - kF2On + kF1On;
        return (uint32_t)-1;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (active >= 0) {
            const uint32_t p = effParam(controls()[active]);
            float sens = (ev.mod & kModifierShift) ? 1500.f : 220.f;
            if (paramDef(p).hints & kHintInteger) sens = std::max(60.f, 18.f * (paramDef(p).max - paramDef(p).min));
            dragNorm = clampf(dragNorm + (dragY - y) / sens, 0.f, 1.f); dragY = y;
            setValue(p, fromNorm(p, dragNorm));
            const uint32_t q = linkedPartner(p);
            if (dragLinked && q != (uint32_t)-1) setValue(q, fromNorm(q, clampf(linkedStart + dragNorm - dragStartNorm, 0.f, 1.f)));
            return true;
        }
        if (dragHandle >= 0) {
            const float dOct = std::log2(hzOfX(x) / hzOfX(dragX0));
            const float dPk = peakOfY(y) - peakOfY(dragY0);
            for (int f = 0; f < 2; ++f) {
                if (dragHandle < 2 && f != dragHandle && !dragOpposite) continue;
                const float sign = (dragHandle < 2 && f != dragHandle) ? -1.f : 1.f;
                const uint32_t b = f == 0 ? kF1On : kF2On;
                setValue(b + 4, clampf(std::pow(2.f, startOct[f] + sign * dOct), 20.f, 20000.f));
                setValue(b + 5, clampf(startPeak[f] + dPk, 0.f, 1.f));
            }
            return true;
        }
        if (menuOpen) { const int m = menuHit(x, y); if (m != menuHover) { menuHover = m; repaint(); } return true; }
        const int hh = headerHit(x, y); if (hh != hoverHeader) { hoverHeader = hh; repaint(); }
        const int pb = presetBarHit(x, y); if (pb != presetHover) { presetHover = pb; repaint(); }
        const bool hc = clearHit(x, y); if (hc != hoverClear) { hoverClear = hc; repaint(); }
        const int h = hit(x, y); if (h != hover) { hover = h; repaint(); }
        const int hd = h >= 0 || learnMode ? -1 : handleHit(x, y); if (hd != hoverHandle) { hoverHandle = hd; repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale(), sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen || saving) return true;
        if (presetBarHit(sx, sy) >= 0) { step(ev.delta.getY() > 0 ? -1 : 1); return true; }
        const int i = hit(sx, sy);
        if (i < 0) return false;
        const Ctl& c = controls()[i];
        const uint32_t p = effParam(c);
        if (c.type == Toggle || c.type == Segment) return false;
        if (c.type == Stepper) { commit(p, (float)(((int)(val[p] + .5f) + (ev.delta.getY() > 0 ? 1 : -1) + kNumStyles) % kNumStyles)); return true; }
        float st = (ev.mod & kModifierShift) ? 0.004f : 0.025f;
        if (paramDef(p).hints & kHintInteger) st = 1.f / (paramDef(p).max - paramDef(p).min);
        commit(p, fromNorm(p, toNorm(p, val[p]) + (ev.delta.getY() > 0 ? st : -st)));
        return true;
    }

private:
    float val[kParamCount], defv[kParamCount], userVals[kParamCount] = {};
    int paramCC[kParamCount];
    int active = -1, hover = -1, lastClick = -1, editing = -1;
    uint lastClickTime = 0;
    float dragY = 0, dragNorm = 0, dragStartNorm = 0, linkedStart = 0; bool dragLinked = false;
    int dragHandle = -1, hoverHandle = -1; bool dragOpposite = false;
    float dragX0 = 0, dragY0 = 0, startOct[2] = {}, startPeak[2] = {};
    std::string editBuf;
    int curFactory = 0, presetHover = -1, menuHover = -1, pendingDelete = -1, hoverHeader = 0;
    std::string curUser;
    std::vector<std::string> userPresets;
    bool menuOpen = false, saving = false, saveError = false;
    std::string saveName;
    bool learnMode = false, hoverClear = false; int learnWaiting = -1; float lastLearnEcho = 0;
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CalderaUI)
};

UI* createUI() { return new CalderaUI(); }

END_NAMESPACE_DISTRHO
