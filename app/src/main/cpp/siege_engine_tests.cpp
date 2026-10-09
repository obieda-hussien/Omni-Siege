#include "siege_engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace siege;
int main() {
    World world;
    assert(world.phase()==Phase::Build);
    assert(world.resources()==475);
    assert(world.blockCount()>25);
    assert(world.botShots()==0);
    assert(world.ammoVolley()==5 && world.ammoBlast()==2);
    assert(terrain(845)<terrain(300));
    assert(!world.place(NAN,985,0));
    assert(!world.place(850,985,0));
    assert(!world.place(480,650,0));
    assert(world.place(482,985,0));
    assert(world.resources()==450);
    assert(!world.place(482,985,0));
    assert(world.undoBuild());
    assert(world.resources()==475);
    assert(!world.undoBuild());
    world.beginBattle();
    assert(world.phase()==Phase::Battle);
    assert(!world.place(482,985,0));
    assert(!world.fire(NAN,800,0));
    assert(!world.fire(42,800,4));
    assert(world.fire(38,780,1));
    assert(world.ammoVolley()==4);
    assert(world.projectileCount()==3);
    assert(!world.fire(38,780,0)); // reload cooldown
    // A real enemy AI must act within several seconds, shooting from the other side.
    bool enemyProjectileObserved=false;
    for(int i=0;i<800 && world.phase()==Phase::Battle;++i) {
        world.tick(1.f/60.f);
        const auto snapshot=world.snapshot();
        assert(snapshot.size()==18+static_cast<size_t>(snapshot[5])*9
            +static_cast<size_t>(snapshot[6])*8
            +static_cast<size_t>(snapshot[7])*4
            +static_cast<size_t>(snapshot[16])*6);
        size_t shotsStart=18+static_cast<size_t>(snapshot[5])*9;
        for(int p=0;p<static_cast<int>(snapshot[6]);++p)
            if(snapshot[shotsStart+p*8+7]==1.f)enemyProjectileObserved=true;
    }
    assert(world.botShots()>0);
    assert(enemyProjectileObserved);
    assert(world.timeLeft()<180.f);
    assert(world.botPlan()>=0 && world.botPlan()<=3);
    world.reset();
    assert(world.phase()==Phase::Build && world.resources()==475);
    assert(world.botShots()==0);
    std::cout<<"Siege physics and enemy AI native tests passed\n";
}
