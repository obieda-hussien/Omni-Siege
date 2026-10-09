#include "siege_engine.h"
#include <algorithm>
#include <cmath>
#include <queue>

namespace siege {
namespace {
constexpr float PI=3.14159265358979323846f;
constexpr float GRAVITY=420.f;
float maxHealth(Kind kind) {
    switch (kind) { case Kind::Wood: return 90.f; case Kind::Stone: return 170.f; default: return 270.f; }
}
bool overlap(float ax,float ay,float aw,float ah,float bx,float by,float bw,float bh) {
    return std::abs(ax-bx)*2.f < aw+bw-1.f && std::abs(ay-by)*2.f < ah+bh-1.f;
}
bool supportedBy(const Block& top,const Block& bottom) {
    return std::abs((top.y+top.height/2.f)-(bottom.y-bottom.height/2.f)) < 3.f &&
      std::abs(top.x-bottom.x) < (top.width+bottom.width)/2.f - 9.f;
}
float quantize(float v,float origin,float step) {
    return origin + std::round((v-origin)/step)*step;
}
}
float terrain(float x) {
    // Flat pads for both castles, an impressive curved ridge in between.
    if (x < 550.f || x > 1230.f) return 1000.f;
    const float ridge=345.f*std::exp(-std::pow((x-845.f)/155.f,2.f));
    const float dip=38.f*std::exp(-std::pow((x-1130.f)/90.f,2.f));
    return 1000.f-ridge+dip;
}
World::World() { reset(); }
void World::add(float x,float y,int owner,Kind kind) {
    blocks_.push_back({x,y,BLOCK_W,BLOCK_H,maxHealth(kind),0.f,owner,kind,false});
}
void World::reset() {
    blocks_.clear(); projectiles_.clear(); explosions_.clear();
    phase_=Phase::Build; resources_=475; builtCount_=0; winner_=-1;
    ammoVolley_=5; ammoBlast_=2; cooldown_=0.f;
    // The blue home fortress is small so building is a meaningful first phase.
    add(306,985,0,Kind::Stone);
    add(306,955,0,Kind::Stone);
    add(306,925,0,Kind::Wood);
    add(306,895,0,Kind::Core);
    add(350,985,0,Kind::Wood);
    add(350,955,0,Kind::Wood);
    // A taller target castle, initially passive. Enemy AI is a separate milestone.
    for (int row=0;row<6;++row) {
        for (int col=0;col<4;++col) {
            int left=1416;
            Kind material=((row==5 && col==1) ? Kind::Core :
                          (row<2 || col==0 || col==3) ? Kind::Stone : Kind::Wood);
            add(static_cast<float>(left+col*44),985.f-row*30.f,1,material);
        }
    }
    add(1460,805,1,Kind::Stone);
    add(1504,805,1,Kind::Stone);
    add(1460,775,1,Kind::Wood);
    add(1504,775,1,Kind::Wood);
}
bool World::place(float x,float y,int material) {
    if (phase_!=Phase::Build || !std::isfinite(x)||!std::isfinite(y)) return false;
    if (material<0 || material>1) return false;
    const int price=material==0?25:40;
    if (resources_<price || blocks_.size()>=155) return false;
    x=quantize(x,174.f,BLOCK_W);
    y=quantize(y,985.f,BLOCK_H);
    if (x<174.f || x>526.f || y<490.f || y>985.f) return false;
    for (const auto& b:blocks_)
        if (overlap(x,y,BLOCK_W,BLOCK_H,b.x,b.y,b.width,b.height)) return false;
    bool supported=y+BLOCK_H/2.f>=terrain(x)-1.f;
    for (const auto& b:blocks_) {
        Block candidate{x,y,BLOCK_W,BLOCK_H,0.f,0.f,0,Kind::Wood,false};
        if (b.owner==0 && supportedBy(candidate,b)) supported=true;
    }
    if (!supported) return false;
    add(x,y,0,material==0?Kind::Wood:Kind::Stone);
    resources_-=price;
    ++builtCount_;
    return true;
}
bool World::undoBuild() {
    if (phase_!=Phase::Build || builtCount_<=0 || blocks_.empty()) return false;
    const Kind kind=blocks_.back().kind;
    resources_+=(kind==Kind::Stone?40:25);
    blocks_.pop_back(); --builtCount_;
    return true;
}
void World::beginBattle() {
    if (phase_==Phase::Build) phase_=Phase::Battle;
}
bool World::fire(float degrees,float speed,int weapon) {
    if (phase_!=Phase::Battle || cooldown_>0.f || !std::isfinite(degrees)
        || !std::isfinite(speed) || degrees<9.f || degrees>82.f
        || speed<530.f || speed>1350.f || weapon<0 || weapon>2
        || projectiles_.size()>20) return false;
    const auto type=static_cast<Weapon>(weapon);
    if (type==Weapon::Volley && ammoVolley_<=0) return false;
    if (type==Weapon::Blast && ammoBlast_<=0) return false;
    if (type==Weapon::Volley) --ammoVolley_;
    if (type==Weapon::Blast) --ammoBlast_;
    const int count=type==Weapon::Volley?3:1;
    for (int i=0;i<count;++i) {
        float offset=type==Weapon::Volley?(i-1)*3.3f:0.f;
        float radians=(degrees+offset)*PI/180.f;
        float magnitude=speed*(type==Weapon::Volley?(0.985f+0.015f*i):1.f);
        projectiles_.push_back({165.f,860.f,std::cos(radians)*magnitude,
             -std::sin(radians)*magnitude,type==Weapon::Blast?15.f:9.f,type,0.f});
    }
    cooldown_=type==Weapon::Volley?1.2f:type==Weapon::Blast?1.6f:0.55f;
    return true;
}
void World::detonate(float x,float y,int hit,Weapon weapon) {
    const float radius=weapon==Weapon::Blast?135.f:weapon==Weapon::Volley?64.f:90.f;
    const float damage=weapon==Weapon::Blast?115.f:weapon==Weapon::Volley?66.f:105.f;
    explosions_.push_back({x,y,radius,0.f});
    if (explosions_.size()>12) explosions_.erase(explosions_.begin());
    for (size_t i=0;i<blocks_.size();++i) {
        auto& b=blocks_[i];
        float dx=std::max(0.f,std::abs(x-b.x)-b.width/2.f);
        float dy=std::max(0.f,std::abs(y-b.y)-b.height/2.f);
        float distance=std::sqrt(dx*dx+dy*dy);
        if (distance<radius) {
            b.health-=damage*(1.f-distance/radius);
            if (static_cast<int>(i)==hit) b.health-=weapon==Weapon::Volley?66.f:105.f;
        }
    }
    blocks_.erase(std::remove_if(blocks_.begin(),blocks_.end(),
       [](const Block& b){return b.health<=0.f;}),blocks_.end());
    updateSupport(); resolveWinner();
}
void World::updateSupport() {
    std::vector<bool> stable(blocks_.size(),false);
    std::queue<size_t> q;
    for (size_t i=0;i<blocks_.size();++i) {
        auto& b=blocks_[i];
        if (b.y+b.height/2.f>=terrain(b.x)-2.f) {
            stable[i]=true; q.push(i); b.falling=false; b.vy=0.f;
        }
    }
    while (!q.empty()) {
        size_t base=q.front(); q.pop();
        for (size_t i=0;i<blocks_.size();++i)
            if (!stable[i] && supportedBy(blocks_[i],blocks_[base])) {
                stable[i]=true; q.push(i); blocks_[i].falling=false; blocks_[i].vy=0.f;
            }
    }
    for (size_t i=0;i<blocks_.size();++i)
        if (!stable[i]) blocks_[i].falling=true;
}
void World::settle(float dt) {
    bool landed=false;
    // Reverse scan allows upper pieces to settle after lower pieces.
    for (size_t i=blocks_.size();i-->0;) {
        auto& b=blocks_[i];
        if (!b.falling) continue;
        b.vy=std::min(950.f,b.vy+GRAVITY*dt);
        float previous=b.y;
        b.y+=b.vy*dt;
        float floor=terrain(b.x)-b.height/2.f;
        for (size_t j=0;j<blocks_.size();++j) {
            if (i==j || blocks_[j].falling) continue;
            const auto& other=blocks_[j];
            if (std::abs(b.x-other.x)<(b.width+other.width)/2.f-9.f &&
                previous+b.height/2.f<=other.y-other.height/2.f+3.f)
                floor=std::min(floor,other.y-other.height/2.f-b.height/2.f);
        }
        if (b.y>=floor) {
            b.y=floor; b.vy=0.f; b.falling=false; landed=true;
        }
    }
    if (landed) updateSupport();
}
void World::resolveWinner() {
    bool player=false,target=false;
    for (const auto& b:blocks_) {
        if (b.kind!=Kind::Core) continue;
        if (b.owner==0) player=true;
        else target=true;
    }
    if (!player||!target) {
        phase_=Phase::Finished;
        winner_=target?1:0;
        projectiles_.clear();
    }
}
void World::step(float dt) {
    for (auto& effect:explosions_) effect.age+=dt;
    explosions_.erase(std::remove_if(explosions_.begin(),explosions_.end(),
        [](const Explosion& e){return e.age>0.85f;}),explosions_.end());
    if (phase_!=Phase::Battle) return;
    cooldown_=std::max(0.f,cooldown_-dt);
    for (size_t i=0;i<projectiles_.size();) {
        auto& p=projectiles_[i];
        p.age+=dt; p.vy+=GRAVITY*dt;
        p.x+=p.vx*dt; p.y+=p.vy*dt;
        int hit=-1;
        for (size_t j=0;j<blocks_.size();++j) {
            const auto& b=blocks_[j];
            if (overlap(p.x,p.y,p.radius*2.f,p.radius*2.f,
                        b.x,b.y,b.width,b.height)) {hit=static_cast<int>(j);break;}
        }
        if (hit>=0 || p.y+p.radius>=terrain(p.x)) {
            float x=p.x,y=std::min(p.y,terrain(p.x));
            Weapon weapon=p.weapon;
            projectiles_.erase(projectiles_.begin()+static_cast<std::ptrdiff_t>(i));
            detonate(x,y,hit,weapon);
            if (phase_==Phase::Finished) return;
        } else if (p.x>WIDTH+60.f || p.x<0.f || p.y>HEIGHT+100.f || p.age>10.f) {
            projectiles_.erase(projectiles_.begin()+static_cast<std::ptrdiff_t>(i));
        } else ++i;
    }
    settle(dt);
}
void World::tick(float seconds) {
    if (phase_==Phase::Build || !std::isfinite(seconds)||seconds<=0.f) return;
    seconds=std::min(seconds,0.05f);
    const int count=std::max(1,static_cast<int>(std::ceil(seconds/(1.f/120.f))));
    for (int i=0;i<count;++i) step(seconds/count);
}
std::vector<float> World::snapshot() const {
    int targetHp=0;
    for (const auto& b:blocks_)
        if (b.owner==1 && b.kind==Kind::Core) {
            targetHp=static_cast<int>(std::round(b.health*100.f/maxHealth(Kind::Core)));
            break;
        }
    std::vector<float> result;
    result.reserve(11+blocks_.size()*7+projectiles_.size()*7+explosions_.size()*4);
    // Protocol 2 header = phase,resources,winner,targetHP,blocks,projectiles,effects,cooldown,volleys,blasts.
    result.insert(result.end(),{2.f,static_cast<float>(phase_),static_cast<float>(resources_),
        static_cast<float>(winner_),static_cast<float>(std::clamp(targetHp,0,100)),
        static_cast<float>(blocks_.size()),static_cast<float>(projectiles_.size()),
        static_cast<float>(explosions_.size()),cooldown_,static_cast<float>(ammoVolley_),
        static_cast<float>(ammoBlast_)});
    for (const auto& b:blocks_)
        result.insert(result.end(),{b.x,b.y,b.width,b.height,static_cast<float>(b.kind),
                      static_cast<float>(b.owner),b.health});
    for (const auto& p:projectiles_)
        result.insert(result.end(),{p.x,p.y,p.vx,p.vy,p.radius,static_cast<float>(p.weapon),p.age});
    for (const auto& e:explosions_) result.insert(result.end(),{e.x,e.y,e.radius,e.age});
    return result;
}
} // namespace siege
