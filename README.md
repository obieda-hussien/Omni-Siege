# Omni Siege

**Android-only portrait physics castle combat — Kotlin, Jetpack Compose and C++17 (NDK).**

## Playable now

- **Portrait** battlefield with mountains, parallax, castle rooftops, wheeled cannons, trajectory lines, explosion VFX and camera tracking
- Build your castle with wood and stone, limited resources, and Undo
- Real-time artillery with cannon, salvo and blast weapons
- **Live CPU enemy (Evo Bot v1)**: attacks from the other side, chooses tactical targets (core, supports, tall pieces, weakened blocks), tests ballistic trajectories to avoid the mountain, rotates strategies and manages limited special ammo
- Enemy can destroy your castle. Separate player/enemy health and a battle timer
- Native falling/rotating pieces, impact damage, debris and swept-substep projectile collisions

## Important honest limitations

This is **a first rule-based adaptive-tactical bot**, not a neural network and **not yet on-device training**. The native ballistic candidate search runs locally and uses the same physics terrain/observed world state. Future Evolution Lab will support headless self-play/selection on your phone after mobile profiling.

Physics is an *arcade approximation*, not a production rigid-body solver: connections are vertical, rotations are limited during falls, and particles/debris are largely cosmetic. AI does not build a castle independently yet. Multiplayer, replay persistence and production-grade assets/audio are future work.

## Development

Uses JDK 17, SDK 35, NDK 27.0.12077973, CMake 3.22.1, Gradle 8.9 and Android Gradle plugin 8.7.3.

```sh
gradle :app:assembleDebug

cmake -S app/src/main/cpp -B build-native
cmake --build build-native
ctest --test-dir build-native --output-on-failure
```

GitHub Actions builds the Debug APK and runs C++ regression tests. Download `omni-siege-debug` from its completed workflow run.

**Development priorities:** refine feel/performance on actual Android handsets, strengthen physically plausible support and fragmentation, add build drag/blueprints, complete AI defensive planning, then mobile-local self-play and evolutionary training. The game has no Internet permission.
