# SpectralShipGen 1.0 Compatibility Policy

SpectralShipGen `1.0.0` begins the public compatibility epoch. Development-era APIs and persistence formats that existed before 1.0 are not part of this contract.

The Library and SpectralShipGen Studio use independent Semantic Versioning. Both begin at `1.0.0` for the coordinated first public release, but later versions do not need to advance in lockstep.

## C++ public API

Public headers under `inc/SpectralShipGen/` follow Semantic Versioning beginning with 1.0.

- Patch releases may fix correctness/documentation issues without intentionally breaking supported source use.
- Minor releases may add backwards-compatible public functionality, including new fields where normal aggregate/source use remains reasonable. Callers should not depend on undocumented object layout or exhaustive assumptions about future struct members.
- A source-breaking removal, rename, signature change, or semantic incompatibility in supported public API requires a major-version change unless the affected API was explicitly documented as experimental/unstable.
- Built-in structural/faction preset convenience APIs are supported public features; presets are convenience/provenance identifiers rather than the behavior identity of custom configurations.

Binary ABI and C++ object-layout compatibility are **not guaranteed** across releases. Consumers should rebuild against the Library version they ship.

## Deprecation

When an established public API is intended for removal, the project should provide a documented replacement and preserve the deprecated API until the next major version. Deprecation is used only when there is a clear migration path; useful convenience APIs are not deprecated merely because lower-level customization exists.

## CMake package

The authoritative numeric Library version is the root CMake `PROJECT_VERSION`, currently `1.0.0`.

`SpectralShipGenConfigVersion.cmake` uses CMake's `SameMajorVersion` compatibility mode. A consumer requesting `find_package(SpectralShipGen 1.0.0 CONFIG REQUIRED)` therefore accepts compatible installed `1.x` packages and rejects `2.x` as a different major compatibility epoch.

The package version is numeric. Release/tag identity belongs to the Git revision and is not encoded separately in CMake's numeric `VERSION` field.

## Deterministic output

Within the **same exact Library release revision**, SpectralShipGen guarantees deterministic generation for the same validated semantic recipe/configuration and deterministic seed state:

```text
same exact Library release revision
+ same validated semantic recipe/configuration
+ same deterministic seed state
= pixel-identical generated FinalImage
```

Across different Library release revisions, pixel identity is **not automatically guaranteed**, even when both revisions share the same numeric CMake version or can read the same recipe schema. Correctness improvements and compatible generator evolution may change generated pixels.

When exact reproduction matters, preserve:

- the recipe/configuration;
- deterministic seed state; and
- the exact Library release tag/revision.

The numeric CMake version alone is therefore not a substitute for retaining the exact release tag/revision when pixel-identical reproduction matters.

## Recipe compatibility

Recipe schema **v6** is the SpectralShipGen 1.0 public baseline.

- Pre-1.0 recipe schemas v1-v5 are unsupported and continue to fail safely.
- Unknown future schema versions fail safely rather than being guessed or partially interpreted.
- Once a recipe schema has been emitted by a public SpectralShipGen 1.x release, later 1.x releases should continue to read that schema.
- Older Library releases are not required to understand schemas introduced by later releases.
- Dropping readability of a recipe schema emitted by a public 1.x release is a major-version compatibility event.

Recipe readability is a serialization compatibility promise; it is not a promise of cross-release pixel identity.

## Diagnostics persistence

Diagnostics `.shipdiag.json` schema **v2** is the 1.0 baseline. Pre-1.0 schema v1 and unsupported future schemas are rejected safely. Diagnostics data is developer/analysis data rather than the primary portable generated-ship interchange format; public recipes remain the portable reproduction format.

## Pre-1.0 compatibility

No compatibility is promised for private development-era APIs, recipes, migrations, aliases, or deterministic output paths removed before 1.0. The 1.0 contract starts from the cleaned baselines above.
