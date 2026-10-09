#pragma once
#include <vector>

namespace siege {
constexpr float WIDTH = 1840.f;
constexpr float HEIGHT = 1280.f;
constexpr float BLOCK_W = 44.f;
constexpr float BLOCK_H = 30.f;
enum class Phase : int { Build=0, Battle=1, Finished=2 };
enum class Kind : int { Wood=0, Stone=1, Core=2 };
enum class Weapon : int { Shell=0, Volley=1, Blast=2 };

float terrain(float x);

struct Block {
    float x, y, width, height, health, vy;
    int owner;
    Kind kind;
    bool falling;
};
struct Projectile {
    float x, y, vx, vy, radius;
    Weapon weapon;
    float age;
};
struct Explosion {
    float x, y, radius, age;
};
class World final {
public:
    World();
    void reset();
    bool place(float x, float y, int material);
    bool undoBuild();
    void beginBattle();
    bool fire(float angleDegrees, float speed, int weapon);
    void tick(float seconds);
    std::vector<float> snapshot() const;
    Phase phase() const { return phase_; }
    int resources() const { return resources_; }
    int winner() const { return winner_; }
    int blockCount() const { return static_cast<int>(blocks_.size()); }
    int projectileCount() const { return static_cast<int>(projectiles_.size()); }
    int ammoVolley() const { return ammoVolley_; }
    int ammoBlast() const { return ammoBlast_; }

private:
    void add(float x, float y, int owner, Kind kind);
    void step(float dt);
    void detonate(float x, float y, int directHit, Weapon weapon);
    void updateSupport();
    void settle(float dt);
    void resolveWinner();

    std::vector<Block> blocks_;
    std::vector<Projectile> projectiles_;
    std::vector<Explosion> explosions_;
    int builtCount_=0;
    Phase phase_=Phase::Build;
    int resources_=0;
    int winner_=-1;
    int ammoVolley_=0;
    int ammoBlast_=0;
    float cooldown_=0.f;
};
} // namespace siege
