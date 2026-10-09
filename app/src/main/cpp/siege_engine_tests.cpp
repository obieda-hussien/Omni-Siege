#include "siege_engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace siege;
int main() {
    World w;
    assert(w.phase()==Phase::Build);
    assert(w.resources()==475);
    assert(w.ammoVolley()==5 && w.ammoBlast()==2);
    assert(w.blockCount()>25);
    assert(terrain(850)<terrain(300));
    assert(!w.place(NAN,985,0));
    assert(!w.place(850,985,0)); // no building on enemy side
    assert(!w.place(480,650,0)); // unsupported
    assert(w.place(482,985,0));
    assert(w.resources()==450);
    assert(!w.place(482,985,0));
    assert(w.undoBuild());
    assert(w.resources()==475);
    assert(!w.undoBuild());
    w.beginBattle();
    assert(w.phase()==Phase::Battle);
    assert(!w.place(482,985,0));
    assert(!w.fire(NAN,800,0));
    assert(!w.fire(42,800,4));
    assert(w.fire(23,970,1));
    assert(w.ammoVolley()==4);
    assert(w.projectileCount()==3);
    assert(!w.fire(23,970,0)); // cooldown
    for (int i=0;i<900 && w.phase()==Phase::Battle;++i) w.tick(1.f/60.f);
    assert(w.projectileCount()==0);
    assert(w.snapshot()[0]==2.f);
    const auto snapshot=w.snapshot();
    assert(snapshot.size()==11+static_cast<size_t>(snapshot[5])*7
        +static_cast<size_t>(snapshot[6])*7+static_cast<size_t>(snapshot[7])*4);
    w.reset();
    assert(w.phase()==Phase::Build && w.resources()==475);
    std::cout<<"Portrait siege native gameplay tests passed\n";
}
