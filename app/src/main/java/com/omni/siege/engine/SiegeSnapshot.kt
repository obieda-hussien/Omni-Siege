package com.omni.siege.engine

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

data class SiegeBlock(
    val x: Float, val y: Float, val width: Float, val height: Float,
    val kind: Int, val owner: Int, val health: Float
)

data class SiegeProjectile(val x: Float, val y: Float, val radius: Float)

data class SiegeSnapshot(
    val phase: Int,
    val resources: Int,
    val winner: Int,
    val enemyCorePercent: Int,
    val blocks: List<SiegeBlock>,
    val projectiles: List<SiegeProjectile>
) {
    companion object {
        // [version, phase, resources, winner, core%, countBlocks, countProjectiles]
        fun decode(raw: FloatArray): SiegeSnapshot {
            require(raw.size >= 7 && raw[0] == 1f) { "Invalid native snapshot version" }
            val blocksCount = raw[5].toInt()
            val projectilesCount = raw[6].toInt()
            require(blocksCount in 0..512 && projectilesCount in 0..128)
            require(raw.size == 7 + blocksCount * 7 + projectilesCount * 3)
            var cursor = 7
            val blocks = List(blocksCount) {
                val block = SiegeBlock(
                    raw[cursor], raw[cursor + 1], raw[cursor + 2], raw[cursor + 3],
                    raw[cursor + 4].toInt(), raw[cursor + 5].toInt(), raw[cursor + 6]
                )
                cursor += 7
                block
            }
            val projectiles = List(projectilesCount) {
                val projectile = SiegeProjectile(raw[cursor], raw[cursor + 1], raw[cursor + 2])
                cursor += 3
                projectile
            }
            return SiegeSnapshot(
                raw[1].toInt(), raw[2].toInt(), raw[3].toInt(), raw[4].toInt(),
                blocks, projectiles
            )
        }
    }
}

/** Only one simulation session is hosted in this activity. Native calls run on the UI thread. */
internal class SiegeSession {
    var frame by mutableStateOf(read())
        private set

    fun reset() {
        NativeSiegeBridge.reset()
        refresh()
    }

    fun place(x: Float, y: Float, material: Int) {
        if (NativeSiegeBridge.placeBlock(x, y, material)) refresh()
    }

    fun startBattle() {
        NativeSiegeBridge.startBattle()
        refresh()
    }

    fun fire(angle: Float, power: Float) {
        NativeSiegeBridge.fire(angle, power)
        refresh()
    }

    fun tick(dt: Float) {
        if (frame.phase == 1) {
            NativeSiegeBridge.tick(dt)
            refresh()
        }
    }

    private fun read() = SiegeSnapshot.decode(NativeSiegeBridge.snapshot())
    private fun refresh() { frame = read() }
}
