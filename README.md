# Omni Siege

**Android-native physics castle siege — Kotlin + Jetpack Compose + C++/NDK.**

> **Status: playable foundation / target-practice prototype.** The AI opponent, learning, sophisticated stress solver, replay, persistence and effects are not implemented yet.

## What's playable now
1. Open the game in landscape.
2. In **Build** mode, tap the left side of the field to stack supported wooden or stone blocks. Resources and overlap constraints are enforced in the C++ engine.
3. Tap **Start battle** to enter the real-time simulation.
4. Set angle and launch speed, then tap **Fire**. Projectiles follow gravity, damage nearby blocks, and can break support connections.
5. Destroy the red training core. Tap **New arena** to restart.

There is **no combat AI** in this milestone. The red fort is an unmoving training target; the blue player has a practice cannon.

## Architecture

- **Kotlin + Jetpack Compose**: activity, responsive landscape HUD, build/tap controls, sliders, canvas rendering.
- **C++17 + NDK (JNI)**: authoritative state, build validation, ballistic simulation, radial damage, coarse connectivity-based collapse, game phase and win handling.
- **CMake**: Android shared library and separate native host tests.
- **Strings resources**: all interface copy in `app/src/main/res/values/strings.xml`.

The simulation uses bounded internal substeps (maximum 1/120 s). It is **an initial arcade physics implementation**, not yet a validated real-world stress model or full Box2D solver. Falling blocks are simplified and do not yet simulate arbitrary rotation.

## Build

Requires JDK 17, Android SDK 35, Android NDK `27.0.12077973`, CMake 3.22.1+, and Gradle 8.9. The project uses Android Gradle Plugin 8.7.3 and Kotlin 2.0.21.

With Gradle installed:

```bash
gradle :app:assembleDebug
```

Debug APK: `app/build/outputs/apk/debug/app-debug.apk`.

### Native unit tests (without Android SDK)

```bash
cmake -S app/src/main/cpp -B build-native
cmake --build build-native
ctest --test-dir build-native --output-on-failure
```

GitHub Actions builds the Android debug APK and runs native unit tests on pushes and pull requests.

## Planned implementation order
1. Improve structural physics (joints, rotation, debris, collision sweep, stress/damage) and rendering quality.
2. Add build previews, undo/redo, reusable blueprints and resource balancing.
3. Add responsive live combat, repair tools, more weapons, match timer, persistence, audio and replay.
4. **Only then** add a rules-based bot and on-device AI training arena.
5. Measure device performance (especially Android 11 low-memory devices), then introduce local self-play, compact models and optional evolution modes.

**On-device AI plan:** use headless accelerated simulations and lightweight algorithms such as mutation/selection or contextual bandits first, then test small policy networks and local gradient updates only when measured improvements justify their cost. No remote inference or compulsory PC training.

## Integrity and boundaries
- Native C++ is the source of truth for build resources and world state.
- JNI snapshot v1 contains one header and flat arrays, with bounds checking on the Kotlin side.
- Input coordinates and launch parameters are checked in native code.
- No INTERNET permission, analytics or invasive device permissions.
