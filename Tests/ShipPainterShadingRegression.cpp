#include "CoreRegressionSuites.h"

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
#include "ShipPainter.h"
#include <SpectralShipGen/ShipPaletteGenerator.h>
#include "WeaponGenerator.h"

namespace
{
    constexpr std::array<uint32_t, 7u> Resolutions = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };
    constexpr std::array<SpectralShipGen::ShipStyle, static_cast<std::size_t>(SpectralShipGen::ShipStyle::SHIP_STYLE_END)> Styles = { SpectralShipGen::ShipStyle::SLEEK, SpectralShipGen::ShipStyle::FIGHTER, SpectralShipGen::ShipStyle::HEAVY, SpectralShipGen::ShipStyle::INDUSTRIAL, SpectralShipGen::ShipStyle::SPEARHEAD, SpectralShipGen::ShipStyle::DELTA };
    constexpr std::array<SpectralShipGen::ShipFactionType, static_cast<std::size_t>(SpectralShipGen::ShipFactionType::SHIP_FACTION_TYPE_END)> Factions = { SpectralShipGen::ShipFactionType::FRONTIER, SpectralShipGen::ShipFactionType::MILITARY, SpectralShipGen::ShipFactionType::ASCENDANT, SpectralShipGen::ShipFactionType::XENO, SpectralShipGen::ShipFactionType::CORPORATE, SpectralShipGen::ShipFactionType::RELIC };
    constexpr uint32_t SamplesPerConfiguration = 2u;
    constexpr uint32_t MaximumHullAttempts = 8u;

    bool generatePipeline(SpectralShipGen::ShipGenerationContext& context)
    {
        SpectralShipGen::HullGenerator hullGenerator;
        SpectralShipGen::CockpitGenerator cockpitGenerator;
        SpectralShipGen::EngineGenerator engineGenerator;
        SpectralShipGen::MajorFeatureGenerator majorFeatureGenerator;
        SpectralShipGen::WeaponGenerator weaponGenerator;
        SpectralShipGen::AttachmentGenerator attachmentGenerator;
        SpectralShipGen::DetailGenerator detailGenerator;
        SpectralShipGen::ShipPainter shipPainter;

        context.Ship.Palette = SpectralShipGen::ShipPaletteGenerator::generate(context.Seeds.Palette, context.FactionProfile, context.Profile);

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
            weaponGenerator.generate(context);

            if (context.Settings.AttachmentsEnabled)
            {
                attachmentGenerator.generate(context);
            }

            detailGenerator.generate(context);
            shipPainter.paint(context);
            return true;
        }

        return false;
    }

    bool imagesEqual(const SpectralShipGen::Image& first, const SpectralShipGen::Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    bool isSemanticPixel(const SpectralShipGen::ShipGenerationContext& context, uint32_t x, uint32_t y)
    {
        const SpectralShipGen::GeneratedShip& ship = context.Ship;
        return ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y) || ship.AttachmentMask.get(x, y) || context.Weapons.OccupiedMask.get(x, y);
    }

    bool hasNeighbouringSemanticPixel(const SpectralShipGen::ShipGenerationContext& context, int32_t x, int32_t y)
    {
        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
        {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
            {
                if (offsetX == 0 && offsetY == 0)
                {
                    continue;
                }

                const int32_t checkX = x + offsetX;
                const int32_t checkY = y + offsetY;

                if (checkX < 0 || checkY < 0 || checkX >= static_cast<int32_t>(context.Ship.FinalImage.getWidth()) || checkY >= static_cast<int32_t>(context.Ship.FinalImage.getHeight()))
                {
                    continue;
                }

                if (isSemanticPixel(context, static_cast<uint32_t>(checkX), static_cast<uint32_t>(checkY)))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool validatePaintedPixels(const SpectralShipGen::ShipGenerationContext& context)
    {
        const SpectralShipGen::Image& image = context.Ship.FinalImage;

        for (uint32_t y = 0u; y < image.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < image.getWidth(); ++x)
            {
                const SpectralShipGen::Color& color = image.getPixel(x, y);

                if (color.A != 0u && color.A != 255u)
                {
                    return false;
                }

                if (color.A == 0u || isSemanticPixel(context, x, y))
                {
                    continue;
                }

                if (!hasNeighbouringSemanticPixel(context, static_cast<int32_t>(x), static_cast<int32_t>(y)))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool maskHasVisiblePixels(const SpectralShipGen::PixelMask& mask, const SpectralShipGen::Image& image)
    {
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (mask.get(x, y) && image.getPixel(x, y).A == 255u)
                {
                    return true;
                }
            }
        }

        return false;
    }
}

int SpectralShipGenTests::runPainterShadingRegression()
{
    for (uint32_t resolution : Resolutions)
    {
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
                    settings.Seed = 0x38000000ull + static_cast<uint64_t>(resolution) * 100000ull + static_cast<uint64_t>(static_cast<uint32_t>(style)) * 10000ull + static_cast<uint64_t>(static_cast<uint32_t>(faction)) * 1000ull + sample;
                    const SpectralShipGen::ShipGenerationSeeds seeds = SpectralShipGen::deriveShipGenerationSeeds(settings.Seed);
                    const SpectralShipGen::ShipGenerationProfile& profile = SpectralShipGen::getShipGenerationProfile(style);
                    SpectralShipGen::ShipGenerationContext firstContext(settings, profile, seeds, nullptr);
                    SpectralShipGen::ShipGenerationContext secondContext(settings, profile, seeds, nullptr);

                    if (!generatePipeline(firstContext) || !generatePipeline(secondContext))
                    {
                        std::cerr << "Shading pipeline generation failed at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (!imagesEqual(firstContext.Ship.FinalImage, secondContext.Ship.FinalImage))
                    {
                        std::cerr << "Shading determinism failure at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (!validatePaintedPixels(firstContext))
                    {
                        std::cerr << "Shading pixel-boundary validation failed at resolution " << resolution << ".\n";
                        return 1;
                    }

                    const uint32_t cockpitPixels = SpectralShipGen::PixelMaskUtils::getMaskPixelCount(firstContext.Ship.CockpitMask);
                    const uint32_t enginePixels = SpectralShipGen::PixelMaskUtils::getMaskPixelCount(firstContext.Ship.EngineMask);

                    if ((cockpitPixels > 0u && !maskHasVisiblePixels(firstContext.Ship.CockpitMask, firstContext.Ship.FinalImage)) || (enginePixels > 0u && !maskHasVisiblePixels(firstContext.Ship.EngineMask, firstContext.Ship.FinalImage)))
                    {
                        std::cerr << "Cockpit or engine readability validation failed at resolution " << resolution << ".\n";
                        return 1;
                    }

                    if (SpectralShipGen::PixelMaskUtils::getMaskPixelCount(firstContext.Weapons.OccupiedMask) > 0u && !maskHasVisiblePixels(firstContext.Weapons.OccupiedMask, firstContext.Ship.FinalImage))
                    {
                        std::cerr << "Weapon readability validation failed at resolution " << resolution << ".\n";
                        return 1;
                    }
                }
            }
        }
    }

    std::cout << "Ship painter shading regression passed.\n";
    return 0;
}
