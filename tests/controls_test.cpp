#include "Controls.h"
#include <cassert>
#include <limits>
int main() {
    Controls c;
    assert(c.finish(std::numeric_limits<float>::infinity())==0.f);
    assert(c.finish(100.f)==1.f);
    for(int i=0;i<10000;i++) c.input(0.5f,-90);
    assert(c.input(0.5f,-90)>0.49f);
    for(int i=0;i<100000;i++) c.input(0.f,-60);
    assert(c.gateGain<0.0001f);
    assert(std::abs(Controls::db(-20)-0.1f)<0.00001f);
}
