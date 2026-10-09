package com.omni.siege.engine
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

data class SiegeBlock(
    val x: Float, val y: Float, val width: Float, val height: Float,
    val kind: Int, val owner: Int, val health: Float
)
data class SiegeProjectile(
    val x: Float, val y: Float, val vx: Float, val vy: Float,
    val radius: Float, val weapon: Int, val age: Float
)
data class SiegeExplosion(val x: Float, val y: Float, val radius: Float, val age: Float)
data class SiegeSnapshot(
    val phase: Int, val resources: Int, val winner: Int, val enemyCorePercent: Int,
    val cooldown: Float, val volleyAmmo: Int, val blastAmmo: Int,
    val blocks: List<SiegeBlock>,
    val projectiles: List<SiegeProjectile>,
    val explosions: List<SiegeExplosion>
) {
    companion object {
        // v2: [version,phase,resources,winner,enemyHp,blocks,shots,effects,cooldown,volleys,blasts]
        fun decode(raw: FloatArray): SiegeSnapshot {
            require(raw.size >= 11 && raw[0] == 2f) { "Invalid native scene format" }
            val b=raw[5].toInt()
            val p=raw[6].toInt()
            val e=raw[7].toInt()
            require(b in 0..200 && p in 0..64 && e in 0..24)
            require(raw.size == 11+b*7+p*7+e*4)
            var cursor=11
            val blocks=List(b) {
                SiegeBlock(raw[cursor],raw[cursor+1],raw[cursor+2],raw[cursor+3],
                    raw[cursor+4].toInt(),raw[cursor+5].toInt(),raw[cursor+6])
                    .also { cursor+=7 }
            }
            val shots=List(p) {
                SiegeProjectile(raw[cursor],raw[cursor+1],raw[cursor+2],raw[cursor+3],
                    raw[cursor+4],raw[cursor+5].toInt(),raw[cursor+6])
                    .also { cursor+=7 }
            }
            val effects=List(e) {
                SiegeExplosion(raw[cursor],raw[cursor+1],raw[cursor+2],raw[cursor+3])
                    .also { cursor+=4 }
            }
            return SiegeSnapshot(raw[1].toInt(),raw[2].toInt(),raw[3].toInt(),
                raw[4].toInt(),raw[8],raw[9].toInt(),raw[10].toInt(),blocks,shots,effects)
        }
    }
}
internal class SiegeSession {
    var frame by mutableStateOf(read())
        private set
    fun reset() { NativeSiegeBridge.reset(); refresh() }
    fun place(x: Float,y: Float,material: Int) {
        if (NativeSiegeBridge.placeBlock(x,y,material)) refresh()
    }
    fun undo() { if (NativeSiegeBridge.undoBuild()) refresh() }
    fun beginBattle() { NativeSiegeBridge.startBattle(); refresh() }
    fun fire(angle: Float,power: Float,weapon: Int) {
        if (NativeSiegeBridge.fire(angle,power,weapon)) refresh()
    }
    fun tick(dt: Float) {
        if (frame.phase != 0) {
            NativeSiegeBridge.tick(dt)
            refresh()
        }
    }
    private fun read()=SiegeSnapshot.decode(NativeSiegeBridge.snapshot())
    private fun refresh() { frame=read() }
}
