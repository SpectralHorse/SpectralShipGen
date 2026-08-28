#include "RegressionSuites.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>

#include "GenerationMath.h"
#include "HullGenerator.h"
#include "PixelMaskUtils.h"
#include "ShipGenerationContext.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerationSettings.h"

namespace
{
    constexpr std::array<uint32_t, 7u> Resolutions = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };
    constexpr std::array<PixelShipGenerator::ShipStyle, static_cast<std::size_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END)> Styles = { PixelShipGenerator::ShipStyle::SLEEK, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipStyle::HEAVY, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipStyle::SPEARHEAD, PixelShipGenerator::ShipStyle::DELTA };
    constexpr uint32_t SamplesPerConfiguration = 40u;
    constexpr uint32_t MaximumHullAttempts = 8u;

    bool masksAreSymmetric(const PixelShipGenerator::PixelMask& mask)
    {
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (mask.get(x, y) != mask.get(mask.getWidth() - 1u - x, y))
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

    bool validateWingRows(const PixelShipGenerator::ShipGenerationContext& context)
    {
        const PixelShipGenerator::WingRegionData& regions = context.WingRegions;
        const PixelShipGenerator::PixelMask& hullMask = context.Ship.HullMask;

        if (!regions.hasWings())
        {
            return true;
        }

        const uint32_t leftCenter = (hullMask.getWidth() - 1u) / 2u;
        const uint32_t rightCenter = hullMask.getWidth() / 2u;
        uint32_t wingRows = 0u;

        for (uint32_t y = regions.StartY; y <= regions.EndY; ++y)
        {
            const uint32_t fuselageHalfWidth = regions.FuselageHalfWidths[y];

            if (fuselageHalfWidth == 0u)
            {
                continue;
            }

            const uint32_t fuselageLeft = leftCenter - (fuselageHalfWidth - 1u);
            const uint32_t fuselageRight = rightCenter + (fuselageHalfWidth - 1u);
            bool rowHasWing = false;

            for (uint32_t x = 0u; x < hullMask.getWidth(); ++x)
            {
                if (!regions.WingMask.get(x, y))
                {
                    continue;
                }

                rowHasWing = true;

                if (x < fuselageLeft)
                {
                    for (uint32_t checkX = x; checkX <= fuselageLeft; ++checkX)
                    {
                        if (!hullMask.get(checkX, y))
                        {
                            return false;
                        }
                    }
                }
                else if (x > fuselageRight)
                {
                    for (uint32_t checkX = fuselageRight; checkX <= x; ++checkX)
                    {
                        if (!hullMask.get(checkX, y))
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    return false;
                }
            }

            if (rowHasWing)
            {
                ++wingRows;
            }
        }

        if (regions.MaximumExtension >= PixelShipGenerator::GenerationMath::scalePixelsFrom64(4u, hullMask.getWidth()) && wingRows < std::max(1u, PixelShipGenerator::GenerationMath::scalePixelsFrom64(3u, hullMask.getHeight())))
        {
            return false;
        }

        return true;
    }

    bool generateValidatedHull(PixelShipGenerator::ShipGenerationContext& context, const PixelShipGenerator::HullGenerator& hullGenerator)
    {
        for (uint32_t attempt = 0u; attempt < MaximumHullAttempts; ++attempt)
        {
            context.Ship.clear();
            hullGenerator.generate(context);

            if (hullGenerator.validate(context))
            {
                return true;
            }
        }

        return false;
    }
}

int PixelShipGeneratorTests::runWingGeometryRegression()
{
    PixelShipGenerator::HullGenerator hullGenerator;

    for (uint32_t resolution : Resolutions)
    {
        uint32_t wingedShipCount = 0u;
        uint32_t maximumObservedExtension = 0u;

        for (PixelShipGenerator::ShipStyle style : Styles)
        {
            for (uint32_t sample = 0u; sample < SamplesPerConfiguration; ++sample)
            {
                PixelShipGenerator::ShipGenerationSettings settings;
                settings.Dimensions.Width = resolution;
                settings.Dimensions.Height = resolution;
                settings.Style = style;
                settings.Seed = 0x35A00000ull + static_cast<uint64_t>(resolution) * 10000ull + static_cast<uint64_t>(static_cast<uint32_t>(style)) * 1000ull + sample;
                const PixelShipGenerator::ShipGenerationSeeds seeds = PixelShipGenerator::deriveShipGenerationSeeds(settings.Seed);
                const PixelShipGenerator::ShipGenerationProfile& profile = PixelShipGenerator::getShipGenerationProfile(style);
                PixelShipGenerator::ShipGenerationDebugInfo debugInfo;
                PixelShipGenerator::ShipGenerationContext firstContext(settings, profile, seeds, &debugInfo);
                PixelShipGenerator::ShipGenerationContext secondContext(settings, profile, seeds, nullptr);

                if (!generateValidatedHull(firstContext, hullGenerator) || !generateValidatedHull(secondContext, hullGenerator))
                {
                    std::cerr << "Failed to generate validated hull at resolution " << resolution << ".\n";
                    return 1;
                }

                for (uint32_t y = 0u; y < resolution; ++y)
                {
                    for (uint32_t x = 0u; x < resolution; ++x)
                    {
                        if (firstContext.Ship.HullMask.get(x, y) != secondContext.Ship.HullMask.get(x, y))
                        {
                            std::cerr << "Hull determinism failure at resolution " << resolution << ".\n";
                            return 1;
                        }
                    }
                }

                const PixelShipGenerator::WingRegionData& regions = firstContext.WingRegions;

                if (!isMaskSubset(regions.WingMask, firstContext.Ship.HullMask) || !isMaskSubset(regions.WingRootMask, regions.WingMask) || !isMaskSubset(regions.OuterWingMask, regions.WingMask))
                {
                    std::cerr << "Wing region subset failure at resolution " << resolution << ".\n";
                    return 1;
                }

                if (!masksAreSymmetric(regions.WingMask) || !masksAreSymmetric(regions.WingRootMask) || !masksAreSymmetric(regions.OuterWingMask))
                {
                    std::cerr << "Wing semantic symmetry failure at resolution " << resolution << ".\n";
                    return 1;
                }

                if (!validateWingRows(firstContext))
                {
                    std::cerr << "Wing connection failure at resolution " << resolution << ".\n";
                    return 1;
                }

                if (regions.hasWings())
                {
                    ++wingedShipCount;
                    maximumObservedExtension = std::max(maximumObservedExtension, regions.MaximumExtension);

                    if (debugInfo.WingShape != regions.Shape || debugInfo.WingMaximumSpan != regions.MaximumSpan || debugInfo.WingMaximumExtension != regions.MaximumExtension || debugInfo.WingRootThickness != regions.RootThickness)
                    {
                        std::cerr << "Wing debug metadata mismatch at resolution " << resolution << ".\n";
                        return 1;
                    }
                }
            }
        }

        if (wingedShipCount == 0u)
        {
            std::cerr << "No winged hulls observed at resolution " << resolution << ".\n";
            return 1;
        }

        if (resolution >= 32u && maximumObservedExtension < 2u)
        {
            std::cerr << "Meaningful wing extension was not observed at resolution " << resolution << ".\n";
            return 1;
        }
    }

    std::cout << "Ship wing geometry regression passed.\n";
    return 0;
}
