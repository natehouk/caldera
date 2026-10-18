// Steady-state sine response of every style x type x slope at low peak; checks slopes and stability.
#include "../plugins/Caldera/ParamDefs.hpp"
#include <cstdio>
#include <vector>
using namespace caldera;
static float gainAt(int style,int type,int slope,float fc,float f,float peak){
  Engine e; e.setSampleRate(48000); e.p[kF1Style]=style; e.p[kF1Type]=type; e.p[kF1Slope]=slope; e.p[kF1Freq]=fc; e.p[kF1Peak]=peak;
  e.p[kAutoMute]=0; e.p[kInGain]=-24; e.p[kOutGain]=24; // small signal so nonlinear styles stay near-linear
  const int N=48000/2; std::vector<float> x(N),l(N),r(N); for(int i=0;i<N;++i) x[i]=0.5f*std::sin(2*kPi*f*i/48000.f);
  e.process(x.data(),x.data(),l.data(),r.data(),N); // single-bin DFT: measure only the fundamental, so a style's harmonic distortion doesn't count as leakage
  double c=0,sn=0; for(int i=N/2;i<N;++i){ double w=2*M_PI*f*i/48000.0; c+=l[i]*std::cos(w); sn+=l[i]*std::sin(w); }
  double amp=2*std::sqrt(c*c+sn*sn)/(N/2);
  return 20*std::log10(amp/0.5+1e-12); }
int main(){ int fails=0; const char* tn[]={"LP","BP","HP"};
  for(int st=0;st<kNumStyles;++st){ int n=0; const char* const* sn=enumLabels(kF1Style,n);
    for(int ty=0;ty<3;++ty){ printf("%-9s %s:",sn[st],tn[ty]);
      for(int sl=0;sl<3;++sl){ float pass = ty==0?gainAt(st,ty,sl,1000,125,0): ty==2?gainAt(st,ty,sl,1000,8000,0):gainAt(st,ty,sl,1000,1000,0);
        float stop = ty==0?gainAt(st,ty,sl,1000,4000,0): ty==2?gainAt(st,ty,sl,1000,250,0):gainAt(st,ty,sl,1000,4000,0);
        float att=pass-stop; float expect = ty==1? 6.f*(1<<sl)*2*0.5f : 12.f*(1<<sl)*2; // 2 octaves away
        bool ok = std::fabs(pass)<4 && att > expect*0.6f; if(!ok) fails++;
        printf("  %ddB pass %+5.1f att %5.1f%s",12<<sl,pass,att,ok?"":" <--"); }
      printf("\n"); } }
  printf("%s (%d failures)\n",fails?"FAILED":"ALL PASSED",fails); return fails?1:0; }
