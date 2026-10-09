#pragma once
#include <vector>

namespace siege {

constexpr float WIDTH = 1200.0f;
constexpr float HEIGHT = 650.0f;
constexpr float GROUND = 560.0f;

enum class Phase : int { Build = 0, Battle = 1, Finished = 2 };
enum class Kind : int { Wood = 0, Stone = 1, Core = 2 };

struct Block {
    float x, y, width, height;
    float health;
    float vy = 0.0f;
    int owner;
    Kind kind;
    bool falling = false;
};

struct Projectile {
    float x, y, vx, vy;
    float radius = 9.0f;
};

class World final {
public:
    World();
    void reset();
    bool place(float x, float y, int kind);
    void beginBattle();
    bool fire(float degrees, float speed);
    void tick(float dt);
    std::vector<float> snapshot() const;
    Phase phase() const { return phase_; }
    int resources() const { return resources_; }
    int winner() const { return winner_; }
    int blockCount() const { return static_cast<int>(blocks_.size()); }
    int projectileCount() const { return static_cast<int>(projectiles_.size()); }

private:
    std::vector<Block> blocks_;
    std::vector<Projectile> projectiles_;
    Phase phase_ = Phase::Build;
    int resources_ = 0;
    int winner_ = -1;
    float cooldown_ = 0.0f;

    void add(float x, float y, int owner, Kind kind);
    void step(float dt);
    void explode(float x, float y, int hitIndex);
    void updateSupport();
    void settle(float dt);
    void resolveWinner();
};

} // namespace siege
