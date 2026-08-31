#pragma once

#include <array>
#include <string_view>

#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipPaletteGenerationProfile.h>

namespace SpectralShipGen
{
    struct BuiltInStructuralPreset
    {
        ShipStyle Preset = ShipStyle::FIGHTER;
        const char* StableId = "FIGHTER";
    };

    struct BuiltInFactionPreset
    {
        ShipFactionType Preset = ShipFactionType::FRONTIER;
        const char* StableId = "FRONTIER";
    };

    // Built-in palette languages currently correspond one-to-one with the
    // canonical built-in faction palette languages. The profile returned is the
    // same public ShipPaletteGenerationProfile used by custom callers.
    struct BuiltInPalettePreset
    {
        ShipFactionType FactionPreset = ShipFactionType::FRONTIER;
        const char* StableId = "FRONTIER";
    };

    const std::array<BuiltInStructuralPreset, 6u>& getBuiltInStructuralPresetCatalog();
    const std::array<BuiltInFactionPreset, 6u>& getBuiltInFactionPresetCatalog();
    const std::array<BuiltInPalettePreset, 6u>& getBuiltInPalettePresetCatalog();

    std::string_view getBuiltInStructuralPresetId(ShipStyle preset);
    std::string_view getBuiltInFactionPresetId(ShipFactionType preset);
    std::string_view getBuiltInPalettePresetId(ShipFactionType preset);
    bool tryGetBuiltInStructuralPreset(std::string_view stableId, ShipStyle& preset) noexcept;
    bool tryGetBuiltInFactionPreset(std::string_view stableId, ShipFactionType& preset) noexcept;
    bool tryGetBuiltInPalettePreset(std::string_view stableId, ShipFactionType& preset) noexcept;

    const ShipGenerationProfile& getBuiltInStructuralPresetProfile(ShipStyle preset);
    const ShipFactionProfile& getBuiltInFactionPresetProfile(ShipFactionType preset);
    ShipPaletteGenerationProfile getBuiltInPalettePresetProfile(ShipFactionType preset);
}
