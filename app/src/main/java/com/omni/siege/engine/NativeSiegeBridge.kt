package com.omni.siege.engine

/**
 * JNI boundary: the C++ simulation owns state; Compose only renders immutable snapshots.
 * Snapshot format is versioned in [SiegeSnapshot].
 */
internal object NativeSiegeBridge {
    init {
        System.loadLibrary("omni_siege")
    }

    external fun reset()
    external fun placeBlock(x: Float, y: Float, kind: Int): Boolean
    external fun startBattle()
    external fun fire(angleDegrees: Float, speed: Float): Boolean
    external fun tick(seconds: Float)
    external fun snapshot(): FloatArray
}
