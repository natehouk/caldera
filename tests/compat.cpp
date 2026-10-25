// 2.x compatibility: random 2.x settings (3.0 parameters at their defaults), with changes between blocks,
// MIDI notes and host tempo, through the frozen 2.0 engine and the 3.0 engine: outputs must match.
#include "../plugins/Caldera/ParamDefs.hpp"
#include "legacy/CalderaEngine2.hpp"
#include <cstdio>
#include <memory>
#include <random>
#include <vector>
using namespace caldera;
int main() {
    std::mt19937 g(5); std::uniform_real_distribution<float> u(0, 1);
    int bad = 0; float worst = -1000;
    for (int trial = 0; trial < 40; ++trial) {
        const int sr = trial % 3 == 0 ? 44100 : (trial % 3 == 1 ? 48000 : 96000);
        std::unique_ptr<Engine> e(new Engine()); std::unique_ptr<caldera2::Engine> o(new caldera2::Engine());
        auto rnd = [&](uint32_t i) {
            const Range r = paramRange(i); float lo = r.min, hi = r.max;
            if (i == kF1Style || i == kF2Style) hi = kNumStyles2 - 1;           // 2.x ranges
            if (i == kF1Type || i == kF2Type || i == kF1Slope || i == kF2Slope) hi = 2;
            if (i == kRouting) hi = 1;
            float v = r.log ? lo * std::pow(hi / lo, u(g)) : lo + (hi - lo) * u(g);
            if (r.integer) v = std::round(v);
            if (i >= kModFirst && i <= kModLast && u(g) < 0.5f) v = 0;
            if (i == kOutGain) v = std::min(v, 6.f);
            return v;
        };
        for (uint32_t i = 0; i < kOutEg; ++i) { const float v = rnd(i); e->p[i] = v; o->p[i] = v; }
        e->setSampleRate((float)sr); o->setSampleRate((float)sr);
        const int N = sr; std::vector<float> l(N), r(N), L1(N), R1(N), L2(N), R2(N);
        for (int i = 0; i < N; ++i) { const float env = (i / (sr / 5)) % 2 ? 0.6f : 0.01f; l[i] = env * (2 * u(g) - 1); r[i] = 0.7f * l[i] + 0.2f * env * (2 * u(g) - 1); }
        for (int i = 0; i < N; i += 333) {
            const int n = std::min(333, N - i);
            const float ev = u(g);
            if (ev < 0.06f) { const int nt = 40 + (int)(u(g) * 40); e->noteOn(nt); o->noteOn(nt); }
            else if (ev < 0.09f) { e->allNotesOff(); o->allNotesOff(); }
            if (u(g) < 0.05f) { const uint32_t k = (uint32_t)(u(g) * kOutEg); const float v = rnd(k); e->p[k] = v; o->p[k] = v; }
            const bool play = (i / 9000) % 2 == 0;
            e->setTransport(true, play, i / (double)sr * 0.5, 120, 4); o->setTransport(true, play, i / (double)sr * 0.5, 120, 4);
            e->process(l.data() + i, r.data() + i, L1.data() + i, R1.data() + i, n);
            o->process(l.data() + i, r.data() + i, L2.data() + i, R2.data() + i, n);
        }
        double d = 0, s = 1e-30; for (int i = 0; i < N; ++i) { d += (L1[i] - L2[i]) * (L1[i] - L2[i]) + (R1[i] - R2[i]) * (R1[i] - R2[i]); s += l[i] * l[i] + r[i] * r[i]; }
        const float db = d == 0 ? -240.f : (float)(10 * std::log10(d / s));
        worst = std::max(worst, db); if (db > -120) { ++bad; printf("trial %d (%d Hz): difference %.1f dB\n", trial, sr, db); }
    }
    printf("2.x compatibility: largest difference %.1f dB below the input level, %d failures\n", -worst, bad);
    return bad;
}
