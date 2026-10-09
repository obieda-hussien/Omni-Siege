package com.omni.siege.engine

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

data class SiegeBlock(
    val x:Float,val y:Float,val width:Float,val height:Float,
    val kind:Int,val owner:Int,val health:Float,
    val rotation:Float,val falling:Boolean
)
data class SiegeProjectile(
    val x:Float,val y:Float,val vx:Float,val vy:Float,
    val radius:Float,val weapon:Int,val age:Float,val owner:Int
)
data class SiegeExplosion(val x:Float,val y:Float,val radius:Float,val age:Float)
data class SiegeDebris(val x:Float,val y:Float,val vx:Float,val vy:Float,val size:Float,val life:Float)
data class SiegeSnapshot(
    val phase:Int,val resources:Int,val winner:Int,
    val enemyCorePercent:Int,val playerCorePercent:Int,
    val cooldown:Float,val volleyAmmo:Int,val blastAmmo:Int,
    val botCooldown:Float,val botPlan:Int,val timeLeft:Float,val botShots:Int,
    val placedBlocks:Int,
    val blocks:List<SiegeBlock>,val projectiles:List<SiegeProjectile>,
    val explosions:List<SiegeExplosion>,val debris:List<SiegeDebris>
) {
    companion object {
        // JNI protocol v3:
        // [version,phase,resources,winner,enemyHealth,blocks,shots,explosions,
        // playerCooldown,volleys,blasts,playerHealth,botCooldown,botPlan,
        // secondsRemaining,botShots,debrisCount,placedCount]
        fun decode(raw:FloatArray):SiegeSnapshot {
            require(raw.size>=18 && raw[0]==3f) { "Unsupported native snapshot" }
            val b=raw[5].toInt();val p=raw[6].toInt()
            val e=raw[7].toInt();val d=raw[16].toInt()
            require(b in 0..200 && p in 0..64 && e in 0..24 && d in 0..80)
            require(raw.size==18+b*9+p*8+e*4+d*6) { "Invalid native snapshot length" }
            var cursor=18
            val blocks=List(b) {
                SiegeBlock(raw[cursor],raw[cursor+1],raw[cursor+2],raw[cursor+3],
                    raw[cursor+4].toInt(),raw[cursor+5].toInt(),raw[cursor+6],
                    raw[cursor+7],raw[cursor+8]!=0f).also { cursor+=9 }
            }
            val projectiles=List(p) {
                SiegeProjectile(raw[cursor],raw[cursor+1],raw[cursor+2],raw[cursor+3],
                    raw[cursor+4],raw[cursor+5].toInt(),raw[cursor+6],
                    raw[cursor+7].toInt()).also { cursor+=8 }
            }
            val explosions=List(e) {
                SiegeExplosion(raw[cursor],raw[cursor+1],raw[cursor+2],raw[cursor+3])
                    .also { cursor+=4 }
            }
            val debris=List(d) {
                SiegeDebris(raw[cursor],raw[cursor+1],raw[cursor+2],raw[cursor+3],
                    raw[cursor+4],raw[cursor+5]).also { cursor+=6 }
            }
            return SiegeSnapshot(raw[1].toInt(),raw[2].toInt(),raw[3].toInt(),
                raw[4].toInt(),raw[11].toInt(),raw[8],raw[9].toInt(),raw[10].toInt(),
                raw[12],raw[13].toInt(),raw[14],raw[15].toInt(),raw[17].toInt(),
                blocks,projectiles,explosions,debris)
        }
    }
}
internal class SiegeSession {
    var frame by mutableStateOf(read())
        private set
    fun reset() { NativeSiegeBridge.reset();refresh() }
    fun place(x:Float,y:Float,material:Int) {
        if(NativeSiegeBridge.placeBlock(x,y,material))refresh()
    }
    fun undo() { if(NativeSiegeBridge.undoBuild())refresh() }
    fun beginBattle() { NativeSiegeBridge.startBattle();refresh() }
    fun fire(angle:Float,power:Float,weapon:Int) {
        if(NativeSiegeBridge.fire(angle,power,weapon))refresh()
    }
    fun tick(dt:Float) {
        if(frame.phase!=0) { NativeSiegeBridge.tick(dt);refresh() }
    }
    private fun read()=SiegeSnapshot.decode(NativeSiegeBridge.snapshot())
    private fun refresh() { frame=read() }
}
