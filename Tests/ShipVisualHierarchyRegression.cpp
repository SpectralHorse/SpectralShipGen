#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>

#include <SpectralShipGen/GenerationComplexityBudget.h>
#include <SpectralShipGen/GenerationScaleTraits.h>
#include <SpectralShipGen/ShipGenerationDebugInfo.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerator.h>
#include "VisualHierarchyPlanner.h"

namespace
{
    using namespace SpectralShipGen;

    constexpr std::array<ShipStyle, 6u> Styles = {
        ShipStyle::SLEEK, ShipStyle::FIGHTER, ShipStyle::HEAVY,
        ShipStyle::INDUSTRIAL, ShipStyle::SPEARHEAD, ShipStyle::DELTA
    };
    constexpr std::array<ShipFactionType, 6u> Factions = {
        ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT,
        ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC
    };
    constexpr std::array<ShipDimensions, 6u> Dimensions = {
        ShipDimensions{32u, 32u}, ShipDimensions{44u, 44u}, ShipDimensions{48u, 64u},
        ShipDimensions{64u, 48u}, ShipDimensions{64u, 64u}, ShipDimensions{96u, 96u}
    };

    struct AnchorRealization
    {
        uint32_t Planned = 0u;
        uint32_t Fulfilled = 0u;
    };

    bool hierarchyEqual(const ShipGenerationDebugInfo& first, const ShipGenerationDebugInfo& second)
    {
        return first.PrimaryVisualAnchor == second.PrimaryVisualAnchor
            && first.SecondaryVisualAnchor == second.SecondaryVisualAnchor
            && first.VisualAnchorTargetRegion == second.VisualAnchorTargetRegion
            && first.VisualHierarchyReservedComplexity == second.VisualHierarchyReservedComplexity
            && first.VisualHierarchyFallbackOccurred == second.VisualHierarchyFallbackOccurred;
    }

    bool anchorFulfilled(const ShipGenerationDebugInfo& debug)
    {
        switch (debug.PrimaryVisualAnchor)
        {
        case ShipVisualAnchorType::SILHOUETTE: return true;
        case ShipVisualAnchorType::COCKPIT: return debug.CockpitPlacementSucceeded;
        case ShipVisualAnchorType::WINGS: return debug.WingShape != WingShapeType::NONE;
        case ShipVisualAnchorType::ENGINES: return debug.EngineCount > 0u;
        case ShipVisualAnchorType::WEAPONS: return debug.WeaponCount > 0u;
        case ShipVisualAnchorType::MAJOR_FEATURE: return debug.MajorFeatureCount > 0u;
        case ShipVisualAnchorType::HULL_LAYERS: return debug.HullLayerCount > 0u;
        case ShipVisualAnchorType::CENTRAL_CORE: return debug.CoreTreatmentCount > 0u;
        case ShipVisualAnchorType::MACRO_ASYMMETRY: return debug.MacroAsymmetryFulfilled;
        case ShipVisualAnchorType::NEGATIVE_SPACE: return debug.StructuralNegativeSpaceCount > 0u;
        default: return false;
        }
    }

    bool generate(const ShipGenerationSettings& settings, GeneratedShip& ship, ShipGenerationDebugInfo& debug)
    {
        try
        {
            ShipGenerator generator;
            ship = generator.generate(settings, &debug);
            return true;
        }
        catch (const std::exception& exception)
        {
            std::cerr << "Generation failed: " << exception.what() << '\n';
            return false;
        }
    }

    bool checkDeterminismAndDomainIsolation()
    {
        ShipGenerationSettings settings;
        settings.Seed = 0xB8D7A34C25F019E1ull;
        settings.Dimensions = { 64u, 64u };
        settings.Style = ShipStyle::FIGHTER;
        settings.Faction = ShipFactionType::MILITARY;

        GeneratedShip firstShip;
        GeneratedShip secondShip;
        ShipGenerationDebugInfo firstDebug;
        ShipGenerationDebugInfo secondDebug;
        if (!generate(settings, firstShip, firstDebug) || !generate(settings, secondShip, secondDebug)) { return false; }
        if (firstShip.FinalImage.getPixels() != secondShip.FinalImage.getPixels() || !hierarchyEqual(firstDebug, secondDebug))
        {
            std::cerr << "Visual hierarchy planning is not deterministic.\n";
            return false;
        }

        const GenerationDomainSeeds domains = resolveGenerationDomainSeeds(firstShip.Seeds, settings.DomainSeedOverrides, settings.RandomStreamMode);
        ShipGenerationSettings cockpitReroll = settings;
        cockpitReroll.DomainSeedOverrides.set(GenerationDomain::COCKPIT, domains.get(GenerationDomain::COCKPIT) ^ 0xA24BAED4963EE407ull);
        GeneratedShip rerolledShip;
        ShipGenerationDebugInfo rerolledDebug;
        if (!generate(cockpitReroll, rerolledShip, rerolledDebug)) { return false; }
        if (!hierarchyEqual(firstDebug, rerolledDebug))
        {
            std::cerr << "An unrelated Cockpit-domain reroll replanned the visual hierarchy.\n";
            return false;
        }
        return true;
    }

