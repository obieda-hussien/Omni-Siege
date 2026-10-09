#include <jni.h>
#include <mutex>
#include "siege_engine.h"

namespace {
std::mutex worldMutex;
siege::World world;
}

extern "C" JNIEXPORT void JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_reset(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(worldMutex);
    world.reset();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_placeBlock(JNIEnv*, jobject, jfloat x, jfloat y, jint kind) {
    std::lock_guard<std::mutex> lock(worldMutex);
    return world.place(x, y, kind) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_startBattle(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(worldMutex);
    world.beginBattle();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_fire(JNIEnv*, jobject, jfloat angle, jfloat speed) {
    std::lock_guard<std::mutex> lock(worldMutex);
    return world.fire(angle, speed) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_tick(JNIEnv*, jobject, jfloat seconds) {
    std::lock_guard<std::mutex> lock(worldMutex);
    world.tick(seconds);
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_snapshot(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lock(worldMutex);
    const auto frame = world.snapshot();
    jfloatArray result = env->NewFloatArray(static_cast<jsize>(frame.size()));
    if (result) env->SetFloatArrayRegion(result, 0, static_cast<jsize>(frame.size()), frame.data());
    return result;
}
