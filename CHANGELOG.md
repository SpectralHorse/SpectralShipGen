# Changelog

All notable public changes to SpectralShipGen are recorded here beginning with the 1.0 compatibility epoch.

## [1.0.0]

Initial public release.

### Generation

- Deterministic native-resolution pixel-art spaceship generation in C++17 with explicit seed state.
- Built-in structural presets (`SLEEK`, `FIGHTER`, `HEAVY`, `INDUSTRIAL`, `SPEARHEAD`, `DELTA`) and faction presets (`FRONTIER`, `MILITARY`, `ASCENDANT`, `XENO`, `CORPORATE`, `RELIC`).
- First-class custom structural, faction, and palette configuration without fabricated built-in identity.
- Deterministic generation domains and selective reroll support.

### Recipes and compatibility

- Public self-contained recipe format with schema v6 as the 1.0 baseline.
- Safe rejection of unsupported pre-1.0 and future recipe versions.
- Explicit 1.0 source-API, CMake package, recipe, and deterministic-output compatibility policy.

### Animation

- IDLE, directional movement, FIRE, and movement-posture + FIRE composition.
- Adaptive sampling and normalized semantic-time evaluation.

### Diagnostics

- Reusable SFML-independent diagnostics/statistics backend.
- Diagnostics persistence schema v2 as the 1.0 baseline.

### Build and consumption

- Public CMake targets `SpectralShipGen::Core` and optional `SpectralShipGen::Diagnostics`.
- Installable CMake package with `SameMajorVersion` compatibility.
- Eight runnable public API examples and installed/source-consumer validation paths.
- zlib licensing; generated output is owned/usable by the end user under the generated-output policy documented in the README.

Pre-public development history is intentionally summarized rather than reproduced as an internal task diary.