    bool checkDeterministicFallback()
    {
        ShipGenerationSettings settings;
        settings.Seed = 0x1234000000000042ull;
        settings.Dimensions = { 64u, 64u };
        settings.Style = ShipStyle::SLEEK;
        settings.Faction = ShipFactionType::FRONTIER;

        GeneratedShip firstShip;
        GeneratedShip secondShip;
        ShipGenerationDebugInfo firstDebug;
        ShipGenerationDebugInfo secondDebug;
        if (!generate(settings, firstShip, firstDebug) || !generate(settings, secondShip, secondDebug)) { return false; }
        if (!firstDebug.VisualHierarchyFallbackOccurred || firstDebug.PrimaryVisualAnchor != ShipVisualAnchorType::SILHOUETTE)
        {
            std::cerr << "Known hierarchy fallback fixture no longer falls back to SILHOUETTE.\n";
            return false;
        }
        if (!hierarchyEqual(firstDebug, secondDebug) || firstShip.FinalImage.getPixels() != secondShip.FinalImage.getPixels())
        {
            std::cerr << "Hierarchy fallback is not deterministic.\n";
            return false;
        }
        return true;
    }

    bool checkBudgetReservation()
    {
        ShipGenerationSettings settings;
        settings.Seed = 0xD1B54A32D192ED03ull;
        settings.Dimensions = { 64u, 64u };
        settings.Style = ShipStyle::HEAVY;
        settings.Faction = ShipFactionType::RELIC;

        for (uint32_t sample = 0u; sample < 80u; ++sample)
        {
            settings.Seed += 0x9E3779B97F4A7C15ull;
            GeneratedShip ship;
            ShipGenerationDebugInfo debug;
            if (!generate(settings, ship, debug)) { return false; }
            if (debug.PrimaryVisualAnchor == ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END || debug.VisualHierarchyReservedComplexity == 0u)
            {
                std::cerr << "A normal hierarchy plan did not reserve complexity.\n";
                return false;
            }

            const GenerationComplexityBudget baseline = GenerationComplexityBudget::create(
                GenerationScaleTraits::fromDimensions(settings.Dimensions), settings.Style, settings.Faction, true);
            const GenerationComplexityCategory category = VisualHierarchyPlanner::getReservedCategory(debug.PrimaryVisualAnchor);
            if (category == GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END)
            {
                if (debug.ComplexityCategoryAllocations != baseline.getAllocations())
                {
                    std::cerr << "Engine/asymmetry hierarchy hold unexpectedly changed category allocations.\n";
                    return false;
                }
                if (debug.ComplexityConsumedBudget + debug.VisualHierarchyReservedComplexity > debug.ComplexityInitialBudget)
                {
                    std::cerr << "Hierarchy global hold did not preserve its reserved capacity.\n";
                    return false;
                }
            }
            else
            {
                const std::size_t index = static_cast<std::size_t>(category);
                if (debug.ComplexityCategoryAllocations[index] < baseline.getCategoryAllocation(category))
                {
                    std::cerr << "Primary anchor category did not receive its Task-46 reservation.\n";
                    return false;
                }
            }

            if (debug.VisualAnchorTargetRegion != GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END)
            {
                const std::size_t index = static_cast<std::size_t>(debug.VisualAnchorTargetRegion);
                const uint32_t area = debug.SpatialRegionAreas[index];
                if (area > 0u && debug.SpatialRegionCapacities[index] == 0u)
                {
                    std::cerr << "Hierarchy target region lost its Task-47 capacity.\n";
                    return false;
                }
            }
        }
        return true;
    }

