#include <PixelShipGenerator/BuiltInPresetCatalog.h>

#include <stdexcept>

namespace PixelShipGenerator
{
    namespace
    {
        constexpr std::array<BuiltInStructuralPreset, 6u> StructuralPresets = {{
            { ShipStyle::SLEEK, "SLEEK" },
            { ShipStyle::FIGHTER, "FIGHTER" },
            { ShipStyle::HEAVY, "HEAVY" },
            { ShipStyle::INDUSTRIAL, "INDUSTRIAL" },
            { ShipStyle::SPEARHEAD, "SPEARHEAD" },
            { ShipStyle::DELTA, "DELTA" }
        }};

        constexpr std::array<BuiltInFactionPreset, 6u> FactionPresets = {{
            { ShipFactionType::FRONTIER, "FRONTIER" },
            { ShipFactionType::MILITARY, "MILITARY" },
            { ShipFactionType::ASCENDANT, "ASCENDANT" },
            { ShipFactionType::XENO, "XENO" },
            { ShipFactionType::CORPORATE, "CORPORATE" },
            { ShipFactionType::RELIC, "RELIC" }
        }};

        constexpr std::array<BuiltInPalettePreset, 6u> PalettePresets = {{
            { ShipFactionType::FRONTIER, "FRONTIER" },
            { ShipFactionType::MILITARY, "MILITARY" },
            { ShipFactionType::ASCENDANT, "ASCENDANT" },
            { ShipFactionType::XENO, "XENO" },
            { ShipFactionType::CORPORATE, "CORPORATE" },
            { ShipFactionType::RELIC, "RELIC" }
        }};
    }

    const std::array<BuiltInStructuralPreset, 6u>& getBuiltInStructuralPresetCatalog() { return StructuralPresets; }
    const std::array<BuiltInFactionPreset, 6u>& getBuiltInFactionPresetCatalog() { return FactionPresets; }
    const std::array<BuiltInPalettePreset, 6u>& getBuiltInPalettePresetCatalog() { return PalettePresets; }

    std::string_view getBuiltInStructuralPresetId(ShipStyle preset)
    {
        for (const auto& entry : StructuralPresets) { if (entry.Preset == preset) { return entry.StableId; } }
        throw std::invalid_argument("Built-in structural preset lookup requires a valid ShipStyle.");
    }

    std::string_view getBuiltInFactionPresetId(ShipFactionType preset)
    {
        for (const auto& entry : FactionPresets) { if (entry.Preset == preset) { return entry.StableId; } }
        throw std::invalid_argument("Built-in faction preset lookup requires a valid ShipFactionType.");
    }

    std::string_view getBuiltInPalettePresetId(ShipFactionType preset)
    {
        for (const auto& entry : PalettePresets) { if (entry.FactionPreset == preset) { return entry.StableId; } }
        throw std::invalid_argument("Built-in palette preset lookup requires a valid ShipFactionType.");
    }

    bool tryGetBuiltInStructuralPreset(std::string_view stableId, ShipStyle& preset) noexcept
    {
        for (const auto& entry : StructuralPresets) { if (stableId == entry.StableId) { preset = entry.Preset; return true; } }
        return false;
    }

    bool tryGetBuiltInFactionPreset(std::string_view stableId, ShipFactionType& preset) noexcept
    {
        for (const auto& entry : FactionPresets) { if (stableId == entry.StableId) { preset = entry.Preset; return true; } }
        return false;
    }

    bool tryGetBuiltInPalettePreset(std::string_view stableId, ShipFactionType& preset) noexcept
    {
        for (const auto& entry : PalettePresets) { if (stableId == entry.StableId) { preset = entry.FactionPreset; return true; } }
        return false;
    }

    const ShipGenerationProfile& getBuiltInStructuralPresetProfile(ShipStyle preset) { return getShipGenerationProfile(preset); }
    const ShipFactionProfile& getBuiltInFactionPresetProfile(ShipFactionType preset) { return getShipFactionProfile(preset); }
    ShipPaletteGenerationProfile getBuiltInPalettePresetProfile(ShipFactionType preset) { return getShipPaletteGenerationProfile(preset); }
}
