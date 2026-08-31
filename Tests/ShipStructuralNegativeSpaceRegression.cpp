#include "CoreRegressionSuites.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <queue>
#include <vector>

#include <SpectralShipGen/GenerationComplexityBudget.h>
#include <SpectralShipGen/GenerationSpatialBudget.h>
#include <SpectralShipGen/ShipFactionType.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipStructuralNegativeSpaceType.h>

namespace
{
    using namespace SpectralShipGen;

    struct NegativeSpaceFixture
    {
        ShipStructuralNegativeSpaceType Type;
        uint64_t Seed;
        ShipFactionType Faction;
    };

    constexpr std::array<NegativeSpaceFixture, 5u> Fixtures = { {
        { ShipStructuralNegativeSpaceType::WING_CHANNEL,    0x5700030040400010ull, ShipFactionType::CORPORATE },
        { ShipStructuralNegativeSpaceType::REAR_FORK,       0x5700030040400019ull, ShipFactionType::MILITARY },
        { ShipStructuralNegativeSpaceType::SHOULDER_GAP,    0x5700030040400034ull, ShipFactionType::CORPORATE },
        { ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY,  0x570003004040000Cull, ShipFactionType::FRONTIER },
        { ShipStructuralNegativeSpaceType::NACELLE_CHANNEL, 0x5700030040400017ull, ShipFactionType::RELIC }
    } };

    constexpr std::array<ShipDimensions, 7u> ReviewDimensions = { {
        {24u,24u}, {32u,32u}, {44u,44u}, {48u,64u}, {64u,48u}, {64u,64u}, {96u,96u}
    } };

    uint32_t countPixels(const PixelMask& mask)
    {
        uint32_t count = 0u;
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
                count += mask.get(x, y) ? 1u : 0u;
        return count;
    }

    bool masksEqual(const PixelMask& first, const PixelMask& second)
    {
        if (first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight()) { return false; }
        for (uint32_t y = 0u; y < first.getHeight(); ++y)
            for (uint32_t x = 0u; x < first.getWidth(); ++x)
                if (first.get(x, y) != second.get(x, y)) { return false; }
        return true;
    }

    bool isSymmetric(const PixelMask& mask)
    {
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
                if (mask.get(x, y) != mask.get(mask.getWidth() - 1u - x, y)) { return false; }
        return true;
    }

