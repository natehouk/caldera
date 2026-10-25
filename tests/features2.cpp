// 3.0 features: four filters and the four routings, left/right and mid/side, the new shapes and the
// 6 dB/oct slope, per-filter drive/delay/level/pan, input pan, 4x HQ (latency and aliasing), the new
// styles, and modulation system v2 (XLFO, sidechain follower, slot-on-slot) next to the classic grid.
#include "../plugins/Caldera/ParamDefs.hpp"
#include <cstdio>
#include <memory>
#include <vector>
using namespace caldera;
static int fails = 0;
#define CHECK(c, ...) do { bool ok_ = (c); printf("%s  ", ok_ ? "PASS" : "FAIL"); printf(__VA_ARGS__); printf("\n"); if (!ok_) fails++; } while (0)
static const int SR = 48000;
typedef std::unique_ptr<Engine> EP;
static EP mk() {             // a clean, transparent starting point: filter 1 off, no auto-mute
    EP e(new Engine()); e->p[kF1On] = 0; e->p[kAutoMute] = 0;
    for (int f = 0; f < kNumFilters; ++f) { e->p[fpar(f, kFStyle)] = 0; e->p[fpar(f, kFPeak)] = 0; e->p[fpar(f, kFSlope)] = kSlope12; }
    return e;
}
static void on(Engine& e, int f, int type, float hz, int slope = kSlope12) { e.p[fpar(f, kFOn)] = 1; e.p[fpar(f, kFType)] = (float)type; e.p[fpar(f, kFFreq)] = hz; e.p[fpar(f, kFSlope)] = (float)slope; }
static std::vector<float> sine(int n, float f, float a) { std::vector<float> v(n); for (int i = 0; i < n; ++i) v[i] = a * std::sin(2 * kPi * f * i / SR); return v; }
static std::vector<float> noise(int n, float a, uint32_t seed = 1) { std::vector<float> v(n); uint32_t r = seed; for (auto& x : v) { r = r * 1664525u + 1013904223u; x = a * ((int32_t)r * (1.f / 2147483648.f)); } return v; }
static double rms(const std::vector<float>& v, size_t a = 0, size_t z = 0) { if (!z) z = v.size(); double s = 0; for (size_t i = a; i < z; ++i) s += v[i] * (double)v[i]; return std::sqrt(s / (z - a)); }
static void run(Engine& e, const std::vector<float>& l, const std::vector<float>& r, std::vector<float>& L, std::vector<float>& R, const float* sc = nullptr) {
    L.resize(l.size()); R.resize(l.size());
    for (size_t i = 0; i < l.size(); i += 256) { const uint32_t n = (uint32_t)std::min<size_t>(256, l.size() - i); e.process(l.data() + i, r.data() + i, sc ? sc + i : nullptr, sc ? sc + i : nullptr, L.data() + i, R.data() + i, n); }
}
// steady-state gain (dB) of the left output at frequency f, from a single-bin DFT
static float gainAt(Engine& e, float f, float amp = 0.25f) {
    e.setSampleRate(SR); const int N = SR / 2; std::vector<float> x = sine(N, f, amp), L, R; run(e, x, x, L, R);
    double c = 0, s = 0; for (int i = N / 2; i < N; ++i) { const double w = 2 * M_PI * f * i / SR; c += L[i] * std::cos(w); s += L[i] * std::sin(w); }
    return (float)(20 * std::log10(2 * std::sqrt(c * c + s * s) / (N / 2) / amp + 1e-12));
}
static float db(double a, double b) { return (float)(20 * std::log10(a / b)); }

