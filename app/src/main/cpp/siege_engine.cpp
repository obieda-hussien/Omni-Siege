#include "siege_engine.h"
#include <algorithm>
#include <cmath>
#include <queue>

namespace siege {
namespace {
constexpr float BW = 48.0f;
constexpr float BH = 32.0f;
constexpr float GRAVITY = 540.0f;
constexpr float PI = 3.14159265358979323846f;

float hp(Kind kind) {
    return kind == Kind::Wood ? 90.0f : kind == Kind::Stone ? 165.0f : 250.0f;
}
bool overlaps(float ax, float ay, float aw, float ah,
              float bx, float by, float bw, float bh) {
    return std::abs(ax - bx) * 2.0f < aw + bw - 0.5f &&
           std::abs(ay - by) * 2.0f < ah + bh - 0.5f;
}
bool touching(const Block& a, const Block& b) {
    const bool horizontal = std::abs(a.x - b.x) <= (a.width + b.width) / 2.0f + 1.0f &&
                            std::abs(a.y - b.y) < 1.0f;
    const bool vertical = std::abs(a.y - b.y) <= (a.height + b.height) / 2.0f + 1.0f &&
                          std::abs(a.x - b.x) < (a.width + b.width) / 2.0f - 4.0f;
    return horizontal || vertical;
}
} // namespace

World::World() { reset(); }

void World::add(float x, float y, int owner, Kind kind) {
    blocks_.push_back({x, y, BW, BH, hp(kind), 0.0f, owner, kind, false});
}

void World::reset() {
    blocks_.clear();
    projectiles_.clear();
    phase_ = Phase::Build;
    resources_ = 450;
    winner_ = -1;
    cooldown_ = 0.0f;
    // Small player fortress. The player can extend it in build mode.
    add(240, 544, 0, Kind::Stone);
    add(240, 512, 0, Kind::Wood);
    add(240, 480, 0, Kind::Core);
    add(288, 544, 0, Kind::Wood);
    add(288, 512, 0, Kind::Stone);
    // Unmoving target fortress (NOT an AI opponent in this milestone).
    add(916, 544, 1, Kind::Stone);
    add(916, 512, 1, Kind::Stone);
    add(916, 480, 1, Kind::Core);
    add(916, 448, 1, Kind::Wood);
    add(964, 544, 1, Kind::Wood);
    add(964, 512, 1, Kind::Stone);
}

bool World::place(float x, float y, int kind) {
    if (phase_ != Phase::Build || !std::isfinite(x) || !std::isfinite(y)) return false;
    if (kind != 0 && kind != 1) return false;
    const int cost = kind == 0 ? 25 : 40;
    if (resources_ < cost || blocks_.size() >= 100) return false;
    x = 120.0f + std::round((x - 120.0f) / BW) * BW;
    y = 544.0f - std::round((544.0f - y) / BH) * BH;
    if (x < 120.0f || x > 504.0f || y < 160.0f || y > 544.0f) return false;
    for (const auto& b : blocks_) {
        if (overlaps(x, y, BW, BH, b.x, b.y, b.width, b.height)) return false;
    }
    bool supported = y + BH / 2.0f >= GROUND - 0.5f;
    for (const auto& b : blocks_) {
        if (b.owner == 0 && std::abs((y + BH / 2.0f) - (b.y - b.height / 2.0f)) < 1.0f &&
            std::abs(x - b.x) < (BW + b.width) / 2.0f - 3.0f) {
            supported = true;
            break;
        }
    }
    if (!supported) return false;
    add(x, y, 0, kind == 0 ? Kind::Wood : Kind::Stone);
    resources_ -= cost;
    return true;
}

void World::beginBattle() {
    if (phase_ == Phase::Build) phase_ = Phase::Battle;
}

bool World::fire(float degrees, float speed) {
    if (phase_ != Phase::Battle || cooldown_ > 0.0f || projectiles_.size() >= 10 ||
        !std::isfinite(degrees) || !std::isfinite(speed) ||
        degrees < 12.0f || degrees > 82.0f || speed < 350.0f || speed > 900.0f) return false;
    const float radians = degrees * PI / 180.0f;
    projectiles_.push_back({88.0f, 510.0f, std::cos(radians) * speed,
                            -std::sin(radians) * speed, 9.0f});
    cooldown_ = 0.7f;
    return true;
}

void World::explode(float x, float y, int hitIndex) {
    const float radius = 85.0f;
    for (size_t i = 0; i < blocks_.size(); ++i) {
        auto& b = blocks_[i];
        const float dx = std::max(0.0f, std::abs(x - b.x) - b.width / 2.0f);
        const float dy = std::max(0.0f, std::abs(y - b.y) - b.height / 2.0f);
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < radius) {
            b.health -= 90.0f * (1.0f - dist / radius);
            if (static_cast<int>(i) == hitIndex) b.health -= 125.0f;
        }
    }
    blocks_.erase(std::remove_if(blocks_.begin(), blocks_.end(),
                                 [](const Block& b) { return b.health <= 0.0f; }),
                  blocks_.end());
    updateSupport();
    resolveWinner();
}