    uint32_t countConnectedComponents(const PixelMask& mask)
    {
        const uint32_t width = mask.getWidth();
        const uint32_t height = mask.getHeight();
        std::vector<uint8_t> visited(static_cast<std::size_t>(width) * height, 0u);
        constexpr std::array<int32_t, 4u> DX = { -1, 1, 0, 0 };
        constexpr std::array<int32_t, 4u> DY = { 0, 0, -1, 1 };
        uint32_t components = 0u;

        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                const std::size_t startIndex = static_cast<std::size_t>(y) * width + x;
                if (!mask.get(x, y) || visited[startIndex]) { continue; }
                ++components;
                std::queue<std::pair<uint32_t, uint32_t>> pending;
                pending.emplace(x, y);
                visited[startIndex] = 1u;
                while (!pending.empty())
                {
                    const auto [px, py] = pending.front();
                    pending.pop();
                    for (std::size_t direction = 0u; direction < DX.size(); ++direction)
                    {
                        const int32_t nx = static_cast<int32_t>(px) + DX[direction];
                        const int32_t ny = static_cast<int32_t>(py) + DY[direction];
                        if (nx < 0 || ny < 0 || nx >= static_cast<int32_t>(width) || ny >= static_cast<int32_t>(height)) { continue; }
                        const std::size_t index = static_cast<std::size_t>(ny) * width + static_cast<uint32_t>(nx);
                        if (visited[index] || !mask.get(static_cast<uint32_t>(nx), static_cast<uint32_t>(ny))) { continue; }
                        visited[index] = 1u;
                        pending.emplace(static_cast<uint32_t>(nx), static_cast<uint32_t>(ny));
                    }
                }
            }
        }
        return components;
    }

    bool overlaps(const PixelMask& first, const PixelMask& second)
    {
        if (first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight()) { return false; }
        for (uint32_t y = 0u; y < first.getHeight(); ++y)
            for (uint32_t x = 0u; x < first.getWidth(); ++x)
                if (first.get(x, y) && second.get(x, y)) { return true; }
        return false;
    }

    bool reservedPixelsAreOutsideSpatialMap(const ShipGenerationDebugInfo& debug)
    {
        if (debug.SpatialRegionMapWidth != debug.ReservedNegativeSpaceMask.getWidth() || debug.SpatialRegionMapHeight != debug.ReservedNegativeSpaceMask.getHeight()) { return false; }
        if (debug.SpatialRegionMap.size() != static_cast<std::size_t>(debug.SpatialRegionMapWidth) * debug.SpatialRegionMapHeight) { return false; }
        const uint8_t emptyRegion = static_cast<uint8_t>(GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END);
        for (uint32_t y = 0u; y < debug.SpatialRegionMapHeight; ++y)
            for (uint32_t x = 0u; x < debug.SpatialRegionMapWidth; ++x)
                if (debug.ReservedNegativeSpaceMask.get(x, y) && debug.SpatialRegionMap[static_cast<std::size_t>(y) * debug.SpatialRegionMapWidth + x] != emptyRegion) { return false; }
        return true;
    }

    bool engineRootsAreValid(const GeneratedShip& ship, const ShipGenerationDebugInfo& debug)
    {
        for (const EngineUnitDebugInfo& unit : debug.EngineUnits)
        {
            if (unit.HousingWidth == 0u || unit.RootStartY >= ship.HullMask.getHeight() || unit.HousingStartX + unit.HousingWidth > ship.HullMask.getWidth()) { return false; }
            for (uint32_t x = unit.HousingStartX; x < unit.HousingStartX + unit.HousingWidth; ++x)
                if (!ship.HullMask.get(x, unit.RootStartY)) { return false; }
        }
        return true;
    }

    bool profileTendenciesAreDistinct()
    {
        const ShipGenerationProfile sleek = getShipGenerationProfile(ShipStyle::SLEEK);
        const ShipGenerationProfile fighter = getShipGenerationProfile(ShipStyle::FIGHTER);
        const ShipGenerationProfile industrial = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        const ShipGenerationProfile spearhead = getShipGenerationProfile(ShipStyle::SPEARHEAD);
        const ShipGenerationProfile delta = getShipGenerationProfile(ShipStyle::DELTA);
        return sleek.StructuralNegativeSpaceChance < fighter.StructuralNegativeSpaceChance &&
            industrial.StructuralNegativeSpaceChance > fighter.StructuralNegativeSpaceChance &&
            delta.StructuralNegativeSpaceChance > fighter.StructuralNegativeSpaceChance &&
            industrial.StructuralNegativeSpaceWeights.OpenFrameBay > sleek.StructuralNegativeSpaceWeights.OpenFrameBay &&
            industrial.StructuralNegativeSpaceWeights.NacelleChannel > sleek.StructuralNegativeSpaceWeights.NacelleChannel &&
            spearhead.StructuralNegativeSpaceWeights.RearFork > spearhead.StructuralNegativeSpaceWeights.WingChannel &&
            delta.StructuralNegativeSpaceWeights.WingChannel > delta.StructuralNegativeSpaceWeights.RearFork &&
            delta.StructuralNegativeSpaceWeights.ShoulderGap > delta.StructuralNegativeSpaceWeights.RearFork;
    }
}

