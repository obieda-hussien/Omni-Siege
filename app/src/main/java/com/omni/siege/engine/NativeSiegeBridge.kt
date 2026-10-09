package com.omni.siege.engine
/** JNI API for the Android-only physics simulation. */
internal object NativeSiegeBridge {
    init { System.loadLibrary("omni_siege") }
    external fun reset()
    external fun placeBlock(x: Float, y: Float, kind: Int): Boolean
    external fun undoBuild(): Boolean
    external fun startBattle()
    external fun fire(angleDegrees: Float, speed: Float, weapon: Int): Boolean
    external fun tick(seconds: Float)
    external fun snapshot(): FloatArray
}