int main() {
    // ---- four filters and routing
    { EP e = mk(); for (int f = 0; f < 4; ++f) on(*e, f, kLowPass, 1000); const float g1 = gainAt(*e, 4000);
      EP o = mk(); on(*o, 0, kLowPass, 1000); const float g0 = gainAt(*o, 4000);
      CHECK(g1 < 4 * g0 + 3 && g1 < -90, "serial: four 12 dB low passes stack (%.1f dB at 2 octaves; one alone %.1f dB)", g1, g0); }
    { EP e = mk(); e->p[kRouting] = kParallel; on(*e, 0, kLowPass, 300); on(*e, 1, kHighPass, 3000); on(*e, 2, kBandPass, 1000); on(*e, 3, kBandPass, 8000);
      const float lo = gainAt(*e, 80), mid = gainAt(*e, 1000), hi = gainAt(*e, 15000);
      CHECK(lo > -8 && mid > -9 && hi > -8, "parallel: four filters each pass their range (%.1f / %.1f / %.1f dB)", lo, mid, hi); }
    { auto build = [](int route) { EP e = mk(); e->p[kRouting] = (float)route; on(*e, 0, kLowPass, 300); on(*e, 2, kHighPass, 3000); return e; };
      EP s = build(kSerial), pp = build(kPairsParallel), ps = build(kPairsSerial);
      const float sLo = gainAt(*s, 80), sHi = gainAt(*s, 12000), pLo = gainAt(*pp, 80), pHi = gainAt(*pp, 12000), qLo = gainAt(*ps, 80), qHi = gainAt(*ps, 12000);
      CHECK(sLo < -30 && sHi < -30, "serial 1>3: low pass then high pass blocks both ends (%.1f / %.1f dB)", sLo, sHi);
      CHECK(pLo > -5 && pHi > -5, "serial pairs in parallel (1>2)|(3>4): lows and highs both pass (%.1f / %.1f dB)", pLo, pHi);
      CHECK(qLo < -30 && qHi < -30, "parallel pairs in series (1|2)>(3|4): low pass feeds the high pass (%.1f / %.1f dB)", qLo, qHi); }
    // ---- left/right and mid/side
    { EP e = mk(); e->p[kChanMode] = kLeftRight; on(*e, 0, kLowPass, 200); e->setSampleRate(SR);
      std::vector<float> x = sine(SR / 2, 4000, 0.3f), L, R; run(*e, x, x, L, R);
      CHECK(rms(L, SR / 4) < 0.01 && std::fabs(db(rms(R, SR / 4), rms(x, SR / 4))) < 0.1, "left/right: filter 1 on the left only (L %.4f, R %.1f dB)", rms(L, SR / 4), db(rms(R, SR / 4), rms(x, SR / 4))); }
    { EP e = mk(); e->p[kChanMode] = kMidSide; on(*e, 0, kLowPass, 200); e->setSampleRate(SR);
      std::vector<float> x = sine(SR / 2, 4000, 0.3f), nx(x.size()), L, R; for (size_t i = 0; i < x.size(); ++i) nx[i] = -x[i];
      run(*e, x, nx, L, R); const float side = db(rms(L, SR / 4), rms(x, SR / 4));
      EP m = mk(); m->p[kChanMode] = kMidSide; on(*m, 0, kLowPass, 200); m->setSampleRate(SR); run(*m, x, x, L, R);
      CHECK(std::fabs(side) < 0.1 && rms(L, SR / 4) < 0.01, "mid/side: filter 1 on the mid only (side %.2f dB, mid %.4f)", side, rms(L, SR / 4)); }
    // ---- shapes
    { EP e = mk(); on(*e, 0, kBell, 1000); e->p[fpar(0, kFEqGain)] = 12; const float at = gainAt(*e, 1000), off = gainAt(*e, 60);
      EP c = mk(); on(*c, 0, kBell, 1000); c->p[fpar(0, kFEqGain)] = -12; const float cut = gainAt(*c, 1000);
      CHECK(std::fabs(at - 12) < 1 && std::fabs(off) < 1.5f && std::fabs(cut + 12) < 1, "bell: +12 dB at the centre (%.1f), %.1f dB far below, cut %.1f dB", at, off, cut); }
    { EP n = mk(); on(*n, 0, kBell, 1000); n->p[fpar(0, kFEqGain)] = 12; n->p[fpar(0, kFPeak)] = 0.8f; const float narrow = gainAt(*n, 1500);
      EP w = mk(); on(*w, 0, kBell, 1000); w->p[fpar(0, kFEqGain)] = 12; const float wide = gainAt(*w, 1500);
      EP s4 = mk(); on(*s4, 0, kBell, 1000, kSlope48); s4->p[fpar(0, kFEqGain)] = 12; const float steep = gainAt(*s4, 1500), c4 = gainAt(*s4, 1000);
      CHECK(narrow < wide - 2 && steep < wide - 1 && std::fabs(c4 - 12) < 1, "bell: Peak and slope narrow the band (%.1f, %.1f vs %.1f dB at 1.5 kHz; 48 dB centre %.1f)", narrow, steep, wide, c4); }
    { EP e = mk(); on(*e, 0, kLowShelf, 300); e->p[fpar(0, kFEqGain)] = 9; const float lo = gainAt(*e, 40), hi = gainAt(*e, 8000);
      EP h = mk(); on(*h, 0, kHighShelf, 3000); h->p[fpar(0, kFEqGain)] = -9; const float hlo = gainAt(*h, 100), hhi = gainAt(*h, 15000);
      CHECK(std::fabs(lo - 9) < 1 && std::fabs(hi) < 1 && std::fabs(hlo) < 1 && std::fabs(hhi + 9) < 1.2f, "shelves: low +9 (%.1f / %.1f dB), high -9 (%.1f / %.1f dB)", lo, hi, hlo, hhi); }
    { EP e = mk(); on(*e, 0, kNotch, 1000); const float at = gainAt(*e, 1000), off = gainAt(*e, 100);
      CHECK(at < -30 && std::fabs(off) < 1, "notch: %.1f dB at the centre, %.1f dB away", at, off); }
    { EP e = mk(); on(*e, 0, kAllPass, 1000, kSlope24); float worst = 0; for (float f : { 50.f, 300.f, 1000.f, 3000.f, 12000.f }) worst = std::max(worst, std::fabs(gainAt(*e, f)));
      EP o = mk(); on(*o, 0, kAllPass, 1000, kSlope6); float w6 = 0; for (float f : { 50.f, 1000.f, 12000.f }) w6 = std::max(w6, std::fabs(gainAt(*o, f)));
      CHECK(worst < 0.3f && w6 < 0.3f, "all pass: flat within %.2f dB (6 dB/oct: %.2f dB)", worst, w6); }
    { EP e = mk(); on(*e, 0, kLowPass, 500, kSlope6); const float a1 = gainAt(*e, 4000), a2 = gainAt(*e, 8000);
      EP h = mk(); on(*h, 0, kHighPass, 2000, kSlope6); const float h1 = gainAt(*h, 250), h2 = gainAt(*h, 125);
      CHECK(std::fabs((a1 - a2) - 6) < 0.8f && std::fabs((h1 - h2) - 6) < 0.8f, "6 dB/oct: low pass %.1f dB/oct, high pass %.1f dB/oct", a1 - a2, h1 - h2); }
    // ---- per-filter drive, level, output pan, delay; input pan
    { EP c = mk(); on(*c, 0, kLowPass, 15000); c->p[fpar(0, kFDrive)] = 24; const float clean = gainAt(*c, 500);
      auto thd = [](int style, float drive) { EP e = mk(); on(*e, 0, kLowPass, 15000); e->p[fpar(0, kFStyle)] = (float)style; e->p[fpar(0, kFDrive)] = drive; e->setSampleRate(SR);
          const int N = SR / 2; std::vector<float> x = sine(N, 500, 0.3f), L, R; run(*e, x, x, L, R);
          double c1 = 0, s1 = 0, tot = 0; for (int i = N / 2; i < N; ++i) { const double w = 2 * M_PI * 500 * i / SR; c1 += L[i] * std::cos(w); s1 += L[i] * std::sin(w); tot += L[i] * (double)L[i]; }
          const double fund = 2 * (c1 * c1 + s1 * s1) / (N / 2) / (N / 2), all = tot / (N / 2); return 10 * std::log10(std::max(all - fund, 1e-20) / fund); };
      const double t0 = thd(4, 0), t1 = thd(4, 18), tc = thd(0, 24);
      CHECK(std::fabs(clean) < 0.2f && tc < -80 && t1 > t0 + 10, "drive: Clean stays clean and level (%.2f dB, THD %.0f dB); Hot THD %.0f -> %.0f dB", clean, tc, t0, t1); }
    { EP e = mk(); on(*e, 0, kLowPass, 15000); e->p[fpar(0, kFLevel)] = -6; e->p[fpar(0, kFOutPan)] = -0.5f; e->setSampleRate(SR);
      std::vector<float> x = sine(SR / 2, 500, 0.3f), L, R; run(*e, x, x, L, R);
      const float l = db(rms(L, SR / 4), rms(x, SR / 4)), r = db(rms(R, SR / 4), rms(x, SR / 4));
      CHECK(std::fabs(l + 6) < 0.2f && std::fabs(r - (-6 + 20 * std::log10(0.5f))) < 0.3f, "level -6 dB and output pan left (L %.1f, R %.1f dB)", l, r); }
    { EP e = mk(); on(*e, 0, kAllPass, 15000, kSlope6); e->p[fpar(0, kFDelay)] = 10; e->setSampleRate(SR);
      std::vector<float> x(SR / 4, 0.f), L, R; x[1000] = 1; run(*e, x, x, L, R);
      int pk = 0; for (int i = 0; i < (int)L.size(); ++i) if (std::fabs(L[i]) > std::fabs(L[pk])) pk = i;
      CHECK(std::abs(pk - 1000 - 480) <= 2, "delay 10 ms: the impulse comes out %d samples later", pk - 1000); }
    { EP e = mk(); e->p[kInPan] = 1; on(*e, 0, kLowPass, 15000); e->setSampleRate(SR); std::vector<float> x = sine(SR / 4, 500, 0.3f), L, R; run(*e, x, x, L, R);
      CHECK(rms(L, SR / 8) < 1e-4 && std::fabs(db(rms(R, SR / 8), rms(x, SR / 8))) < 0.2f, "input pan right: left input silenced"); }
    // ---- HQ: latency and aliasing
    { EP e = mk(); e->p[kHq] = 1; e->setSampleRate(SR);
      CHECK(e->latency() == 48, "HQ reports %u samples of latency", e->latency());
      auto alias = [](bool hq) { EP e = mk(); e->p[kHq] = hq ? 1.f : 0.f; on(*e, 0, kLowPass, 20000); e->p[fpar(0, kFStyle)] = 6; e->p[fpar(0, kFDrive)] = 24; e->p[fpar(0, kFPeak)] = 0.3f; e->setSampleRate(SR);
          const int N = SR; const float f0 = 7919; std::vector<float> x = sine(N, f0, 0.5f), L, R; run(*e, x, x, L, R);
          // energy at frequencies that are not harmonics of f0 (aliases), relative to the fundamental
          auto bin = [&](double f) { double c = 0, s = 0; for (int i = N / 2; i < N; ++i) { const double w = 2 * M_PI * f * i / SR; c += L[i] * std::cos(w); s += L[i] * std::sin(w); } return c * c + s * s; };
          double al = 0; for (int k = 2; k <= 9; ++k) { double h = std::fmod(k * f0, (double)SR); if (h > SR / 2) h = SR - h; if (h > 20 && h < 20000 && std::fabs(h - f0) > 100) al += bin(h); }
          return 10 * std::log10(al / bin(f0) + 1e-30); };
      const double a0 = alias(false), a1 = alias(true);
      CHECK(a1 < a0 - 10, "HQ: aliasing from Raw at full drive %.0f dB -> %.0f dB", a0, a1); }
    { EP e = mk(); on(*e, 0, kLowPass, 2000); e->setSampleRate(SR); std::vector<float> x = noise(SR, 0.3f), L, R; L.resize(SR); R.resize(SR);
      float worstJump = 0; bool finite = true;
      for (int i = 0; i < SR; i += 256) { if (i == SR / 3) e->p[kHq] = 1; if (i == 2 * SR / 3) e->p[kHq] = 0;
          e->process(x.data() + i, x.data() + i, L.data() + i, R.data() + i, std::min(256, SR - i)); }
      for (int i = 1; i < SR; ++i) { finite &= std::isfinite(L[i]); worstJump = std::max(worstJump, std::fabs(L[i] - L[i - 1])); }
      CHECK(finite && worstJump < 0.5f, "switching HQ while playing fades over (largest step %.3f)", worstJump); }
    // ---- styles 3.0
    { int n = 0; const char* const* nm = enumLabels(kF1Style, n);
      CHECK(n == 11 && std::string(nm[7]) == "Tube" && std::string(nm[10]) == "Hollow", "eleven styles (%d)", n);
      for (int st = 7; st < kNumStyles; ++st) { EP e = mk(); on(*e, 0, kLowPass, 15000); e->p[fpar(0, kFStyle)] = (float)st; const float g = gainAt(*e, 300, 0.05f);
        CHECK(std::fabs(g) < 2.f, "%s: small signals pass at about unity (%.1f dB)", nm[st], g); } }
    { EP e = mk(); on(*e, 0, kLowPass, 2000); e->p[fpar(0, kFStyle)] = 7; e->setSampleRate(SR);
      std::vector<float> x = sine(SR / 2, 300, 0.8f), L, R; run(*e, x, x, L, R);
      double c2 = 0, s2 = 0, c3 = 0, s3 = 0; for (int i = SR / 4; i < SR / 2; ++i) { const double w = 2 * M_PI * 300 * i / SR; c2 += L[i] * std::cos(2 * w); s2 += L[i] * std::sin(2 * w); c3 += L[i] * std::cos(3 * w); s3 += L[i] * std::sin(3 * w); }
      double mean = 0; for (int i = SR / 4; i < SR / 2; ++i) mean += L[i]; mean /= SR / 4;
      CHECK(c2 * c2 + s2 * s2 > c3 * c3 + s3 * s3 && std::fabs(mean) < 0.01, "Tube: asymmetric (2nd harmonic above 3rd), no DC (%.4f)", mean); }
    // ---- modulation v2
    { EP e = mk(); on(*e, 0, kLowPass, 1000); e->p[lp_(0, kLRate)] = 4; e->setSlot(0, kSrcXlfo1, kDstFilterFirst + 0 * kNumFilterDests + 1, 0.3f); e->setSampleRate(SR);
      float lo = 1e9, hi = 0; std::vector<float> x(256, 0.1f), L(256), R(256);
      for (int b = 0; b < SR / 256; ++b) { e->process(x.data(), x.data(), L.data(), R.data(), 256); lo = std::min(lo, e->liveFreq(0)); hi = std::max(hi, e->liveFreq(0)); }
      CHECK(hi / lo > 20 && hi / lo < 1000, "XLFO 1 on Filter 1 Freq sweeps the cutoff (%.0f - %.0f Hz)", lo, hi); }
    { EP e = mk(); on(*e, 1, kLowPass, 200); e->p[fp(0, kFSource)] = 1; e->setSlot(3, kSrcEf1, kDstFilterFirst + 1 * kNumFilterDests + 1, 0.5f); e->setSampleRate(SR);
      std::vector<float> x(SR, 0.2f), sc(SR, 0.f), L, R; for (int i = SR / 2; i < SR; ++i) sc[i] = 0.5f * std::sin(0.05f * i);
      L.resize(SR); R.resize(SR); float quiet = 0, loud = 0;
      for (int i = 0; i < SR; i += 256) { e->process(x.data() + i, x.data() + i, sc.data() + i, sc.data() + i, L.data() + i, R.data() + i, std::min(256, SR - i)); if (i < SR / 2 - 512) quiet = e->liveFreq(1); else loud = e->liveFreq(1); }
      CHECK(quiet < 260 && loud > 1000, "EF 1 listening to the sidechain opens filter 2 (%.0f -> %.0f Hz)", quiet, loud); }
    { EP e = mk(); on(*e, 0, kLowPass, 1000); e->p[kL1Rate] = 2; e->p[modParam(kSrcLfo1, kDstF1Freq)] = 0.25f;
      e->p[xyp(0, 0)] = 1; e->setSlot(0, kSrcXy1X, kDstFilterFirst + 1, 0.1f); e->setSampleRate(SR);
      float lo = 1e9, hi = 0; std::vector<float> x(256, 0.1f), L(256), R(256);
      for (int b = 0; b < SR / 256; ++b) { e->process(x.data(), x.data(), L.data(), R.data(), 256); if (b < 20) continue; lo = std::min(lo, e->liveFreq(0)); hi = std::max(hi, e->liveFreq(0)); }
      const float centre = std::sqrt(lo * hi), expect = valueOf(kF1Freq, normOf(kF1Freq, 1000) + 0.1f);
      CHECK(hi / lo > 3.5f && std::fabs(std::log2(centre / expect)) < 0.15f, "classic grid and v2 slots add up (sweep %.0f-%.0f Hz around %.0f, expected %.0f)", lo, hi, centre, expect); }
    { EP e = mk(); on(*e, 0, kLowPass, 1000); e->p[xyp(0, 0)] = 1; e->setSlot(0, kSrcXy1X, kDstFilterFirst + 1, 0.f); e->setSlot(1, kSrcWheel, kDstSlotFirst + 0, 0.2f);
      e->modWheel(127); e->setSampleRate(SR); std::vector<float> x(4096, 0.1f), L(4096), R(4096); e->process(x.data(), x.data(), L.data(), R.data(), 4096);
      const float expect = valueOf(kF1Freq, normOf(kF1Freq, 1000) + 0.2f);
      CHECK(std::fabs(std::log2(e->liveFreq(0) / expect)) < 0.05f, "slot-on-slot: the mod wheel sets slot 1's amount (%.0f Hz, expected %.0f)", e->liveFreq(0), expect); }
    printf("%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails); return fails ? 1 : 0;
}