int SpectralShipGenTests::runStructuralNegativeSpaceRegression()
{
    using namespace SpectralShipGen;
    if (!profileTendenciesAreDistinct())
    {
        std::cerr << "Task 57 regression failed: style-specific negative-space profile tendencies collapsed.\n";
        return 1;
    }

    ShipGenerator generator;

    // Stable fixtures give deterministic coverage of every semantic void type.
    for (const NegativeSpaceFixture& fixture : Fixtures)
    {
        ShipGenerationSettings settings;
        settings.Seed = fixture.Seed;
        settings.Dimensions = { 64u,64u };
        settings.Style = ShipStyle::INDUSTRIAL;
        settings.Faction = fixture.Faction;
        ShipGenerationDebugInfo firstDebug;
        ShipGenerationDebugInfo secondDebug;
        const GeneratedShip first = generator.generate(settings, &firstDebug);
        const GeneratedShip second = generator.generate(settings, &secondDebug);
        const std::size_t typeIndex = static_cast<std::size_t>(fixture.Type);

        if (firstDebug.StructuralNegativeSpaceTypeCounts[typeIndex] == 0u || firstDebug.StructuralNegativeSpaceCount == 0u || firstDebug.StructuralNegativeSpacePixelCount == 0u)
        {
            std::cerr << "Task 57 regression failed: semantic negative-space fixture no longer produces " << getShipStructuralNegativeSpaceTypeName(fixture.Type) << ".\n";
            return 1;
        }
        if (first.FinalImage.getPixels() != second.FinalImage.getPixels() || !masksEqual(firstDebug.ReservedNegativeSpaceMask, secondDebug.ReservedNegativeSpaceMask) || firstDebug.StructuralNegativeSpaceTypeCounts != secondDebug.StructuralNegativeSpaceTypeCounts)
        {
            std::cerr << "Task 57 regression failed: structural negative space is not deterministic.\n";
            return 1;
        }
        if (countPixels(firstDebug.ReservedNegativeSpaceMask) != firstDebug.StructuralNegativeSpacePixelCount || !isSymmetric(firstDebug.ReservedNegativeSpaceMask) || countConnectedComponents(first.HullMask) != 1u)
        {
            std::cerr << "Task 57 regression failed: reserved void/connectivity semantics are invalid.\n";
            return 1;
        }
        if (overlaps(firstDebug.ReservedNegativeSpaceMask, first.HullMask) || overlaps(firstDebug.ReservedNegativeSpaceMask, first.CockpitMask) || overlaps(firstDebug.ReservedNegativeSpaceMask, first.EngineMask) || overlaps(firstDebug.ReservedNegativeSpaceMask, first.EngineExhaustMask) || overlaps(firstDebug.ReservedNegativeSpaceMask, first.AttachmentMask) || overlaps(firstDebug.ReservedNegativeSpaceMask, firstDebug.WeaponOccupiedMask))
        {
            std::cerr << "Task 57 regression failed: a later structural component filled reserved negative space.\n";
            return 1;
        }
        if (!reservedPixelsAreOutsideSpatialMap(firstDebug) || !engineRootsAreValid(first, firstDebug) || firstDebug.LastSilhouetteValidationFailure != SilhouetteValidationFailureReason::NONE)
        {
            std::cerr << "Task 57 regression failed: spatial/engine/silhouette compatibility is invalid.\n";
            return 1;
        }
        if (firstDebug.ComplexityCategoryConsumed[static_cast<std::size_t>(GenerationComplexityCategory::SILHOUETTE)] == 0u)
        {
            std::cerr << "Task 57 regression failed: semantic negative space did not consume silhouette complexity budget.\n";
            return 1;
        }

        if ((fixture.Type == ShipStructuralNegativeSpaceType::REAR_FORK || fixture.Type == ShipStructuralNegativeSpaceType::NACELLE_CHANNEL) && firstDebug.EngineCount >= 2u && countConnectedComponents(first.EngineExhaustMask) < 2u)
        {
            std::cerr << "Task 57 regression failed: separated rear architecture collapsed its exhaust paths.\n";
            return 1;
        }
        if (fixture.Type == ShipStructuralNegativeSpaceType::NACELLE_CHANNEL)
        {
            const bool hasStructuralNacelle = std::any_of(firstDebug.EngineUnits.begin(), firstDebug.EngineUnits.end(), [](const EngineUnitDebugInfo& unit) { return unit.Nacelle; });
            if (!hasStructuralNacelle)
            {
                std::cerr << "Task 57 regression failed: nacelle channel did not preserve separated nacelle presentation.\n";
                return 1;
            }
        }
    }

    // All styles, factions and requested native/rectangular dimensions remain deterministic and structurally valid.
    for (uint32_t styleIndex = 0u; styleIndex < static_cast<uint32_t>(ShipStyle::SHIP_STYLE_END); ++styleIndex)
    {
        uint64_t rejectionCount = 0u;
        for (std::size_t dimensionIndex = 0u; dimensionIndex < ReviewDimensions.size(); ++dimensionIndex)
        {
            ShipGenerationSettings settings;
            settings.Seed = 0x57A11E0000000000ull + static_cast<uint64_t>(styleIndex) * 0x100000ull + dimensionIndex;
            settings.Dimensions = ReviewDimensions[dimensionIndex];
            settings.Style = static_cast<ShipStyle>(styleIndex);
            settings.Faction = static_cast<ShipFactionType>((styleIndex + dimensionIndex) % static_cast<uint32_t>(ShipFactionType::SHIP_FACTION_TYPE_END));
            ShipGenerationDebugInfo firstDebug;
            ShipGenerationDebugInfo secondDebug;
            const GeneratedShip first = generator.generate(settings, &firstDebug);
            const GeneratedShip second = generator.generate(settings, &secondDebug);
            rejectionCount += firstDebug.HullValidationRejectionCount;

            if (first.FinalImage.getPixels() != second.FinalImage.getPixels() || countConnectedComponents(first.HullMask) != 1u || !isSymmetric(first.HullMask) || !masksEqual(firstDebug.ReservedNegativeSpaceMask, secondDebug.ReservedNegativeSpaceMask) || firstDebug.LastSilhouetteValidationFailure != SilhouetteValidationFailureReason::NONE)
            {
                std::cerr << "Task 57 regression failed: style/dimension deterministic structural validation failed.\n";
                return 1;
            }
            if (firstDebug.StructuralNegativeSpaceCount > 0u && (!reservedPixelsAreOutsideSpatialMap(firstDebug) || overlaps(firstDebug.ReservedNegativeSpaceMask, firstDebug.WeaponOccupiedMask) || overlaps(firstDebug.ReservedNegativeSpaceMask, first.AttachmentMask)))
            {
                std::cerr << "Task 57 regression failed: reserved gap was not preserved through later placement.\n";
                return 1;
            }
            if (firstDebug.WingShape != WingShapeType::NONE && (firstDebug.WingPixelCount == 0u || firstDebug.WingRootPixelCount == 0u))
            {
                std::cerr << "Task 57 regression failed: semantic wing/root information became invalid.\n";
                return 1;
            }
            if (ReviewDimensions[dimensionIndex].Width == 24u && firstDebug.StructuralNegativeSpaceCount > 0u &&
                (firstDebug.StructuralNegativeSpaceTypeCounts[static_cast<std::size_t>(ShipStructuralNegativeSpaceType::OPEN_FRAME_BAY)] != 0u || firstDebug.StructuralNegativeSpaceTypeCounts[static_cast<std::size_t>(ShipStructuralNegativeSpaceType::NACELLE_CHANNEL)] != 0u))
            {
                std::cerr << "Task 57 regression failed: ambitious negative-space structure appeared at 24x24.\n";
                return 1;
            }
        }

        if (rejectionCount * 100u > ReviewDimensions.size() * 20u)
        {
            std::cerr << "Task 57 regression failed: pathological hull retry interaction detected.\n";
            return 1;
        }
    }

    // Explicitly exercise every faction under a high-negative-space structural style.
    for (uint32_t factionIndex = 0u; factionIndex < static_cast<uint32_t>(ShipFactionType::SHIP_FACTION_TYPE_END); ++factionIndex)
    {
        ShipGenerationSettings settings;
        settings.Seed = 0x57FAC71000000000ull + factionIndex;
        settings.Dimensions = { 64u,64u };
        settings.Style = ShipStyle::INDUSTRIAL;
        settings.Faction = static_cast<ShipFactionType>(factionIndex);
        ShipGenerationDebugInfo debug;
        const GeneratedShip ship = generator.generate(settings, &debug);
        if (countConnectedComponents(ship.HullMask) != 1u || debug.LastSilhouetteValidationFailure != SilhouetteValidationFailureReason::NONE)
        {
            std::cerr << "Task 57 regression failed: faction composition broke structural hull validity.\n";
            return 1;
        }
    }

    std::cout << "Task 57 structural negative-space regression passed.\n";
    return 0;
}
