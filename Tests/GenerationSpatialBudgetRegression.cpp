#include "RegressionSuites.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>

#include "GenerationSpatialBudget.h"
#include "ShipGenerator.h"

namespace
{
    using namespace PixelShipGenerator;

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    GenerationSpatialBudget createSyntheticBudget()
    {
        PixelMask hull(64u, 64u, false);
        PixelMask root(64u, 64u, false);
        PixelMask outer(64u, 64u, false);

        for (uint32_t y = 4u; y <= 58u; ++y)
        {
            for (uint32_t x = 24u; x <= 39u; ++x) { hull.set(x, y, true); }
        }
        for (uint32_t y = 24u; y <= 40u; ++y)
        {
            for (uint32_t x = 16u; x <= 23u; ++x) { hull.set(x, y, true); root.set(x, y, true); }
            for (uint32_t x = 40u; x <= 47u; ++x) { hull.set(x, y, true); root.set(x, y, true); }
        }
        for (uint32_t y = 28u; y <= 36u; ++y)
        {
            for (uint32_t x = 4u; x <= 15u; ++x) { hull.set(x, y, true); outer.set(x, y, true); }
            for (uint32_t x = 48u; x <= 59u; ++x) { hull.set(x, y, true); outer.set(x, y, true); }
        }

        GenerationSpatialBudget budget;
        budget.initialize(hull, outer, root, outer, GenerationScaleTraits::fromDimensions({ 64u, 64u }));
        return budget;
    }
}

