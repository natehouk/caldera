// Behaviour checks: self-oscillation, stereo pan, routing, EG triggers, LFO sync, auto-mute, key tracking, modulation.
#include "../plugins/Caldera/ParamDefs.hpp"
#include <cstdio>
#include <vector>
using namespace caldera;
static int fails=0;
#define CHECK(c,msg) do{ bool ok_=(c); printf("%s  %s\n", ok_?"PASS":"FAIL", msg); if(!ok_) fails++; }while(0)
static const int SR=48000;
struct Buf{ std::vector<float> l,r; };
static Buf run(Engine& e, const std::vector<float>& x){ Buf b; b.l.resize(x.size()); b.r.resize(x.size()); e.process(x.data(),x.data(),b.l.data(),b.r.data(),x.size()); return b; }
static float rms(const std::vector<float>& v,size_t a=0,size_t z=0){ if(!z) z=v.size(); double s=0; for(size_t i=a;i<z;++i) s+=v[i]*v[i]; return std::sqrt(s/(z-a)); }
static std::vector<float> noise(int n,float amp){ std::vector<float> v(n); uint32_t r=1; for(auto& x:v){ r=r*1664525u+1013904223u; x=amp*((int32_t)r*(1.f/2147483648.f)); } return v; }
static std::vector<float> sine(int n,float f,float amp){ std::vector<float> v(n); for(int i=0;i<n;++i) v[i]=amp*std::sin(2*kPi*f*i/SR); return v; }
int main(){
  // self-oscillation for the four resonant styles; Clean/Smooth/Warm must not
  for(int st=0; st<kNumStyles; ++st){ Engine e; e.setSampleRate(SR); e.p[kF1Style]=st; e.p[kF1Peak]=1; e.p[kF1Freq]=800; e.p[kAutoMute]=0;
    std::vector<float> x(SR,0.f); x[0]=0.5f; Buf b=run(e,x); float tail=rms(b.l,SR/2,SR);
    int n; const char* nm=enumLabels(kF1Style,n)[st]; char msg[96];
    bool expect = styleDef(st).kMin < 0; snprintf(msg,sizeof msg,"%-8s %s self-oscillate (tail rms %.3f)",nm,expect?"does":"does not",tail);
    CHECK(expect ? tail>0.02f : tail<0.005f, msg); }
  { // pan: L and R see different cutoffs
    Engine e; e.setSampleRate(SR); e.p[kF1Pan]=1; e.p[kF1Freq]=1000; e.p[kF1Peak]=0; Buf b=run(e,noise(SR,0.3f));
    float rl=rms(b.l), rr=rms(b.r); printf("      pan L rms %.3f R rms %.3f\n",rl,rr); CHECK(rr>rl*1.5f,"pan: right channel brighter than left at pan +1"); }
  { // serial LP 500 -> HP 2000 is far quieter than parallel
    auto mk=[](float route){ Engine e; e.setSampleRate(SR); e.p[kF2On]=1; e.p[kF1Freq]=500; e.p[kF2Type]=2; e.p[kF2Freq]=2000; e.p[kF1Peak]=0; e.p[kF2Peak]=0; e.p[kRouting]=route; return e; };
    Engine s=mk(0), p=mk(1); float rs=rms(run(s,noise(SR,0.3f)).l), rp=rms(run(p,noise(SR,0.3f)).l);
    printf("      serial %.4f parallel %.4f\n",rs,rp); CHECK(rp>rs*3,"routing: parallel LP+HP passes much more than serial"); }
  { // serial with filter 1 bypassed: filter 2 still filters the input
    Engine e; e.setSampleRate(SR); e.p[kF1On]=0; e.p[kF2On]=1; e.p[kF2Type]=0; e.p[kF2Freq]=200; e.p[kF2Peak]=0;
    float r=rms(run(e,sine(SR,4000,0.5f)).l); CHECK(r<0.01f,"serial: filter 2 works when filter 1 is bypassed"); }
  { // mix 0 = dry
    Engine e; e.setSampleRate(SR); e.p[kMix]=0; e.p[kF1Freq]=100; auto x=sine(SR,3000,0.5f); Buf b=run(e,x);
    float d=0; for(int i=9600;i<SR;++i) d=std::max(d,std::fabs(b.l[i]-x[i])); CHECK(d<1e-3f,"mix 0%: output equals input"); }
  { // EG audio trigger: modulates cutoff only while input is loud
    Engine e; e.setSampleRate(SR); e.p[kF1Freq]=200; e.p[kF1Peak]=0; e.p[modParam(kSrcEg,kDstF1Freq)]=1; e.p[kEgThreshold]=-20; e.p[kEgAttack]=0.001f; e.p[kEgSustain]=1;
    std::vector<float> x=sine(SR,3000,0.02f); auto loud=sine(SR,3000,0.5f); x.insert(x.end(),loud.begin(),loud.end());
    Buf b=run(e,x); float q=rms(b.l,SR/2,SR)/0.02f, l=rms(b.l,SR+SR/2,2*SR)/0.5f;
    printf("      relative gain quiet %.3f loud %.3f\n",q,l); CHECK(l>q*5,"EG audio trigger opens the filter above threshold"); }
  { // EG MIDI trigger
    Engine e; e.setSampleRate(SR); e.p[kEgMidi]=1; e.p[kEgAttack]=0.001f; e.p[kEgSustain]=1; run(e,sine(4800,100,0.f));
    CHECK(e.egValue()<1e-3f,"EG MIDI: idle before note"); e.noteOn(60); run(e,sine(4800,100,0.f)); CHECK(e.egValue()>0.9f,"EG MIDI: note-on opens envelope");
    e.noteOff(60); e.p[kEgRelease]=0.01f; run(e,sine(SR/2,100,0.f)); CHECK(e.egValue()<1e-3f,"EG MIDI: note-off releases"); }
  { // key tracking
    Engine e; e.setSampleRate(SR); e.p[kF1Midi]=1; e.p[kF1Freq]=1000; e.noteOn(72); run(e,sine(SR/4,100,0.f));
    printf("      live cutoff %.0f Hz\n",e.liveFreq(0)); CHECK(std::fabs(e.liveFreq(0)-2000)<20,"MIDI key tracking: C5 doubles cutoff"); }
  { // LFO modulation moves cutoff; sync locks to song position
    Engine e; e.setSampleRate(SR); e.p[modParam(kSrcLfo1,kDstF1Freq)]=0.5f; e.p[kL1Rate]=2;
    float lo=1e9,hi=0; for(int k=0;k<48;++k){ run(e,sine(1000,100,0.f)); lo=std::min(lo,e.liveFreq(0)); hi=std::max(hi,e.liveFreq(0)); }
    printf("      LFO sweep %.0f..%.0f Hz\n",lo,hi); CHECK(hi/lo>3,"LFO 1 sweeps cutoff");
    Engine a,b; a.setSampleRate(SR); b.setSampleRate(SR);
    for(Engine* x:{&a,&b}){ x->p[kL1Sync]=1; x->p[kL1Div]=4; x->p[modParam(kSrcLfo1,kDstF1Freq)]=0.5f; }
    a.setTransport(true,true,7.25,120,4); b.setTransport(true,true,3.25,90,4); run(a,sine(1,100,0)); run(b,sine(1,100,0));
    CHECK(std::fabs(a.liveFreq(0)-b.liveFreq(0))<5,"LFO sync: same bar phase gives same modulation"); }
  { // auto-mute stops self-oscillation after the input goes silent
    Engine e; e.setSampleRate(SR); e.p[kF1Style]=3; e.p[kF1Peak]=1; auto x=noise(SR/4,0.3f); x.resize(SR*2,0.f);
    Buf b=run(e,x); CHECK(rms(b.l,SR*3/2,SR*2)<1e-3f,"auto-mute silences self-oscillation");
    Engine f; f.setSampleRate(SR); f.p[kF1Style]=3; f.p[kF1Peak]=1; f.p[kAutoMute]=0; Buf c=run(f,x); CHECK(rms(c.l,SR*3/2,SR*2)>0.01f,"auto-mute off: self-oscillation continues"); }
  printf("%s (%d failures)\n", fails?"FAILED":"ALL PASSED", fails); return fails?1:0; }