void World::updateSupport() {
    std::vector<bool> stable(blocks_.size(), false);
    std::queue<size_t> queue;
    for (size_t i = 0; i < blocks_.size(); ++i) {
        auto& b = blocks_[i];
        if (b.y + b.height / 2.0f >= GROUND - 0.5f) {
            stable[i] = true;
            queue.push(i);
            b.falling = false;
            b.vy = 0.0f;
        }
    }
    while (!queue.empty()) {
        const size_t current = queue.front();
        queue.pop();
        for (size_t i = 0; i < blocks_.size(); ++i) {
            if (!stable[i] && touching(blocks_[current], blocks_[i])) {
                stable[i] = true;
                queue.push(i);
            }
        }
    }
    for (size_t i = 0; i < blocks_.size(); ++i) {
        if (!stable[i]) blocks_[i].falling = true;
    }
}

void World::settle(float dt) {
    bool landed = false;
    for (size_t i = 0; i < blocks_.size(); ++i) {
        Block& b = blocks_[i];
        if (!b.falling) continue;
        b.vy = std::min(900.0f, b.vy + GRAVITY * dt);
        const float oldY = b.y;
        b.y = std::min(GROUND - b.height / 2.0f, b.y + b.vy * dt);
        float stoppingY = GROUND - b.height / 2.0f;
        for (size_t j = 0; j < blocks_.size(); ++j) {
            if (i == j || blocks_[j].falling) continue;
            const Block& other = blocks_[j];
            if (std::abs(b.x - other.x) < (b.width + other.width) / 2.0f - 2.0f &&
                oldY + b.height / 2.0f <= other.y - other.height / 2.0f + 2.0f) {
                stoppingY = std::min(stoppingY, other.y - other.height / 2.0f - b.height / 2.0f);
            }
        }
        if (b.y >= stoppingY) {
            b.y = stoppingY;
            b.falling = false;
            b.vy = 0.0f;
            landed = true;
        }
    }
    if (landed) updateSupport();
}

void World::resolveWinner() {
    bool playerCore = false;
    bool targetCore = false;
    for (const auto& b : blocks_) {
        if (b.kind != Kind::Core) continue;
        if (b.owner == 0) playerCore = true;
        if (b.owner == 1) targetCore = true;
    }
    if (!targetCore || !playerCore) {
        phase_ = Phase::Finished;
        winner_ = targetCore ? 1 : 0;
        projectiles_.clear();
    }
}

void World::step(float dt) {
    cooldown_ = std::max(0.0f, cooldown_ - dt);
    for (size_t i = 0; i < projectiles_.size();) {
        auto& p = projectiles_[i];
        p.vy += GRAVITY * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        int hit = -1;
        for (size_t j = 0; j < blocks_.size(); ++j) {
            const Block& b = blocks_[j];
            if (overlaps(p.x, p.y, p.radius * 2.0f, p.radius * 2.0f,
                         b.x, b.y, b.width, b.height)) {
                hit = static_cast<int>(j);
                break;
            }
        }
        if (hit >= 0 || p.y + p.radius >= GROUND) {
            const float x = p.x;
            const float y = std::min(p.y, GROUND);
            projectiles_.erase(projectiles_.begin() + static_cast<long>(i));
            explode(x, y, hit);
            if (phase_ == Phase::Finished) return;
        } else if (p.x > WIDTH + 40 || p.y < -100 || p.y > HEIGHT + 40) {
            projectiles_.erase(projectiles_.begin() + static_cast<long>(i));
        } else {
            ++i;
        }
    }
    settle(dt);
}

void World::tick(float dt) {
    if (phase_ != Phase::Battle || !std::isfinite(dt) || dt <= 0.0f) return;
    dt = std::min(dt, 0.05f);
    const int n = static_cast<int>(std::ceil(dt / (1.0f / 120.0f)));
    for (int i = 0; i < n && phase_ == Phase::Battle; ++i) step(dt / n);
}

std::vector<float> World::snapshot() const {
    int targetCore = 0;
    for (const auto& b : blocks_) {
        if (b.owner == 1 && b.kind == Kind::Core) {
            targetCore = static_cast<int>(std::round(b.health * 100.0f / hp(Kind::Core)));
            break;
        }
    }
    std::vector<float> data;
    data.reserve(7 + blocks_.size() * 7 + projectiles_.size() * 3);
    data.insert(data.end(), {1.0f, static_cast<float>(phase_), static_cast<float>(resources_),
                             static_cast<float>(winner_), static_cast<float>(std::clamp(targetCore, 0, 100)),
                             static_cast<float>(blocks_.size()), static_cast<float>(projectiles_.size())});
    for (const auto& b : blocks_) {
        data.insert(data.end(), {b.x, b.y, b.width, b.height,
                                 static_cast<float>(b.kind), static_cast<float>(b.owner), b.health});
    }
    for (const auto& p : projectiles_) data.insert(data.end(), {p.x, p.y, p.radius});
    return data;
}

} // namespace siege