int PixelShipGeneratorTests::runGenerationSpatialBudgetRegression()
{
    using namespace PixelShipGenerator;

    try
    {
        GenerationSpatialBudget budget = createSyntheticBudget();
        const auto leftOuter = budget.makeRegionSet(GenerationSpatialRegion::LEFT_OUTER_WING);
        const auto mid = budget.makeRegionSet(GenerationSpatialRegion::MID_FUSELAGE);
        const auto leftRoot = budget.makeRegionSet(GenerationSpatialRegion::LEFT_WING_ROOT);

        GenerationSpatialBudget hierarchyPreferred = createSyntheticBudget();
        const auto beforeHierarchyStates = hierarchyPreferred.getRegionStates();
        hierarchyPreferred.applyHierarchyPreference(GenerationSpatialRegion::LEFT_WING_ROOT, 128u);
        const auto& afterHierarchyStates = hierarchyPreferred.getRegionStates();
        const std::size_t leftRootIndex = static_cast<std::size_t>(GenerationSpatialRegion::LEFT_WING_ROOT);
        const std::size_t rightRootIndex = static_cast<std::size_t>(GenerationSpatialRegion::RIGHT_WING_ROOT);
        if (afterHierarchyStates[leftRootIndex].Capacity <= beforeHierarchyStates[leftRootIndex].Capacity || afterHierarchyStates[rightRootIndex].Capacity <= beforeHierarchyStates[rightRootIndex].Capacity || afterHierarchyStates[leftRootIndex].Capacity != afterHierarchyStates[rightRootIndex].Capacity)
        {
            std::cerr << "Hierarchy spatial preference did not increase the focal region and its mirror coherently.\n";
            return 1;
        }

        if (budget.getRegionStates()[static_cast<std::size_t>(GenerationSpatialRegion::LEFT_OUTER_WING)].Capacity != budget.getRegionStates()[static_cast<std::size_t>(GenerationSpatialRegion::RIGHT_OUTER_WING)].Capacity)
        {
            std::cerr << "Symmetric semantic regions received different capacities.\n";
            return 1;
        }

        const uint32_t initialDominantAcceptance = budget.getPlacementAcceptancePercent(leftOuter, 18u, true);
        budget.consume(leftOuter, 18u, true);
        const uint32_t competingDominantAcceptance = budget.getPlacementAcceptancePercent(leftOuter, 18u, true);
        if (competingDominantAcceptance >= initialDominantAcceptance || competingDominantAcceptance > 22u)
        {
            std::cerr << "A dominant wing feature does not strongly discourage another dominant feature locally.\n";
            return 1;
        }

        if (budget.getDetailPreferencePercent(leftOuter, 1u) == 0u)
        {
            std::cerr << "Small compatible details were completely disabled in a loaded region.\n";
            return 1;
        }

        if (budget.getDetailPreferencePercent(mid, 1u) <= budget.getDetailPreferencePercent(leftOuter, 1u))
        {
            std::cerr << "Sparse regions are not preferred for subtle detail placement.\n";
            return 1;
        }

        GenerationSpatialBudget symmetric = createSyntheticBudget();
        auto wingPair = symmetric.makeRegionSet(GenerationSpatialRegion::LEFT_OUTER_WING);
        symmetric.addRegion(wingPair, GenerationSpatialRegion::RIGHT_OUTER_WING);
        symmetric.consume(wingPair, 18u, true);
        const auto& states = symmetric.getRegionStates();
        const auto& leftState = states[static_cast<std::size_t>(GenerationSpatialRegion::LEFT_OUTER_WING)];
        const auto& rightState = states[static_cast<std::size_t>(GenerationSpatialRegion::RIGHT_OUTER_WING)];
        if (leftState.Load != rightState.Load || leftState.DominantFeatureCount != rightState.DominantFeatureCount)
        {
            std::cerr << "Symmetric feature load was not updated coherently.\n";
            return 1;
        }

        if (budget.getRegionStates()[static_cast<std::size_t>(GenerationSpatialRegion::MID_FUSELAGE)].Capacity <= budget.getRegionStates()[static_cast<std::size_t>(GenerationSpatialRegion::LEFT_WING_ROOT)].Capacity)
        {
            std::cerr << "Regional capacity does not account for semantic area/capability differences.\n";
            return 1;
        }

        if (budget.getPlacementAcceptancePercent(leftRoot, 8u, false) == 0u)
        {
            std::cerr << "Usable sparse regions were incorrectly blocked.\n";
            return 1;
        }

        constexpr std::array<ShipDimensions, 12u> Dimensions = { {
            { 24u, 24u }, { 32u, 32u }, { 44u, 44u }, { 64u, 64u }, { 96u, 96u }, { 160u, 160u },
            { 32u, 44u }, { 44u, 32u }, { 48u, 64u }, { 64u, 48u }, { 64u, 96u }, { 96u, 64u }
        } };

        ShipGenerator generator;
        uint32_t observedSpatialRejections = 0u;
        for (std::size_t index = 0u; index < Dimensions.size(); ++index)
        {
            ShipGenerationSettings settings;
            settings.Seed = 0xA4093822299F31D0ull ^ (static_cast<uint64_t>(index) * 0x9E3779B97F4A7C15ull);
            settings.Dimensions = Dimensions[index];
            settings.Style = static_cast<ShipStyle>(index % static_cast<std::size_t>(ShipStyle::SHIP_STYLE_END));
            settings.Faction = static_cast<ShipFactionType>(index % static_cast<std::size_t>(ShipFactionType::SHIP_FACTION_TYPE_END));
            settings.DetailDensity = 70u;
            settings.AttachmentsEnabled = true;

            ShipGenerationDebugInfo firstDebug;
            ShipGenerationDebugInfo secondDebug;
            const GeneratedShip first = generator.generate(settings, &firstDebug);
            const GeneratedShip second = generator.generate(settings, &secondDebug);

            if (!imagesEqual(first.FinalImage, second.FinalImage) || firstDebug.SpatialRegionLoads != secondDebug.SpatialRegionLoads || firstDebug.SpatialRegionRejections != secondDebug.SpatialRegionRejections)
            {
                std::cerr << "Semantic spatial budgeting is not deterministic.\n";
                return 1;
            }

            if (firstDebug.ComplexityConsumedBudget > firstDebug.ComplexityInitialBudget)
            {
                std::cerr << "Spatial budgeting bypassed the global complexity budget.\n";
                return 1;
            }

            if (firstDebug.SpatialRegionMapWidth != settings.Dimensions.Width || firstDebug.SpatialRegionMapHeight != settings.Dimensions.Height || firstDebug.SpatialRegionMap.size() != static_cast<std::size_t>(settings.Dimensions.Width) * settings.Dimensions.Height)
            {
                std::cerr << "Semantic region map does not match native rectangular dimensions.\n";
                return 1;
            }

            for (std::size_t region = 0u; region < GenerationSpatialBudget::RegionCount; ++region)
            {
                if (firstDebug.SpatialRegionAreas[region] == 0u && (firstDebug.SpatialRegionCapacities[region] != 0u || firstDebug.SpatialRegionLoads[region] != 0u))
                {
                    std::cerr << "Empty semantic regions received capacity/load.\n";
                    return 1;
                }
            }
            observedSpatialRejections += firstDebug.SpatialOverloadRejectionCount;
        }

        if (observedSpatialRejections == 0u)
        {
            std::cerr << "The integration sample never exercised a spatial-overload rejection.\n";
            return 1;
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Generation spatial budget regression failed: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
