# SpectralShipGen

SpectralShipGen is an SFML-independent C++17 procedural pixel-art spaceship generation library.

## Requirements

- CMake 3.16 or newer
- A C++17 compiler

Core has no SFML dependency.

## Public CMake targets

- `SpectralShipGen::Core` — primary generation, configuration, recipe, validation, and animation API.
- `SpectralShipGen::Diagnostics` — optional reusable SFML-independent diagnostics backend when `SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS=ON`.

## Build options

Developer-only targets default to ON for a top-level Library checkout and OFF when SpectralShipGen is added as a subdirectory/FetchContent dependency.

- `SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS` — reusable diagnostics library, default ON.
- `SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS_CLI` — diagnostics CLI.
- `SPECTRAL_SHIP_GEN_BUILD_EXAMPLES` — public API examples.
- `SPECTRAL_SHIP_GEN_BUILD_CORE_REGRESSION` — unified Core regression runner.
- `SPECTRAL_SHIP_GEN_BUILD_PUBLIC_HEADER_CHECKS` — isolated compilation check for every public header.

The package version currently defaults to the explicit development placeholder `0.0.0`; Task 109 will establish the release version contract. It can be overridden at configure time with `SPECTRAL_SHIP_GEN_PACKAGE_VERSION` without duplicating version constants.

## Installed consumption

```cmake
find_package(SpectralShipGen CONFIG REQUIRED)
target_link_libraries(MyGame PRIVATE SpectralShipGen::Core)
```

A typical local install is:

```text
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=<prefix>
cmake --build build
cmake --install build
```

Point the consumer at the installation with `CMAKE_PREFIX_PATH=<prefix>` when it is outside a standard prefix.

## FetchContent / source consumption

Source consumers use the same public target:

```cmake
include(FetchContent)
FetchContent_Declare(SpectralShipGen
    GIT_REPOSITORY https://github.com/SpectralHorse/SpectralShipGen.git
    GIT_TAG master)
FetchContent_MakeAvailable(SpectralShipGen)
target_link_libraries(MyGame PRIVATE SpectralShipGen::Core)
```

The repository remains private during current development, so remote FetchContent relies on the developer's normal Git authentication. Release/tag policy is intentionally deferred to later release tasks.

`Tests/Consumer` is an independent acceptance project that supports both installed `find_package` consumption and a local FetchContent `SOURCE_DIR` path.
