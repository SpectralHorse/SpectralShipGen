#pragma once

#include <optional>

#include <SpectralShipGen/ShipFactionType.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipPaletteConfiguration.h>

namespace SpectralShipGen
{
    // Lightweight source metadata only. Runtime behavior uses resolved profiles,
    // animation traits and the resolved palette rather than these identifiers.
    struct ShipGenerationProvenance
    {
        std::optional<ShipStyle> StructuralPreset;
        std::optional<ShipFactionType> FactionPreset;
        ShipPaletteSourceMode PaletteSource = ShipPaletteSourceMode::FACTION_PROFILE_GENERATED;
        // Present only when FACTION_PROFILE_GENERATED colors came from a known
        // built-in faction preset. Custom faction palette language leaves this empty.
        std::optional<ShipFactionType> PaletteFactionPreset;
    };
}
