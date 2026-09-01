# SpectralShipGen

SpectralShipGen is an SFML-independent C++17 library for deterministic native-resolution pixel-art spaceship generation and animation.

The Library owns generation, public configuration, recipes, deterministic rerolls, animation, validation, and the reusable diagnostics backend. The separate [SpectralShipGen Studio](https://github.com/SpectralHorse/SpectralShipGen-Studio) application consumes this public API; Studio is not required to use the Library.

## Requirements

- CMake 3.16 or newer
- a C++17 compiler
- no SFML dependency for `SpectralShipGen::Core`

## Public CMake targets

- `SpectralShipGen::Core` — generation, configuration, recipes, validation, rerolls, and animation.
- `SpectralShipGen::Diagnostics` — optional reusable SFML-independent diagnostics backend when `SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS=ON`.

## Quick start

```cpp
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>

int main()
{
    SpectralShipGen::ShipGenerationSettings settings;
    settings.Seed = 1234u;
    settings.Dimensions = { 64u, 64u };
    settings.Style = SpectralShipGen::ShipStyle::FIGHTER;
    settings.Faction = SpectralShipGen::ShipFactionType::MILITARY;

    const SpectralShipGen::GeneratedShip ship = SpectralShipGen::ShipGenerator{}.generate(settings);
    return ship.FinalImage.empty() ? 1 : 0;
}
```

Within the same exact SpectralShipGen Library release revision, the same validated semantic configuration and deterministic seed state produce pixel-identical output. See [`COMPATIBILITY.md`](COMPATIBILITY.md) for the cross-version contract.

## Build and install

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build -L normal --output-on-failure
cmake --install build --prefix <install-prefix>
```

Installed consumption:

```cmake
find_package(SpectralShipGen 1.0.0 CONFIG REQUIRED)
target_link_libraries(MyGame PRIVATE SpectralShipGen::Core)
```

If the install prefix is non-standard, pass it through `CMAKE_PREFIX_PATH` when configuring the consumer.

Source/FetchContent consumption uses the same target:

```cmake
include(FetchContent)
FetchContent_Declare(SpectralShipGen
    GIT_REPOSITORY https://github.com/SpectralHorse/SpectralShipGen.git
    GIT_TAG v1.0.0)
FetchContent_MakeAvailable(SpectralShipGen)

target_link_libraries(MyGame PRIVATE SpectralShipGen::Core)
```

For the public 1.0 release, source-fetch examples pin the immutable `v1.0.0` tag. SpectralShipGen and Studio version independently after their coordinated initial 1.0 release.

## Build options

Developer-only targets default to ON for a top-level Library checkout and OFF when SpectralShipGen is consumed as a subdirectory/FetchContent dependency.

- `SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS` — reusable diagnostics library.
- `SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS_CLI` — diagnostics CLI.
- `SPECTRAL_SHIP_GEN_BUILD_EXAMPLES` — runnable public API examples.
- `SPECTRAL_SHIP_GEN_BUILD_CORE_REGRESSION` — unified Core regression runner.
- `SPECTRAL_SHIP_GEN_BUILD_PUBLIC_HEADER_CHECKS` — isolated compilation check for every public header.

The authoritative numeric package version is the root CMake `PROJECT_VERSION` (`1.0.0`). The generated package version file uses `SameMajorVersion` compatibility.

## CI and regression policy

The repository keeps routine and expensive validation separate:

- `.github/workflows/library-ci.yml` runs Windows/MSVC and Linux/GCC builds, Core NORMAL regression, installation, installed-package consumption, and a separate source-consumer check.
- `.github/workflows/library-sanitizers.yml` runs the project-owned Library code under Clang ASan+UBSan on Linux. `SPECTRAL_SHIP_GEN_ENABLE_SANITIZERS=ON` is a development/CI-only option and is not part of the installed package interface.
- `.github/workflows/library-long.yml` runs the full NORMAL+LONG regression set only by manual dispatch and on the weekly schedule. LONG sample counts and many-seed matrices are intentionally preserved.

Local test labels remain `normal` and `long`, so focused CTest runs can use `-L normal` or `-L long`.

## Public configuration model

SpectralShipGen separates *what a ship should be like* from the exact generated ship.

### `ShipGenerationProfile` — structural/design language

Controls structural form: silhouette ranges, wings, cockpit tendencies, engines, major features, weapons, attachments, negative space, complexity, spatial capacity, and structural animation traits.

`Chance` fields are probabilities. `Weight` fields are relative selection weights and do not need to sum to 100. Multiplier-style `Percent` fields use `100` as baseline and may validly exceed 100 where the public validator permits it.

Built-in structural presets are convenient starting values:

- `SLEEK`
- `FIGHTER`
- `HEAVY`
- `INDUSTRIAL`
- `SPEARHEAD`
- `DELTA`

You can use a preset directly, copy and modify its public profile, or construct a fully custom profile.

### `ShipFactionProfile` — technological/material/cultural language

Controls faction-level composition over the structural design: material and finish tendencies, weapon language, engine/cockpit preferences, details, livery, palette behavior, hierarchy, and animation response.

Built-in faction presets are:

- `FRONTIER`
- `MILITARY`
- `ASCENDANT`
- `XENO`
- `CORPORATE`
- `RELIC`

A custom faction profile does not need a fabricated built-in faction identity.

### `ShipPaletteConfiguration` — color source

Palette selection is independent from structural and faction selection:

- `FACTION_PROFILE_GENERATED` — derive colors from the supplied faction profile.
- `EXPLICIT_GENERATED` — use a caller-supplied `ShipPaletteGenerationProfile`.
- `FIXED` — use an exact semantic `ShipPalette` supplied by the caller.

A fully custom generation can therefore combine custom structural + custom faction + custom palette values with no built-in identity requirement.

### Built-ins are defaults, not an identity matrix

Structural presets and faction presets are independent reusable inputs. SpectralShipGen does not require a fixed style × faction matrix. Built-ins are useful defaults and sources to copy; explicit public values are first-class generation inputs.

## Recipes

`ShipGenerationRecipe` is a portable, self-contained definition of one reproducible generation state. Built-in sources retain truthful preset provenance; custom structural/faction sources embed their public profile values. Palette configuration and resolved deterministic seed state are carried with the recipe.

Typical flow:

```text
generation configuration
    -> makeShipGenerationRecipe(...)
    -> serializeShipGenerationRecipe(...)
    -> deserializeShipGenerationRecipe(...)
    -> ShipGenerator::generate(recipe)
```

Recipes do not depend on SpectralShipGen Studio's local profile library or Favorites database.

See [`Examples/recipe_round_trip.cpp`](Examples/recipe_round_trip.cpp).

## Deterministic domains and selective rerolls

Generation uses deterministic domains such as hull, wings, cockpit, engines, weapons, palette, details, and attachments. `rerollGenerationDomains(...)` derives a new recipe while changing only selected domain seed state.

For example, a Palette-domain reroll changes generated colors while preserving geometry. Domain relationships remain part of the deterministic Core model; callers do not need to reproduce internal RNG implementation details.

See [`Examples/deterministic_reroll.cpp`](Examples/deterministic_reroll.cpp).

## Animation

Animation is generated from the resolved `GeneratedShip`; it does not require the original built-in style/faction identity.

Public animation families include:

- `IDLE`
- `MOVE_LEFT`
- `MOVE_RIGHT`
- `MOVE_UP`
- `MOVE_DOWN`
- `FIRE`

IDLE and movement use adaptive sampling by default, so frame counts are derived from the animated ship and configured bounds rather than one global fixed count. Normalized semantic time remains the stable evaluation input.

`ShipAnimationStateCoordinator` composes a transient FIRE event over an underlying movement posture instead of requiring bespoke combined clips such as `MOVE_LEFT_FIRE`.

See [`Examples/animation.cpp`](Examples/animation.cpp).

## Public diagnostics

`SpectralShipGen::Diagnostics` provides the reusable SFML-independent diagnostics backend for generation statistics and analysis. The separate Studio repository owns the SFML dashboard/application layer.

## Runnable examples

The focused examples under [`Examples/`](Examples/) all link only `SpectralShipGen::Core`:

- [`built_in_generation.cpp`](Examples/built_in_generation.cpp) — simplest built-in path.
- [`custom_structural_profile.cpp`](Examples/custom_structural_profile.cpp) — copy, modify, validate, generate a structural profile.
- [`custom_faction_profile.cpp`](Examples/custom_faction_profile.cpp) — focused custom faction profile.
- [`palette_configuration.cpp`](Examples/palette_configuration.cpp) — explicit generated and fixed palettes.
- [`fully_custom_generation.cpp`](Examples/fully_custom_generation.cpp) — custom structural + faction + palette without built-in identity selectors.
- [`recipe_round_trip.cpp`](Examples/recipe_round_trip.cpp) — portable recipe serialization and regeneration.
- [`animation.cpp`](Examples/animation.cpp) — IDLE, movement, and movement + FIRE composition.
- [`deterministic_reroll.cpp`](Examples/deterministic_reroll.cpp) — selective Palette-domain reroll.

With `SPECTRAL_SHIP_GEN_BUILD_EXAMPLES=ON`, build the aggregate target:

```text
cmake --build build --target SpectralShipGenExamples
```

`Examples/CMakeLists.txt` can also be configured as a standalone project against an installed package, proving the examples do not depend on private Library source paths:

```text
cmake -S Examples -B examples-build -DCMAKE_PREFIX_PATH=<install-prefix>
cmake --build examples-build
```

## Versioning and compatibility

SpectralShipGen `1.0.0` begins the public compatibility epoch. Public C++ source API follows Semantic Versioning, while binary ABI/object-layout stability is not promised. Recipe schema v6 and diagnostics schema v2 are the 1.0 baselines. Pre-1.0 development APIs/formats remain unsupported.

See [`COMPATIBILITY.md`](COMPATIBILITY.md) for the complete source API, CMake package, deprecation, recipe, diagnostics, and deterministic-output contract.

## Generated output rights

Images, spritesheets, animation frames, and other output generated with SpectralShipGen may be used for **any purpose**, including commercial use. You may modify, redistribute, publish, sell, incorporate, paint over, or otherwise use generated output with **no attribution requirement**. The zlib license governs the software source, not generated output.

Showing the project an interesting or funny generated result is always welcome, but entirely optional.

## Repository relationship

- **SpectralShipGen** — this repository; reusable C++17 Library, SFML-independent.
- **[SpectralShipGen Studio](https://github.com/SpectralHorse/SpectralShipGen-Studio)** — separate SFML GUI/application consuming the Library through its public CMake targets.

The dependency direction is Studio → Library only.

## License

SpectralShipGen is licensed under the **zlib License**. See [`LICENSE`](LICENSE).

The recipe/serialization JSON implementation is project-owned code inside the Library; SpectralShipGen does not bundle or require a third-party JSON library.

