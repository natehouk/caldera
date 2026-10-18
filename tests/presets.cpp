// Run pink-ish noise and a drum-like pulse train through every factory preset; report levels.
#include "../plugins/Caldera/ParamDefs.hpp"
#include "../plugins/Caldera/Presets.hpp"
#include <cstdio>
#include <vector>
using namespace caldera;
int main(){ const int SR=48000, N=SR*3; int bad=0;
 std::vector<float> x(N); uint32_t r=3; float lp=0;
 for(int i=0;i<N;++i){ r=r*1664525u+1013904223u; float w=(int32_t)r*(1.f/2147483648.f); lp+=0.05f*(w-lp); x[i]=0.25f*(w*0.5f+lp*2.f); if(i%(SR/2)<2400) x[i]+=0.6f*std::sin(2*kPi*60*i/(float)SR); }
 double sin_=0; for(float v:x) sin_+=v*v; float inR=std::sqrt(sin_/N);
 for(uint32_t p=0;p<kNumPresets;++p){ Engine e; e.setSampleRate(SR);
  for(uint32_t k=0;k<kParamCount;++k) e.p[k]=presets()[p].v[k];
  e.setTransport(true,true,0,124,4); e.noteOn(48);
  std::vector<float> L(N),R(N); e.process(x.data(),x.data(),L.data(),R.data(),N);
  double s=0; float pk=0; bool nf=false; for(int i=0;i<N;++i){ s+=L[i]*L[i]+R[i]*R[i]; pk=std::max(pk,std::max(std::fabs(L[i]),std::fabs(R[i]))); nf|=!std::isfinite(L[i]); }
  float rel=20*std::log10(std::sqrt(s/(2*N))/inR+1e-9);
  bool ok=!nf && rel>-30 && pk<=1.f; bad+=!ok;
  printf("%-18s %-10s level %+6.1f dB  peak %.2f %s\n",presets()[p].name,presets()[p].category,rel,pk,ok?"":"<-- CHECK"); }
 printf("%u presets, %d flagged\n",kNumPresets,bad); return bad; }
