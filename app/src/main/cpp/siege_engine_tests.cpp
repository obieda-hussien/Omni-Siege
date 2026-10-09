#include "siege_engine.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    siege::World world;
    assert(world.phase() == siege::Phase::Build);
    assert(world.resources() == 450);
    assert(world.blockCount() == 11);
    assert(!world.place(NAN, 544, 0));
    assert(!world.place(700, 544, 0));
    assert(!world.place(340, 320, 0)); // Unsupported/floating block.
    assert(world.place(360, 544, 0));
    assert(world.resources() == 425);
    assert(!world.place(360, 544, 0)); // Overlap.
    world.beginBattle();
    assert(world.phase() == siege::Phase::Battle);
    assert(!world.place(408, 544, 0));
    assert(!world.fire(NAN, 700));
    assert(!world.fire(90, 700));
    assert(world.fire(45, 740));
    assert(!world.fire(45, 740)); // Cooldown.
    for (int i = 0; i < 400 && world.phase() == siege::Phase::Battle; ++i) {
        world.tick(1.0f / 60.0f);
    }
    assert(world.projectileCount() == 0);
    const auto data = world.snapshot();
    assert(data[0] == 1.0f); // Snapshot protocol version.
    assert(data.size() == 7 + static_cast<size_t>(data[5]) * 7 +
                             static_cast<size_t>(data[6]) * 3);
    world.reset();
    assert(world.phase() == siege::Phase::Build);
    assert(world.resources() == 450);
    std::cout << "Siege native simulation tests passed\n";
    return 0;
}
