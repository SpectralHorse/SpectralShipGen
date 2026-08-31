#include "CoreRegressionSuites.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>

#include <SpectralShipGen/GenerationComplexityBudget.h>
#include <SpectralShipGen/GenerationSpatialBudget.h>
#include "ShipGenerationContext.h"
#include "ShipGenerationContextTestUtils.h"
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include "VisualHierarchyPlanner.h"

namespace
{
    using namespace SpectralShipGen;

    struct SyntheticSpatialMasks
    {
        PixelMask Hull = PixelMask(96u, 64u, false);
        PixelMask Wing = PixelMask(96u, 64u, false);
        PixelMask WingRoot = PixelMask(96u, 64u, false);
        PixelMask OuterWing = PixelMask(96u, 64u, false);
    };

    SyntheticSpatialMasks createSyntheticSpatialMasks()
    {
        SyntheticSpatialMasks masks;
        for (uint32_t y = 3u; y <= 59u; ++y)
        {
            for (uint32_t x = 38u; x <= 57u; ++x)
            {
                masks.Hull.set(x, y, true);
            }
        }
        for (uint32_t y = 22u; y <= 44u; ++y)
        {
            for (uint32_t x = 24u; x <= 37u; ++x)
            {
                masks.Hull.set(x, y, true);
                masks.Wing.set(x, y, true);
                masks.WingRoot.set(x, y, true);
            }
            for (uint32_t x = 58u; x <= 71u; ++x)
            {
                masks.Hull.set(x, y, true);
                masks.Wing.set(x, y, true);
                masks.WingRoot.set(x, y, true);
            }
        }
        for (uint32_t y = 27u; y <= 39u; ++y)
        {
            for (uint32_t x = 8u; x <= 23u; ++x)
            {
                masks.Hull.set(x, y, true);
                masks.Wing.set(x, y, true);
                masks.OuterWing.set(x, y, true);
            }
            for (uint32_t x = 72u; x <= 87u; ++x)
            {
                masks.Hull.set(x, y, true);
                masks.Wing.set(x, y, true);
                masks.OuterWing.set(x, y, true);
            }
        }
        return masks;
    }

    bool spatialBudgetsEqual(const GenerationSpatialBudget& first, const GenerationSpatialBudget& second)
    {
        if (first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight() || first.getRegionMap() != second.getRegionMap())
        {
            return false;
        }

        const auto& firstStates = first.getRegionStates();
        const auto& secondStates = second.getRegionStates();
        for (std::size_t index = 0u; index < firstStates.size(); ++index)
        {
            if (firstStates[index].AreaPixels != secondStates[index].AreaPixels ||
                firstStates[index].Capacity != secondStates[index].Capacity ||
                firstStates[index].Load != secondStates[index].Load ||
                firstStates[index].DominantFeatureCount != secondStates[index].DominantFeatureCount ||
                firstStates[index].RejectionCount != secondStates[index].RejectionCount)
            {
                return false;
            }
        }
        return true;
    }
}

