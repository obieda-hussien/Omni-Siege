#include "siege_engine.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <limits>
#include <cstddef>

namespace siege {
namespace {
constexpr float PI=3.14159265358979323846f;
constexpr float GRAVITY=420.f;
constexpr float PLAYER_X=165.f, BOT_X=1665.f, LAUNCH_Y=860.f;
float maxHealth(Kind kind) {
    switch(kind) {case Kind::Wood:return 90.f; case Kind::Stone:return 170.f; default:return 270.f;}
}
float clamp01(float x){return std::clamp(x,0.f,1.f);}
bool overlap(float ax,float ay,float aw,float ah,float bx,float by,float bw,float bh) {
    return std::abs(ax-bx)*2.f<aw+bw-1.f && std::abs(ay-by)*2.f<ah+bh-1.f;
}
bool supportedBy(const Block& top,const Block& bottom) {
    return std::abs((top.y+top.height/2.f)-(bottom.y-bottom.height/2.f))<3.5f &&
        std::abs(top.x-bottom.x)<(top.width+bottom.width)/2.f-9.f;
}
float quantize(float value,float origin,float step){
    return origin+std::round((value-origin)/step)*step;
}
}
float terrain(float x) {
    if(x<550.f || x>1230.f) return 1000.f;
    return 1000.f-345.f*std::exp(-std::pow((x-845.f)/155.f,2.f))
       +38.f*std::exp(-std::pow((x-1130.f)/90.f,2.f));
}

World::World(){reset();}
void World::add(float x,float y,int owner,Kind kind){
    blocks_.push_back({x,y,BLOCK_W,BLOCK_H,maxHealth(kind),0.f,owner,kind,false,0.f,0.f});
}
void World::reset(){
    blocks_.clear();projectiles_.clear();explosions_.clear();debris_.clear();
    phase_=Phase::Build; resources_=475;builtCount_=0;winner_=-1;
    ammoVolley_=5;ammoBlast_=2;botAmmoVolley_=5;botAmmoBlast_=2;
    cooldown_=0.f;botCooldown_=2.25f;botShots_=0;botPlan_=0;timeLeft_=180.f;
    rng_=0xA31FC729u;
    add(306,985,0,Kind::Stone); add(306,955,0,Kind::Stone);
    add(306,925,0,Kind::Wood);add(306,895,0,Kind::Core);
    add(350,985,0,Kind::Wood);add(350,955,0,Kind::Wood);
    for(int row=0;row<6;++row){
        for(int col=0;col<4;++col){
            Kind kind=(row==5 && col==1)?Kind::Core:
               (row<2 || col==0 || col==3)?Kind::Stone:Kind::Wood;
            add(static_cast<float>(1416+col*44),985.f-row*30.f,1,kind);
        }
    }
    add(1460,805,1,Kind::Stone);add(1504,805,1,Kind::Stone);
    add(1460,775,1,Kind::Wood);add(1504,775,1,Kind::Wood);
}
bool World::place(float x,float y,int material){
    if(phase_!=Phase::Build || !std::isfinite(x)||!std::isfinite(y) ||
       (material!=0&&material!=1)) return false;
    int price=material==0?25:40;
    if(resources_<price||blocks_.size()>=155) return false;
    x=quantize(x,174.f,BLOCK_W);
    y=quantize(y,985.f,BLOCK_H);
    if(x<174.f||x>526.f||y<490.f||y>985.f) return false;
    Block candidate{x,y,BLOCK_W,BLOCK_H,0.f,0.f,0,Kind::Wood,false,0.f,0.f};
    for(const auto& b:blocks_)
        if(overlap(x,y,BLOCK_W,BLOCK_H,b.x,b.y,b.width,b.height)) return false;
    bool supported=y+BLOCK_H/2.f>=terrain(x)-1.f;
    for(const auto& b:blocks_) if(b.owner==0&&supportedBy(candidate,b))supported=true;
    if(!supported)return false;
    add(x,y,0,material==0?Kind::Wood:Kind::Stone);
    resources_-=price; ++builtCount_;
    return true;
}
bool World::undoBuild(){
    if(phase_!=Phase::Build||builtCount_<=0||blocks_.empty()) return false;
    auto kind=blocks_.back().kind;
    resources_+=(kind==Kind::Stone?40:25);
    blocks_.pop_back();--builtCount_;return true;
}
void World::beginBattle(){
    if(phase_==Phase::Build) {
        phase_=Phase::Battle;
        updateSupport();
    }
}
bool World::launch(int owner,float degrees,float speed,Weapon weapon){
    const float radians=degrees*PI/180.f;
    const int count=weapon==Weapon::Volley?3:1;
    for(int i=0;i<count;++i){
        const float spread=weapon==Weapon::Volley?(i-1)*2.3f:0.f;
        const float theta=radians+spread*PI/180.f;
        const float muzzle=owner==0?PLAYER_X:BOT_X;
        const float speedScale=weapon==Weapon::Volley?(0.99f+i*0.01f):1.f;
        projectiles_.push_back({muzzle,LAUNCH_Y,
            (owner==0?1.f:-1.f)*std::cos(theta)*speed*speedScale,
            -std::sin(theta)*speed*speedScale,
            weapon==Weapon::Blast?15.f:9.f,weapon,0.f,owner});
    }
    return true;
}
bool World::fire(float degrees,float speed,int weapon){
    if(phase_!=Phase::Battle||cooldown_>0.f||projectiles_.size()>32 ||
       !std::isfinite(degrees)||!std::isfinite(speed)||degrees<9.f||
       degrees>82.f||speed<530.f||speed>1350.f||weapon<0||weapon>2)return false;
    auto kind=static_cast<Weapon>(weapon);
    if(kind==Weapon::Volley&&ammoVolley_<=0)return false;
    if(kind==Weapon::Blast&&ammoBlast_<=0)return false;
    if(kind==Weapon::Volley)--ammoVolley_;
    if(kind==Weapon::Blast)--ammoBlast_;
    launch(0,degrees,speed,kind);
    cooldown_=kind==Weapon::Volley?1.15f:kind==Weapon::Blast?1.55f:0.6f;
    return true;
}
float World::corePercent(int owner)const{
    for(const auto& b:blocks_)
        if(b.owner==owner&&b.kind==Kind::Core)
            return clamp01(b.health/maxHealth(Kind::Core))*100.f;
    return 0.f;
}
void World::botTurn(){
    if(phase_!=Phase::Battle||botCooldown_>0.f||projectiles_.size()>18)return;
    // The AI acts only on the public native world state. No player input, profile,
    // secrets, impossible precision or privileged data is used.
    // Vary tactics: core rush, destroy low supports, remove tall structures.
    const int plan=botShots_%4;
    botPlan_=plan;
    float desiredX=306.f, desiredY=895.f;
    float highestValue=-1.f;
    for(const auto& b:blocks_) {
        if(b.owner!=0)continue;
        float value=0.f;
        if(plan==0)value=(b.kind==Kind::Core?200.f:25.f)-std::abs(b.x-306.f)*0.05f;
        else if(plan==1)value=b.y*0.15f+(b.kind==Kind::Stone?22.f:0.f);
        else if(plan==2)value=1200.f-b.y+(b.kind==Kind::Core?12.f:0.f);
        else value=(b.kind==Kind::Wood?100.f:72.f)+
            (1.f-clamp01(b.health/maxHealth(b.kind)))*100.f;
        if(value>highestValue){highestValue=value;desiredX=b.x;desiredY=b.y;}
    }
    if(highestValue<0.f)return;
    struct Candidate {float angle,speed,score;};
    Candidate best{43.f,790.f,-1e8f};
    // Cheap analytic rollout: ~350 candidate trajectories, native C++, no NN
    // inference and no per-frame screen analysis. Account for the mountain.
    for(float angle=28.f;angle<=68.f;angle+=4.f){
        const float radians=angle*PI/180.f;
        const float cs=std::cos(radians),sn=std::sin(radians);
        for(float speed=660.f;speed<=1260.f;speed+=30.f){
            const float vx=cs*speed;
            float impactX=BOT_X,impactY=LAUNCH_Y;
            bool collided=false;
            for(float x=BOT_X-18.f;x>85.f;x-=18.f){
                const float time=(BOT_X-x)/vx;
                const float y=LAUNCH_Y-sn*speed*time+GRAVITY*time*time*0.5f;
                if(y>terrain(x)-6.f) {
                    impactX=x;impactY=terrain(x);collided=true;break;
                }
                for(const auto& b:blocks_){
                    if(overlap(x,y,20.f,20.f,b.x,b.y,b.width,b.height)){
                        impactX=x;impactY=y;collided=true;break;
                    }
                }
                if(collided)break;
            }
            float distance=std::hypot(impactX-desiredX,impactY-desiredY);
            float score=-distance*0.27f;
            // Prefer structurally valuable splash damage and especially the core.
            for(const auto& b:blocks_){
                if(b.owner!=0) continue;
                const float d=std::hypot(impactX-b.x,impactY-b.y);
                if(d<125.f) {
                    float worth=b.kind==Kind::Core?78.f:
                        (b.y>935.f?32.f:22.f);
                    score+=worth*(1.f-d/125.f);
                }
            }
            // Do not choose an intercepted shot hitting the bot's own fortress.
            if(impactX>1240.f)score-=400.f;
            if(!collided)score-=100.f;
            if(score>best.score)best={angle,speed,score};
        }
    }
    // Tiny, seeded exploration to prevent identical repetitions in every battle.
    rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;
    float wobble=static_cast<float>(rng_%9)-4.f;
    auto weapon=Weapon::Shell;
    if(plan==2 && botAmmoVolley_>0){weapon=Weapon::Volley;--botAmmoVolley_;}
    if(plan==1 && botAmmoBlast_>0){weapon=Weapon::Blast;--botAmmoBlast_;}
    launch(1,std::clamp(best.angle+wobble*0.35f,9.f,82.f),best.speed,weapon);
    ++botShots_;
    botCooldown_=(weapon==Weapon::Volley?2.8f:weapon==Weapon::Blast?3.f:2.35f);
}
void World::detonate(float x,float y,int directHit,Weapon weapon,int attacker){
    const float radius=weapon==Weapon::Blast?138.f:weapon==Weapon::Volley?68.f:96.f;
    const float damage=weapon==Weapon::Blast?120.f:weapon==Weapon::Volley?68.f:112.f;
    explosions_.push_back({x,y,radius,0.f});
    if(explosions_.size()>18)explosions_.erase(explosions_.begin());
    for(size_t i=0;i<blocks_.size();++i){
        auto& b=blocks_[i];
        const float dx=std::max(0.f,std::abs(x-b.x)-b.width/2.f);
        const float dy=std::max(0.f,std::abs(y-b.y)-b.height/2.f);
        float d=std::hypot(dx,dy);
        if(d<radius){
            b.health-=damage*(1.f-d/radius);
            if(static_cast<int>(i)==directHit)b.health-=weapon==Weapon::Volley?58.f:104.f;
            if(b.health>0.f && d<radius*0.65f) {
                b.angularVelocity+=(b.x-x>0?1.f:-1.f)*0.05f;
            }
        }
    }
    const float speed=weapon==Weapon::Blast?225.f:160.f;
    // Lightweight physical pieces. Bound count; decay before final removal.
    for(int i=0;i<11 && debris_.size()<80;++i){
        float theta=(i*2.f*PI/11.f)+x*0.006f;
        float s=speed*(0.45f+(i%5)*0.13f);
        debris_.push_back({x,y,std::cos(theta)*s,std::sin(theta)*s-90.f,
            2.5f+(i%4)*1.6f,0.45f+(i%4)*0.17f});
    }
    blocks_.erase(std::remove_if(blocks_.begin(),blocks_.end(),
                [](const Block& b){return b.health<=0.f;}),blocks_.end());
    updateSupport();resolveWinner();
    (void)attacker;
}
void World::updateSupport(){
    std::vector<bool> stable(blocks_.size(),false);
    std::queue<size_t> q;
    for(size_t i=0;i<blocks_.size();++i){
        auto& b=blocks_[i];
        if(b.y+b.height/2.f>=terrain(b.x)-2.f){
            stable[i]=true;q.push(i);b.falling=false;b.vy=0.f;
        }
    }
    while(!q.empty()){
        size_t from=q.front();q.pop();
        for(size_t i=0;i<blocks_.size();++i){
            if(!stable[i] && supportedBy(blocks_[i],blocks_[from])){
                stable[i]=true;q.push(i);blocks_[i].falling=false;blocks_[i].vy=0.f;
            }
        }
    }
    for(size_t i=0;i<blocks_.size();++i) if(!stable[i]){
        auto& b=blocks_[i]; if(!b.falling) {
            b.falling=true;b.angularVelocity=(b.x>930.f?1.f:-1.f)*0.44f;
        }
    }
}
void World::damageCollision(Block& b,float velocity){
    float severity=std::max(0.f,velocity-155.f);
    b.health-=severity*0.12f;
    b.rotation=std::clamp(b.rotation,-0.42f,0.42f);
}
void World::settle(float dt){
    bool landed=false;
    for(size_t i=blocks_.size();i-->0;){
        auto& b=blocks_[i];
        if(!b.falling)continue;
        b.vy=std::min(900.f,b.vy+GRAVITY*dt);
        b.rotation=std::clamp(b.rotation+b.angularVelocity*dt,-0.42f,0.42f);
        const float previous=b.y;
        b.y+=b.vy*dt;
        float floor=terrain(b.x)-b.height/2.f;
        int floorIndex=-1;
        for(size_t j=0;j<blocks_.size();++j){
            if(i==j||blocks_[j].falling)continue;
            const auto& other=blocks_[j];
            if(std::abs(b.x-other.x)<(b.width+other.width)/2.f-9.f &&
                previous+b.height/2.f<=other.y-other.height/2.f+3.f &&
                other.y-other.height/2.f-b.height/2.f<floor) {
                floor=other.y-other.height/2.f-b.height/2.f;
                floorIndex=static_cast<int>(j);
            }
        }
        if(b.y>=floor) {
            b.y=floor;
            const float impactSpeed=b.vy;
            b.vy=0.f;b.falling=false;
            damageCollision(b,impactSpeed);
            if(floorIndex>=0 && impactSpeed>180.f)
                blocks_[static_cast<size_t>(floorIndex)].health-=
                    (impactSpeed-180.f)*0.13f;
            landed=true;
        }
    }
    if(landed){
        blocks_.erase(std::remove_if(blocks_.begin(),blocks_.end(),
             [](const Block& b){return b.health<=0.f;}),blocks_.end());
        updateSupport();resolveWinner();
    }
}
void World::resolveWinner(){
    bool player=false,enemy=false;
    for(const auto& b:blocks_){
        if(b.kind!=Kind::Core)continue;
        if(b.owner==0)player=true;
        else enemy=true;
    }
    if(!player||!enemy) {
        phase_=Phase::Finished;winner_=player?0:enemy?1:2;
        projectiles_.clear();
    }
}
void World::step(float dt){
    for(auto& effect:explosions_)effect.age+=dt;
    explosions_.erase(std::remove_if(explosions_.begin(),explosions_.end(),
        [](const Explosion& e){return e.age>0.85f;}),explosions_.end());
    for(auto& piece:debris_) {
        piece.life-=dt;
        piece.vy+=GRAVITY*dt;
        piece.x+=piece.vx*dt;piece.y+=piece.vy*dt;
        if(piece.y>terrain(piece.x)){piece.y=terrain(piece.x);piece.vy*=-0.22f;piece.vx*=0.72f;}
    }
    debris_.erase(std::remove_if(debris_.begin(),debris_.end(),
        [](const Debris& d){return d.life<=0.f;}),debris_.end());
    if(phase_!=Phase::Battle)return;
    timeLeft_=std::max(0.f,timeLeft_-dt);
    cooldown_=std::max(0.f,cooldown_-dt);
    botCooldown_=std::max(0.f,botCooldown_-dt);
    // Prevent projectile tunneling by sampling within a fixed 120Hz physics step.
    for(size_t i=0;i<projectiles_.size();){
        auto& p=projectiles_[i];
        p.age+=dt;
        const float vx=p.vx,vy=p.vy;
        const float distance=std::hypot(vx*dt,vy*dt);
        int sub=std::clamp(static_cast<int>(std::ceil(distance/8.f)),1,8);
        int hit=-1;bool detonateNow=false;
        for(int s=0;s<sub;++s){
            const float stepTime=dt/sub;
            p.vy+=GRAVITY*stepTime;
            p.x+=p.vx*stepTime;p.y+=p.vy*stepTime;
            for(size_t j=0;j<blocks_.size();++j){
                const auto& b=blocks_[j];
                if(overlap(p.x,p.y,p.radius*2.f,p.radius*2.f,
                    b.x,b.y,b.width,b.height)) {
                    hit=static_cast<int>(j);break;
                }
            }
            if(hit>=0 || p.y+p.radius>=terrain(p.x)) {
                detonateNow=true;break;
            }
        }
        if(detonateNow){
            const float x=p.x,y=std::min(p.y,terrain(p.x));
            const auto weapon=p.weapon;
            const int owner=p.owner;
            projectiles_.erase(projectiles_.begin()+static_cast<std::ptrdiff_t>(i));
            detonate(x,y,hit,weapon,owner);
            if(phase_==Phase::Finished)break;
        } else if(p.x>WIDTH+70.f||p.x<-70.f||p.y>HEIGHT+80.f||p.age>10.f){
            projectiles_.erase(projectiles_.begin()+static_cast<std::ptrdiff_t>(i));
        } else ++i;
    }
    settle(dt);
    if(phase_!=Phase::Battle)return;
    if(timeLeft_<=0.f){
        phase_=Phase::Finished;
        auto player=corePercent(0),enemy=corePercent(1);
        winner_=std::abs(player-enemy)<1.f?2:player>enemy?0:1;
        projectiles_.clear();return;
    }
    botTurn();
}
void World::tick(float seconds){
    if(phase_==Phase::Build||!std::isfinite(seconds)||seconds<=0.f)return;
    seconds=std::min(seconds,0.05f);
    const int count=std::max(1,static_cast<int>(std::ceil(seconds/(1.f/120.f))));
    for(int i=0;i<count;++i)step(seconds/count);
}
std::vector<float> World::snapshot()const {
    std::vector<float> result;
    result.reserve(18+blocks_.size()*9+projectiles_.size()*8+
                   explosions_.size()*4+debris_.size()*6);
    result.insert(result.end(),{3.f,static_cast<float>(phase_),
        static_cast<float>(resources_),static_cast<float>(winner_),corePercent(1),
        static_cast<float>(blocks_.size()),static_cast<float>(projectiles_.size()),
        static_cast<float>(explosions_.size()),cooldown_,static_cast<float>(ammoVolley_),
        static_cast<float>(ammoBlast_),corePercent(0),botCooldown_,
        static_cast<float>(botPlan_),timeLeft_,static_cast<float>(botShots_),
        static_cast<float>(debris_.size()),static_cast<float>(builtCount_)});
    for(const auto& b:blocks_) result.insert(result.end(),
        {b.x,b.y,b.width,b.height,static_cast<float>(b.kind),
         static_cast<float>(b.owner),b.health,b.rotation,b.falling?1.f:0.f});
    for(const auto& p:projectiles_)result.insert(result.end(),
        {p.x,p.y,p.vx,p.vy,p.radius,static_cast<float>(p.weapon),p.age,
         static_cast<float>(p.owner)});
    for(const auto& e:explosions_)result.insert(result.end(),
        {e.x,e.y,e.radius,e.age});
    for(const auto& d:debris_)result.insert(result.end(),
        {d.x,d.y,d.vx,d.vy,d.size,d.life});
    return result;
}
} // namespace siege
