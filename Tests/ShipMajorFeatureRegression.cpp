#include "CoreRegressionSuites.h"
#include "ShipGenerationContextTestUtils.h"

#include <array>
#include <cstdint>
#include <iostream>

#include "AttachmentGenerator.h"
#include "CockpitGenerator.h"
#include "DetailGenerator.h"
#include "EngineGenerator.h"
#include "HullGenerator.h"
#include "MajorFeatureGenerator.h"
#include "PixelMaskUtils.h"
#include "ShipGenerationContext.h"
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerationSettings.h>

namespace
{
    constexpr std::array<uint32_t, 7u> Resolutions = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };
    constexpr std::array<SpectralShipGen::ShipStyle, static_cast<std::size_t>(SpectralShipGen::ShipStyle::SHIP_STYLE_END)> Styles = { SpectralShipGen::ShipStyle::SLEEK, SpectralShipGen::ShipStyle::FIGHTER, SpectralShipGen::ShipStyle::HEAVY, SpectralShipGen::ShipStyle::INDUSTRIAL, SpectralShipGen::ShipStyle::SPEARHEAD, SpectralShipGen::ShipStyle::DELTA };
    constexpr std::array<SpectralShipGen::ShipFactionType, static_cast<std::size_t>(SpectralShipGen::ShipFactionType::SHIP_FACTION_TYPE_END)> Factions = { SpectralShipGen::ShipFactionType::FRONTIER, SpectralShipGen::ShipFactionType::MILITARY, SpectralShipGen::ShipFactionType::ASCENDANT, SpectralShipGen::ShipFactionType::XENO, SpectralShipGen::ShipFactionType::CORPORATE, SpectralShipGen::ShipFactionType::RELIC };
    constexpr uint32_t SamplesPerConfiguration = 12u;
    constexpr uint32_t MaximumHullAttempts = 8u;

    bool masksEqual(const SpectralShipGen::PixelMask& first, const SpectralShipGen::PixelMask& second)
    {
        if (first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight())
        {
            return false;
        }

        for (uint32_t y = 0u; y < first.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < first.getWidth(); ++x)
            {
                if (first.get(x, y) != second.get(x, y))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool isMaskSubset(const SpectralShipGen::PixelMask& subset, const SpectralShipGen::PixelMask& superset)
    {
        for (uint32_t y = 0u; y < subset.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < subset.getWidth(); ++x)
            {
                if (subset.get(x, y) && !superset.get(x, y))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool generatePipeline(SpectralShipGen::ShipGenerationContext& context)
    {
        SpectralShipGen::HullGenerator hullGenerator;
        SpectralShipGen::CockpitGenerator cockpitGenerator;
        SpectralShipGen::EngineGenerator engineGenerator;
        SpectralShipGen::MajorFeatureGenerator majorFeatureGenerator;
        SpectralShipGen::AttachmentGenerator attachmentGenerator;
        SpectralShipGen::DetailGenerator detailGenerator;

        for (uint32_t attempt = 0u; attempt < MaximumHullAttempts; ++attempt)
        {
            context.Ship.clear();
            hullGenerator.generate(context);

            if (!hullGenerator.validate(context))
            {
                continue;
            }

            cockpitGenerator.generate(context);
            engineGenerator.generate(context);
            majorFeatureGenerator.generate(context);

            if (context.Settings.AttachmentsEnabled)
            {
                attachmentGenerator.generate(context);
            }

            detailGenerator.generate(context);
            return true;
        }

        return false;
    }

    bool validateMajorFeatures(const SpectralShipGen::ShipGenerationContext& context)
    {
        const SpectralShipGen::MajorFeatureData& features = context.MajorFeatures;

        if (!isMaskSubset(features.OccupiedMask, context.Ship.HullMask) || !isMaskSubset(features.RaisedMask, features.OccupiedMask) || !isMaskSubset(features.RecessedMask, features.OccupiedMask) || !isMaskSubset(features.MechanicalMask, features.OccupiedMask) || !isMaskSubset(features.EmissiveMask, features.OccupiedMask))
        {
            return false;
        }

        if (SpectralShipGen::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.CockpitMask) || SpectralShipGen::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.EngineMask) || SpectralShipGen::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.EngineExhaustMask))
        {
            return false;
        }

        if (SpectralShipGen::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.AccentMask) || SpectralShipGen::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.MechanicalDetailMask) || SpectralShipGen::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.LightMask))
        {
            return false;
        }

        for (const SpectralShipGen::ShipAttachmentPlacement& placement : context.Ship.AttachmentPlacements)
        {
            if (features.OccupiedMask.get(placement.AnchorX, placement.AnchorY))
            {
                return false;
            }
        }

        return true;
    }
}

