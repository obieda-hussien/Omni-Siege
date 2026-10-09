#include <jni.h>
#include <mutex>
#include "siege_engine.h"
namespace {
std::mutex guard;
siege::World simulation;
}
extern "C" JNIEXPORT void JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_reset(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(guard); simulation.reset();
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_placeBlock(JNIEnv*, jobject, jfloat x,jfloat y,jint kind) {
    std::lock_guard<std::mutex> lock(guard);
    return simulation.place(x,y,kind)?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_undoBuild(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(guard);
    return simulation.undoBuild()?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT void JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_startBattle(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(guard);simulation.beginBattle();
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_fire(JNIEnv*, jobject,jfloat angle,jfloat speed,jint weapon) {
    std::lock_guard<std::mutex> lock(guard);
    return simulation.fire(angle,speed,weapon)?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT void JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_tick(JNIEnv*, jobject,jfloat seconds) {
    std::lock_guard<std::mutex> lock(guard);simulation.tick(seconds);
}
extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_omni_siege_engine_NativeSiegeBridge_snapshot(JNIEnv* env,jobject) {
    std::lock_guard<std::mutex> lock(guard);
    const auto data=simulation.snapshot();
    auto result=env->NewFloatArray(static_cast<jsize>(data.size()));
    if (result) env->SetFloatArrayRegion(result,0,static_cast<jsize>(data.size()),data.data());
    return result;
}