int SpectralShipGenTests::runStaticProfileRoutingRegression()
{
    using namespace SpectralShipGen;

    try
    {
        constexpr std::array<ShipStyle, 6u> Styles = { {
            ShipStyle::SLEEK,
            ShipStyle::FIGHTER,
            ShipStyle::HEAVY,
            ShipStyle::INDUSTRIAL,
            ShipStyle::SPEARHEAD,
            ShipStyle::DELTA
        } };
        constexpr std::array<ShipFactionType, 3u> Factions = { {
            ShipFactionType::CORPORATE,
            ShipFactionType::FRONTIER,
            ShipFactionType::MILITARY
        } };

        const GenerationScaleTraits scaleTraits = GenerationScaleTraits::fromDimensions({ 96u, 64u });
        const SyntheticSpatialMasks masks = createSyntheticSpatialMasks();

        for (std::size_t styleIndex = 0u; styleIndex < Styles.size(); ++styleIndex)
        {
            const ShipStyle style = Styles[styleIndex];
            const ShipGenerationProfile profile = getShipGenerationProfile(style);
            const ShipFactionType faction = Factions[styleIndex % Factions.size()];

            for (const bool reserveCockpit : { true, false })
            {
                const GenerationComplexityBudget fromProfile = GenerationComplexityBudget::create(scaleTraits, profile, faction, reserveCockpit);
                const GenerationComplexityBudget fromPreset = GenerationComplexityBudget::create(scaleTraits, style, faction, reserveCockpit);
                if (fromProfile.getInitialBudget() != fromPreset.getInitialBudget() ||
                    fromProfile.getAllocations() != fromPreset.getAllocations() ||
                    fromProfile.getConsumedByCategory() != fromPreset.getConsumedByCategory())
                {
                    std::cerr << "Complexity budget preset compatibility diverged from its resolved profile.\n";
                    return 1;
                }
            }

            GenerationSpatialBudget profileSpatial;
            profileSpatial.initialize(masks.Hull, masks.Wing, masks.WingRoot, masks.OuterWing, scaleTraits, profile);
            GenerationSpatialBudget presetSpatial;
            presetSpatial.initialize(masks.Hull, masks.Wing, masks.WingRoot, masks.OuterWing, scaleTraits, style);
            if (!spatialBudgetsEqual(profileSpatial, presetSpatial))
            {
                std::cerr << "Spatial budget preset compatibility diverged from its resolved profile.\n";
                return 1;
            }
        }

        ShipGenerationSettings fighterSettings;
        fighterSettings.Seed = 0x79A0C311A55EED01ull;
        fighterSettings.Dimensions = { 96u, 64u };
        fighterSettings.Style = ShipStyle::FIGHTER;
        fighterSettings.Faction = ShipFactionType::MILITARY;
        const ShipGenerationSeeds fighterSeeds = deriveShipGenerationSeeds(fighterSettings.Seed);
        const ShipGenerationProfile spearheadProfile = getShipGenerationProfile(ShipStyle::SPEARHEAD);
        ShipGenerationContext spearheadProfileContext(makeTestExplicitGenerationConfiguration(fighterSettings), spearheadProfile, getShipFactionProfile(fighterSettings.Faction), fighterSeeds);
        VisualHierarchyPlanner hierarchyPlanner;
        hierarchyPlanner.createPlan(spearheadProfileContext);
        if (spearheadProfileContext.VisualHierarchy.InfluenceEnabled ||
            spearheadProfileContext.VisualHierarchy.PrimaryAnchor != ShipVisualAnchorType::SILHOUETTE)
        {
            std::cerr << "Static hierarchy behavior followed settings.Style instead of the resolved SPEARHEAD profile.\n";
            return 1;
        }

        const GenerationComplexityBudget expectedSpearheadBudget = GenerationComplexityBudget::create(
            spearheadProfileContext.ScaleTraits, spearheadProfile, fighterSettings.Faction, true);
        if (spearheadProfileContext.ComplexityBudget.getInitialBudget() != expectedSpearheadBudget.getInitialBudget() ||
            spearheadProfileContext.ComplexityBudget.getAllocations() != expectedSpearheadBudget.getAllocations())
        {
            std::cerr << "Generation context complexity budget did not consume the resolved profile.\n";
            return 1;
        }

        ShipGenerationSettings spearheadSettings = fighterSettings;
        spearheadSettings.Seed = 0x79A0C311A55EED02ull;
        spearheadSettings.Style = ShipStyle::SPEARHEAD;
        const ShipGenerationProfile fighterProfile = getShipGenerationProfile(ShipStyle::FIGHTER);
        ShipGenerationContext fighterProfileContext(makeTestExplicitGenerationConfiguration(spearheadSettings), fighterProfile, getShipFactionProfile(spearheadSettings.Faction), deriveShipGenerationSeeds(spearheadSettings.Seed));
        hierarchyPlanner.createPlan(fighterProfileContext);
        if (!fighterProfileContext.VisualHierarchy.InfluenceEnabled)
        {
            std::cerr << "Static hierarchy behavior followed SPEARHEAD settings.Style instead of the resolved FIGHTER profile.\n";
            return 1;
        }

        const GenerationComplexityBudget expectedFighterBudget = GenerationComplexityBudget::create(
            fighterProfileContext.ScaleTraits, fighterProfile, spearheadSettings.Faction, true);
        if (fighterProfileContext.ComplexityBudget.getInitialBudget() != expectedFighterBudget.getInitialBudget() ||
            fighterProfileContext.ComplexityBudget.getAllocations() != expectedFighterBudget.getAllocations())
        {
            std::cerr << "Conflicting preset metadata altered resolved-profile complexity behavior.\n";
            return 1;
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Static profile routing regression failed: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