int SpectralShipGenTests::runMajorFeatureRegression()
{
    std::array<uint32_t, Resolutions.size()> shipsWithFeatures = {};
    std::array<uint32_t, static_cast<std::size_t>(SpectralShipGen::ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END)> observedTypeCounts = {};

    for (std::size_t resolutionIndex = 0u; resolutionIndex < Resolutions.size(); ++resolutionIndex)
    {
        const uint32_t resolution = Resolutions[resolutionIndex];

        for (SpectralShipGen::ShipStyle style : Styles)
        {
            for (SpectralShipGen::ShipFactionType faction : Factions)
            {
                for (uint32_t sample = 0u; sample < SamplesPerConfiguration; ++sample)
                {
                    SpectralShipGen::ShipGenerationSettings settings;
                    settings.Dimensions.Width = resolution;
                    settings.Dimensions.Height = resolution;
                    settings.Style = style;
                    settings.Faction = faction;
                    settings.Seed = 0x36000000ull + static_cast<uint64_t>(resolution) * 100000ull + static_cast<uint64_t>(static_cast<uint32_t>(style)) * 10000ull + static_cast<uint64_t>(static_cast<uint32_t>(faction)) * 1000ull + sample;
                    const SpectralShipGen::ShipGenerationSeeds seeds = SpectralShipGen::deriveShipGenerationSeeds(settings.Seed);
                    const SpectralShipGen::ShipGenerationProfile& profile = SpectralShipGen::getShipGenerationProfile(style);
                    SpectralShipGen::ShipGenerationContext firstContext(SpectralShipGenTests::makeTestExplicitGenerationConfiguration(settings), profile, getShipFactionProfile(settings.Faction), seeds, nullptr);
                    SpectralShipGen::ShipGenerationContext secondContext(SpectralShipGenTests::makeTestExplicitGenerationConfiguration(settings), profile, getShipFactionProfile(settings.Faction), seeds, nullptr);

                    if (!generatePipeline(firstContext) || !generatePipeline(secondContext))
                    {
                        std::cerr << "Pipeline generation failed at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (!masksEqual(firstContext.MajorFeatures.OccupiedMask, secondContext.MajorFeatures.OccupiedMask) || !masksEqual(firstContext.MajorFeatures.RaisedMask, secondContext.MajorFeatures.RaisedMask) || !masksEqual(firstContext.MajorFeatures.RecessedMask, secondContext.MajorFeatures.RecessedMask) || !masksEqual(firstContext.MajorFeatures.MechanicalMask, secondContext.MajorFeatures.MechanicalMask) || !masksEqual(firstContext.MajorFeatures.EmissiveMask, secondContext.MajorFeatures.EmissiveMask))
                    {
                        std::cerr << "Major-feature determinism failure at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (!validateMajorFeatures(firstContext))
                    {
                        std::cerr << "Major-feature validation failure at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (!firstContext.MajorFeatures.Placements.empty())
                    {
                        ++shipsWithFeatures[resolutionIndex];

                        for (const SpectralShipGen::MajorFeaturePlacement& placement : firstContext.MajorFeatures.Placements)
                        {
                            ++observedTypeCounts[static_cast<std::size_t>(placement.Type)];
                        }
                    }
                }
            }
        }

        if (resolution >= 32u && shipsWithFeatures[resolutionIndex] == 0u)
        {
            std::cerr << "No major features observed at resolution " << resolution << ".\n";
            return 1;
        }
    }

    uint32_t observedTypes = 0u;
    for (uint32_t count : observedTypeCounts) { if (count > 0u) { ++observedTypes; } }

    if (observedTypes < 5u)
    {
        std::cerr << "Insufficient major-feature type variety observed.\n";
        return 1;
    }

    std::cout << "Ship major feature regression passed.\n";
    return 0;
}
