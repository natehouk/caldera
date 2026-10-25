// Per-source presets: every factory entry is in range, and user files round-trip.
#include "../plugins/Caldera/ComponentPresets.hpp"
#include <cstdio>
#include <cstdlib>
using namespace caldera;
int main() {
    int bad = 0;
    for (int k = 0; k < kNumCompKinds; ++k) {
        const std::vector<int> f = compFactoryFor(k);
        if (f.size() < 5) { printf("FAIL %s has only %zu factory presets\n", compKindName(k), f.size()); ++bad; }
        for (int i : f) {
            float v[kXlfoFields]; compFactoryValues(i, v);
            for (int fl = 0; fl < compFieldCount(k); ++fl) { const Range r = paramRange(compParam(k, 0, fl));
                if (!(v[fl] >= r.min && v[fl] <= r.max)) { printf("FAIL %s / %s field %d = %g out of range\n", compKindName(k), compFactory()[i].name, fl, v[fl]); ++bad; } }
        }
        printf("%-9s %zu factory presets\n", compKindName(k), f.size());
    }
    // round trip through a temporary preset folder
#ifndef _WIN32
    setenv("XDG_CONFIG_HOME", "/tmp/caldera-modpreset-test", 1);
#endif
    for (int k = 0; k < kNumCompKinds; ++k) {
        float a[kXlfoFields], b[kXlfoFields];
        for (int fl = 0; fl < compFieldCount(k); ++fl) { const Range r = paramRange(compParam(k, 0, fl)); a[fl] = r.integer ? r.max : r.min + 0.37f * (r.max - r.min); }
        if (!saveCompPreset(k, "Round Trip", a) || !loadCompPreset(k, "Round Trip", b)) { printf("FAIL %s save/load\n", compKindName(k)); ++bad; continue; }
        for (int fl = 0; fl < compFieldCount(k); ++fl) if (std::fabs(a[fl] - b[fl]) > 1e-4f * std::max(1.f, std::fabs(a[fl]))) { printf("FAIL %s field %d %g != %g\n", compKindName(k), fl, a[fl], b[fl]); ++bad; }
        const std::vector<std::string> l = listCompPresets(k);
        if (l.size() != 1 || l[0] != "Round Trip") { printf("FAIL %s listing\n", compKindName(k)); ++bad; }
        deleteCompPreset(k, "Round Trip");
        if (!listCompPresets(k).empty()) { printf("FAIL %s delete\n", compKindName(k)); ++bad; }
    }
    printf(bad ? "%d FAILED\n" : "all passed\n", bad); return bad;
}
