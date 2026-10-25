// CPU: one, two and four filters; normal and HQ; with modulation running.
#include "../plugins/Caldera/ParamDefs.hpp"
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>
using namespace caldera;
int main() {
    const int SR = 48000, N = SR * 4; std::vector<float> x(N), L(N), R(N); uint32_t r = 1;
    for (auto& v : x) { r = r * 1664525u + 1013904223u; v = 0.3f * ((int32_t)r * (1.f / 2147483648.f)); }
    for (int hq = 0; hq < 2; ++hq) for (int nf : { 1, 2, 4 }) {
        std::unique_ptr<Engine> e(new Engine());
        for (int f = 0; f < nf; ++f) { e->p[fpar(f, kFOn)] = 1; e->p[fpar(f, kFStyle)] = 4; e->p[fpar(f, kFSlope)] = kSlope48; e->p[fpar(f, kFDrive)] = 6; }
        e->p[kHq] = (float)hq; e->p[kRouting] = kParallel; e->setSlot(0, kSrcXlfo1, kDstFilterFirst + 1, 0.2f); e->setSlot(1, kSrcXlfo2, kDstFilterFirst + kNumFilterDests + 1, 0.2f);
        e->setSampleRate(SR);
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < N; i += 256) e->process(x.data() + i, x.data() + i, L.data() + i, R.data() + i, 256);
        const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        printf("%s, %d filter%s (48 dB/oct, Hot, drive, modulated): %.1f%% of one core\n", hq ? "HQ 4x " : "normal", nf, nf > 1 ? "s" : " ", 100 * s / (N / (double)SR));
    }
}
