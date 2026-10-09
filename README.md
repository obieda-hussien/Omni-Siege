# Omni Siege

**Portrait-first Android castle siege game** built entirely with **Kotlin + Jetpack Compose + C++17 / Android NDK**.

> Current milestone: a more polished portrait **target practice** prototype. The enemy fortress is static. AI bots, on-device neural training and production-quality dynamic fracture are future milestones, not completed features.

## Portrait gameplay

Inspired by portrait physics-siege games: tall sky and layered mountains, curved ridge between combatants, rolling camera following rockets, stone-and-timber fortresses on wheeled bases, tracer smoke, explosions and debris accents. All prototype artwork is procedural Canvas rendering with no copyrighted game assets.

1. **Build**: tap on the left fortress to place blocks. Choose TIMBER or STONE, spend resources, use UNDO for your last placed block.
2. Tap **START BATTLE**.
3. **Aim**: drag across the gameplay canvas vertically for angle and horizontally for launch strength. The aiming arc shows an approximate trajectory.
4. **Choose a weapon**: CANNON (unlimited), SALVO (3 projectiles, five charges), BLAST (large area impact, two charges).
5. Tap **LAUNCH**, watch the camera track the projectiles. Use **SCOUT / HOME** to inspect the target.
6. Break the red core and tap **PLAY AGAIN**.

The entire activity is locked to **portrait**, including on Android 11. No Unity dependency.

## Architecture

- **Compose**: insets-aware game HUD, build palette, weapon cards, landscape-independent gestures, vector battle scene.
- **C++**: authoritative construction and ammo constraints, target castle, ballistics, curved collision terrain, shot cooldowns, impact damage, simplified support/collapse, effect events.
- **JNI v2**: immutable snapshot with bounded block/projectile/explosion arrays.
- **Android SDK 35**, NDK `27.0.12077973`, JDK 17, CMake 3.22.1, AGP 8.7.3, Gradle 8.9.

## Build and tests

```bash
gradle :app:assembleDebug
cmake -S app/src/main/cpp -B build-native
cmake --build build-native
ctest --test-dir build-native --output-on-failure
```

GitHub Actions builds a Debug APK and runs native tests; look for `omni-siege-debug` in the workflow artifacts.

## Current limitations (intentional)

- **Not an AI opponent yet**: the opposing castle is a practice target and never fires.
- Block collapse is vertical and support-based; rotation, bending, real material stress, collision manifolds and full destruction debris are not yet modeled.
- Generated smoke and shards are cosmetic; there are no imported graphics or sound assets yet.
- No local training, user save games, advanced blueprint editing or opponent turns yet.
- Android emulator/CI building does not substitute for performance testing on a real phone.

## Roadmap

1. Polish physics correctness, touch response, dynamic camera and visuals on real Android devices.
2. Add undoable blueprints, drag placements, larger building variety, multi-level missions, sounds and replay.
3. Add a fully fair rule-based bot using the exact same simulation observations and actions as the player.
4. Create an optional **on-device Evolution Lab** using accelerated headless simulated episodes, mutation/selection or contextual bandits first, then compact policy networks only if they outperform cheaper approaches.
5. Provide training limits for heat, battery, background work and memory.

This game needs no INTERNET permission for gameplay.