    bool checkStylesFactionsScalesAndRealization()
    {
        std::array<AnchorRealization, static_cast<std::size_t>(ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END)> realization = {};
        uint32_t sparseShips = 0u;
        uint32_t noSecondaryShips = 0u;
        uint32_t nonAnchorWeaponShips = 0u;
        uint32_t nonAnchorMajorFeatureShips = 0u;
        uint32_t nonAnchorLayerShips = 0u;
        uint32_t nonAnchorCoreShips = 0u;

        for (std::size_t styleIndex = 0u; styleIndex < Styles.size(); ++styleIndex)
        {
            for (uint32_t sample = 0u; sample < 150u; ++sample)
            {
                ShipGenerationSettings settings;
                settings.Seed = 0x839A000000000000ull + static_cast<uint64_t>(styleIndex) * 1000ull + sample;
                settings.Dimensions = Dimensions[sample % Dimensions.size()];
                settings.Style = Styles[styleIndex];
                settings.Faction = Factions[sample % Factions.size()];

                GeneratedShip ship;
                ShipGenerationDebugInfo debug;
                if (!generate(settings, ship, debug)) { return false; }
                if (ship.FinalImage.getWidth() != settings.Dimensions.Width || ship.FinalImage.getHeight() != settings.Dimensions.Height)
                {
                    std::cerr << "Hierarchy generation broke native rectangular dimensions.\n";
                    return false;
                }
                if (debug.PrimaryVisualAnchor == ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END)
                {
                    std::cerr << "A domain-substream ship has no primary visual anchor.\n";
                    return false;
                }
                if (debug.SecondaryVisualAnchor == ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END) { ++noSecondaryShips; }
                if (debug.WeaponCount == 0u && debug.MajorFeatureCount == 0u && debug.HullLayerCount == 0u) { ++sparseShips; }
                if (debug.WeaponCount > 0u && debug.PrimaryVisualAnchor != ShipVisualAnchorType::WEAPONS) { ++nonAnchorWeaponShips; }
                if (debug.MajorFeatureCount > 0u && debug.PrimaryVisualAnchor != ShipVisualAnchorType::MAJOR_FEATURE) { ++nonAnchorMajorFeatureShips; }
                if (debug.HullLayerCount > 0u && debug.PrimaryVisualAnchor != ShipVisualAnchorType::HULL_LAYERS) { ++nonAnchorLayerShips; }
                if (debug.CoreTreatmentCount > 0u && debug.PrimaryVisualAnchor != ShipVisualAnchorType::CENTRAL_CORE) { ++nonAnchorCoreShips; }

                const std::size_t anchorIndex = static_cast<std::size_t>(debug.PrimaryVisualAnchor);
                ++realization[anchorIndex].Planned;
                if (anchorFulfilled(debug)) { ++realization[anchorIndex].Fulfilled; }

                if (settings.Style == ShipStyle::SPEARHEAD)
                {
                    if (debug.PrimaryVisualAnchor != ShipVisualAnchorType::SILHOUETTE || debug.SecondaryVisualAnchor != ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END || debug.VisualHierarchyReservedComplexity != 0u)
                    {
                        std::cerr << "SPEARHEAD hierarchy protection changed its fixed non-influencing plan.\n";
                        return false;
                    }
                }
            }
        }

        if (noSecondaryShips < 250u || sparseShips == 0u)
        {
            std::cerr << "Hierarchy planning removed legitimate sparse/single-anchor designs.\n";
            return false;
        }
        if (nonAnchorMajorFeatureShips == 0u || nonAnchorLayerShips == 0u || nonAnchorCoreShips == 0u)
        {
            std::cerr << "Hierarchy planning permanently starved a non-anchor structural system.\n";
            return false;
        }

        constexpr std::array<ShipVisualAnchorType, 7u> StronglyRealized = {
            ShipVisualAnchorType::COCKPIT, ShipVisualAnchorType::WINGS, ShipVisualAnchorType::ENGINES,
            ShipVisualAnchorType::WEAPONS, ShipVisualAnchorType::MAJOR_FEATURE,
            ShipVisualAnchorType::HULL_LAYERS, ShipVisualAnchorType::CENTRAL_CORE
        };
        for (ShipVisualAnchorType anchor : StronglyRealized)
        {
            const AnchorRealization& value = realization[static_cast<std::size_t>(anchor)];
            if (value.Planned == 0u || value.Fulfilled * 100u < value.Planned * 75u)
            {
                std::cerr << getShipVisualAnchorTypeName(anchor) << " primary anchor was not realized often enough.\n";
                return false;
            }
        }
        const AnchorRealization& negative = realization[static_cast<std::size_t>(ShipVisualAnchorType::NEGATIVE_SPACE)];
        if (negative.Planned == 0u || negative.Fulfilled != negative.Planned)
        {
            std::cerr << "Resolved NEGATIVE_SPACE anchors did not preserve actual structural negative space.\n";
            return false;
        }
        const AnchorRealization& macro = realization[static_cast<std::size_t>(ShipVisualAnchorType::MACRO_ASYMMETRY)];
        if (macro.Planned == 0u || macro.Fulfilled * 100u < macro.Planned * 25u)
        {
            std::cerr << "MACRO_ASYMMETRY anchor is effectively infeasible under Task-49 safety rules.\n";
            return false;
        }
        return true;
    }
}

int SpectralShipGenTests::runVisualHierarchyRegression()
{
    bool success = true;
    success = checkDeterminismAndDomainIsolation() && success;
    success = checkDeterministicFallback() && success;
    success = checkBudgetReservation() && success;
    success = checkStylesFactionsScalesAndRealization() && success;
    if (success) { std::cout << "Task 58 visual hierarchy regression passed.\n"; }
    return success ? 0 : 1;
}
