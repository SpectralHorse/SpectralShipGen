#include "RegressionSuites.h"

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
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerationSettings.h"

namespace
{
    constexpr std::array<uint32_t, 7u> Resolutions = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };
    constexpr std::array<PixelShipGenerator::ShipStyle, static_cast<std::size_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END)> Styles = { PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipStyle::SPEARHEAD, PixelShipGenerator::ShipStyle::DELTA };
    constexpr std::array<PixelShipGenerator::ShipFactionType, static_cast<std::size_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END)> Factions = { PixelShipGenerator::ShipFactionType::FRONTIER, PixelShipGenerator::ShipFactionType::MILITARY, PixelShipGenerator::ShipFactionType::ASCENDANT, PixelShipGenerator::ShipFactionType::XENO, PixelShipGenerator::ShipFactionType::CORPORATE, PixelShipGenerator::ShipFactionType::RELIC };
    constexpr uint32_t SamplesPerConfiguration = 12u;
    constexpr uint32_t MaximumHullAttempts = 8u;

    bool masksEqual(const PixelShipGenerator::PixelMask& first, const PixelShipGenerator::PixelMask& second)
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

    bool isMaskSubset(const PixelShipGenerator::PixelMask& subset, const PixelShipGenerator::PixelMask& superset)
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

    bool generatePipeline(PixelShipGenerator::ShipGenerationContext& context)
    {
        PixelShipGenerator::HullGenerator hullGenerator;
        PixelShipGenerator::CockpitGenerator cockpitGenerator;
        PixelShipGenerator::EngineGenerator engineGenerator;
        PixelShipGenerator::MajorFeatureGenerator majorFeatureGenerator;
        PixelShipGenerator::AttachmentGenerator attachmentGenerator;
        PixelShipGenerator::DetailGenerator detailGenerator;

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

    bool validateMajorFeatures(const PixelShipGenerator::ShipGenerationContext& context)
    {
        const PixelShipGenerator::MajorFeatureData& features = context.MajorFeatures;

        if (!isMaskSubset(features.OccupiedMask, context.Ship.HullMask) || !isMaskSubset(features.RaisedMask, features.OccupiedMask) || !isMaskSubset(features.RecessedMask, features.OccupiedMask) || !isMaskSubset(features.MechanicalMask, features.OccupiedMask) || !isMaskSubset(features.EmissiveMask, features.OccupiedMask))
        {
            return false;
        }

        if (PixelShipGenerator::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.CockpitMask) || PixelShipGenerator::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.EngineMask) || PixelShipGenerator::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.EngineExhaustMask))
        {
            return false;
        }

        if (PixelShipGenerator::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.AccentMask) || PixelShipGenerator::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.MechanicalDetailMask) || PixelShipGenerator::PixelMaskUtils::masksOverlap(features.OccupiedMask, context.Ship.LightMask))
        {
            return false;
        }

        for (const PixelShipGenerator::ShipAttachmentPlacement& placement : context.Ship.AttachmentPlacements)
        {
            if (features.OccupiedMask.get(placement.AnchorX, placement.AnchorY))
            {
                return false;
            }
        }

        return true;
    }
}

int PixelShipGeneratorTests::runMajorFeatureRegression()
{
    std::array<uint32_t, Resolutions.size()> shipsWithFeatures = {};
    std::array<uint32_t, static_cast<std::size_t>(PixelShipGenerator::ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END)> observedTypeCounts = {};

    for (std::size_t resolutionIndex = 0u; resolutionIndex < Resolutions.size(); ++resolutionIndex)
    {
        const uint32_t resolution = Resolutions[resolutionIndex];

        for (PixelShipGenerator::ShipStyle style : Styles)
        {
            for (PixelShipGenerator::ShipFactionType faction : Factions)
            {
                for (uint32_t sample = 0u; sample < SamplesPerConfiguration; ++sample)
                {
                    PixelShipGenerator::ShipGenerationSettings settings;
                    settings.Dimensions.Width = resolution;
                    settings.Dimensions.Height = resolution;
                    settings.Style = style;
                    settings.Faction = faction;
                    settings.Seed = 0x36000000ull + static_cast<uint64_t>(resolution) * 100000ull + static_cast<uint64_t>(static_cast<uint32_t>(style)) * 10000ull + static_cast<uint64_t>(static_cast<uint32_t>(faction)) * 1000ull + sample;
                    const PixelShipGenerator::ShipGenerationSeeds seeds = PixelShipGenerator::deriveShipGenerationSeeds(settings.Seed);
                    const PixelShipGenerator::ShipGenerationProfile& profile = PixelShipGenerator::getShipGenerationProfile(style);
                    PixelShipGenerator::ShipGenerationContext firstContext(settings, profile, seeds, nullptr);
                    PixelShipGenerator::ShipGenerationContext secondContext(settings, profile, seeds, nullptr);

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

                        for (const PixelShipGenerator::MajorFeaturePlacement& placement : firstContext.MajorFeatures.Placements)
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
