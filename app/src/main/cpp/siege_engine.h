#pragma once
#include <vector>
#include <cstdint>

namespace siege {
constexpr float WIDTH=1840.f, HEIGHT=1280.f;
constexpr float BLOCK_W=44.f, BLOCK_H=30.f;
enum class Phase:int { Build=0,Battle=1,Finished=2 };
enum class Kind:int { Wood=0,Stone=1,Core=2 };
enum class Weapon:int { Shell=0,Volley=1,Blast=2 };
float terrain(float x);
struct Block {
    float x,y,width,height,health,vy;
    int owner;
    Kind kind;
    bool falling;
    float rotation=0.f, angularVelocity=0.f;
};
struct Projectile {
    float x,y,vx,vy,radius;
    Weapon weapon;
    float age;
    int owner=0;
};
struct Explosion { float x,y,radius,age; };
struct Debris { float x,y,vx,vy,size,life; };

class World final {
public:
    World();
    void reset();
    bool place(float x,float y,int material);
    bool undoBuild();
    void beginBattle();
    bool fire(float angleDegrees,float speed,int weapon);
    void tick(float seconds);
    std::vector<float> snapshot() const;
    Phase phase() const { return phase_; }
    int resources() const { return resources_; }
    int winner() const { return winner_; }
    int blockCount() const { return static_cast<int>(blocks_.size()); }
    int projectileCount() const { return static_cast<int>(projectiles_.size()); }
    int ammoVolley() const { return ammoVolley_; }
    int ammoBlast() const { return ammoBlast_; }
    int botShots() const { return botShots_; }
    int botPlan() const { return botPlan_; }
    float timeLeft() const { return timeLeft_; }
private:
    void add(float x,float y,int owner,Kind kind);
    void step(float dt);
    bool launch(int owner,float degrees,float speed,Weapon weapon);
    void botTurn();
    void detonate(float x,float y,int directHit,Weapon weapon,int attacker);
    void updateSupport();
    void settle(float dt);
    void resolveWinner();
    void damageCollision(Block& block,float speed);
    float corePercent(int owner) const;
    std::vector<Block> blocks_;
    std::vector<Projectile> projectiles_;
    std::vector<Explosion> explosions_;
    std::vector<Debris> debris_;
    int builtCount_=0;
    Phase phase_=Phase::Build;
    int resources_=0,winner_=-1;
    int ammoVolley_=0,ammoBlast_=0;
    int botAmmoVolley_=0,botAmmoBlast_=0,botShots_=0,botPlan_=0;
    float cooldown_=0.f,botCooldown_=0.f,timeLeft_=180.f;
    uint32_t rng_=0xA31FC729u;
};
} // namespace siege
