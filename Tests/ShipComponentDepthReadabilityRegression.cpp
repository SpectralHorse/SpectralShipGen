#include "CoreRegressionSuites.h"

#include <cstdint>
#include <iostream>

#include "PixelMaskUtils.h"
#include "ShipGenerationContext.h"
#include <PixelShipGenerator/ShipGenerationProfile.h>
#include <PixelShipGenerator/ShipGenerationSeeds.h>
#include <PixelShipGenerator/ShipGenerationSettings.h>
#include "ShipPainter.h"

namespace
{
    using namespace PixelShipGenerator;

    bool masksEqual(const PixelMask& first, const PixelMask& second)
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

    void fillRectangle(PixelMask& mask, uint32_t minX, uint32_t minY, uint32_t maxX, uint32_t maxY)
    {
        for (uint32_t y = minY; y <= maxY; ++y)
        {
            for (uint32_t x = minX; x <= maxX; ++x)
            {
                mask.set(x, y, true);
            }
        }
    }

    void prepareSyntheticScene(ShipGenerationContext& context)
    {
        GeneratedShip& ship = context.Ship;
        ship.Palette = ShipPalette();

        const uint32_t width = ship.HullMask.getWidth();
        const uint32_t height = ship.HullMask.getHeight();
        const uint32_t minX = width / 4u;
        const uint32_t maxX = width - 1u - minX;
        const uint32_t minY = height / 8u;
        const uint32_t maxY = height - 1u - height / 6u;
        fillRectangle(ship.HullMask, minX, minY, maxX, maxY);

        // Explicit weapon socket embedded in the hull with a raised housing above it.
        const uint32_t rootMinX = width / 2u - 3u;
        const uint32_t rootMaxX = width / 2u + 2u;
        const uint32_t rootMinY = height / 2u;
        const uint32_t rootMaxY = rootMinY + 1u;
        fillRectangle(context.Weapons.RootMask, rootMinX, rootMinY, rootMaxX, rootMaxY);
        fillRectangle(context.Weapons.BodyMask, rootMinX - 1u, rootMinY - 5u, rootMaxX + 1u, rootMinY - 1u);
        PixelMaskUtils::mergeMask(context.Weapons.OccupiedMask, context.Weapons.RootMask);
        PixelMaskUtils::mergeMask(context.Weapons.OccupiedMask, context.Weapons.BodyMask);
        WeaponPlacement weaponPlacement;
        weaponPlacement.AnchorX = width / 2u;
        weaponPlacement.AnchorY = rootMinY;
        weaponPlacement.BodyMinX = rootMinX - 1u;
        weaponPlacement.BodyMaxX = rootMaxX + 1u;
        weaponPlacement.BodyMinY = rootMinY - 5u;
        weaponPlacement.BodyMaxY = rootMinY - 1u;
        context.Weapons.Placements.push_back(weaponPlacement);

        // Side attachment whose first outward pixel is connected to a known hull anchor.
        ShipAttachmentPlacement attachment;
        attachment.Type = ShipAttachmentType::AUXILIARY_POD;
        attachment.Region = ShipAttachmentRegion::MIDDLE_SIDE;
        attachment.Direction = ShipAttachmentDirection::RIGHT;
        attachment.AnchorX = maxX;
        attachment.AnchorY = height / 2u + 7u;
        attachment.MinimumX = maxX + 1u;
        attachment.MaximumX = maxX + 4u;
        attachment.MinimumY = attachment.AnchorY - 1u;
        attachment.MaximumY = attachment.AnchorY + 1u;
        ship.AttachmentPlacements.push_back(attachment);
        for (uint32_t x = attachment.MinimumX; x <= attachment.MaximumX && x < width; ++x)
        {
            ship.AttachmentMask.set(x, attachment.AnchorY, true);
        }

        // Rear engine housing overlapping the hull.  The row directly inboard of
        // the engine remains plain hull and is the expected mounting seam.
        const uint32_t engineMinX = width / 2u - 3u;
        const uint32_t engineMaxX = width / 2u + 2u;
        const uint32_t engineMinY = maxY - 4u;
        const uint32_t engineMaxY = maxY;
        fillRectangle(ship.EngineMask, engineMinX, engineMinY, engineMaxX, engineMaxY);

        // A simple cockpit with a distinct structural frame around glass.
        const uint32_t cockpitMinX = width / 2u - 4u;
        const uint32_t cockpitMaxX = width / 2u + 3u;
        const uint32_t cockpitMinY = minY + 5u;
        const uint32_t cockpitMaxY = cockpitMinY + 6u;
        fillRectangle(ship.CockpitMask, cockpitMinX, cockpitMinY, cockpitMaxX, cockpitMaxY);
        fillRectangle(context.Cockpit.BaseMask, cockpitMinX, cockpitMaxY - 1u, cockpitMaxX, cockpitMaxY);
        fillRectangle(context.Cockpit.FrameMask, cockpitMinX, cockpitMinY, cockpitMaxX, cockpitMaxY);
        fillRectangle(context.Cockpit.GlassMask, cockpitMinX + 1u, cockpitMinY + 1u, cockpitMaxX - 1u, cockpitMaxY - 2u);
        for (uint32_t y = cockpitMinY + 1u; y <= cockpitMaxY - 2u; ++y)
        {
            for (uint32_t x = cockpitMinX + 1u; x <= cockpitMaxX - 1u; ++x)
            {
                context.Cockpit.FrameMask.set(x, y, false);
            }
        }

        // Semantic negative space is intentionally outside all component masks.
        context.StructuralNegativeSpace.ReservedMask.set(2u, 2u, true);
    }

    bool verifyMaskPreservation(const ShipGenerationContext& context,
        const PixelMask& hullBefore,
        const PixelMask& cockpitBefore,
        const PixelMask& engineBefore,
        const PixelMask& attachmentBefore,
        const PixelMask& weaponBefore,
        const PixelMask& negativeSpaceBefore)
    {
        return masksEqual(hullBefore, context.Ship.HullMask)
            && masksEqual(cockpitBefore, context.Ship.CockpitMask)
            && masksEqual(engineBefore, context.Ship.EngineMask)
            && masksEqual(attachmentBefore, context.Ship.AttachmentMask)
            && masksEqual(weaponBefore, context.Weapons.OccupiedMask)
            && masksEqual(negativeSpaceBefore, context.StructuralNegativeSpace.ReservedMask);
    }

    bool runSemanticCueCase(uint32_t width, uint32_t height, bool expectFullDepthTreatment)
    {
        ShipGenerationSettings settings;
        settings.Seed = 0x8100D3A7ull + static_cast<uint64_t>(width) * 257ull + height;
        settings.Dimensions = { width, height };
        settings.Style = ShipStyle::FIGHTER;
        settings.Faction = ShipFactionType::FRONTIER;

        const ShipGenerationProfile& profile = getShipGenerationProfile(settings.Style);
        const ShipGenerationSeeds seeds = deriveShipGenerationSeeds(settings.Seed);
        ShipGenerationContext first(settings, profile, seeds, nullptr);
        ShipGenerationContext second(settings, profile, seeds, nullptr);
        prepareSyntheticScene(first);
        prepareSyntheticScene(second);

        const PixelMask hullBefore = first.Ship.HullMask;
        const PixelMask cockpitBefore = first.Ship.CockpitMask;
        const PixelMask engineBefore = first.Ship.EngineMask;
        const PixelMask attachmentBefore = first.Ship.AttachmentMask;
        const PixelMask weaponBefore = first.Weapons.OccupiedMask;
        const PixelMask negativeSpaceBefore = first.StructuralNegativeSpace.ReservedMask;

        ShipPainter painter;
        painter.paint(first);
        painter.paint(second);

        if (first.Ship.FinalImage.getPixels() != second.Ship.FinalImage.getPixels())
        {
            std::cerr << "Component-depth painter lost deterministic output at " << width << "x" << height << ".\n";
            return false;
        }

        if (!verifyMaskPreservation(first, hullBefore, cockpitBefore, engineBefore, attachmentBefore, weaponBefore, negativeSpaceBefore))
        {
            std::cerr << "Component-depth painting modified semantic geometry at " << width << "x" << height << ".\n";
            return false;
        }

        if (first.Ship.FinalImage.getPixel(2u, 2u).A != 0u)
        {
            std::cerr << "Component-depth painting filled reserved negative space at " << width << "x" << height << ".\n";
            return false;
        }

        const ShipPalette& palette = first.Ship.Palette;
        const uint32_t hullMaxX = width - 1u - width / 4u;
        const uint32_t attachmentAnchorY = height / 2u + 7u;
        const Color attachmentSocket = first.Ship.FinalImage.getPixel(hullMaxX, attachmentAnchorY);

        if (expectFullDepthTreatment)
        {
            if (attachmentSocket != palette.MechanicalDark)
            {
                std::cerr << "Attachment root did not receive the expected semantic socket cue at " << width << "x" << height << ".\n";
                return false;
            }

            const uint32_t rootMinY = height / 2u;
            const Color rootHighlightSide = first.Ship.FinalImage.getPixel(width / 2u, rootMinY);
            const Color rootShadowSide = first.Ship.FinalImage.getPixel(width / 2u, rootMinY + 1u);
            if (rootHighlightSide != palette.MechanicalBase || rootShadowSide != palette.MechanicalDark)
            {
                std::cerr << "Weapon root collar did not preserve directional mount semantics at " << width << "x" << height << ".\n";
                return false;
            }

            const uint32_t hullMaxY = height - 1u - height / 6u;
            const uint32_t engineMinY = hullMaxY - 4u;
            const Color engineJoin = first.Ship.FinalImage.getPixel(width / 2u, engineMinY - 1u);
            if (engineJoin != palette.HullDeepShadow)
            {
                std::cerr << "Engine housing did not receive the expected inboard join seam at " << width << "x" << height << ".\n";
                return false;
            }
        }
        else if (attachmentSocket == palette.MechanicalDark)
        {
            std::cerr << "Tiny-resolution component-depth treatment was not restrained at " << width << "x" << height << ".\n";
            return false;
        }

        return true;
    }
}

int PixelShipGeneratorTests::runComponentDepthReadabilityRegression()
{
    if (!runSemanticCueCase(24u, 24u, false))
    {
        return 1;
    }

    if (!runSemanticCueCase(64u, 64u, true))
    {
        return 1;
    }

    if (!runSemanticCueCase(96u, 64u, true))
    {
        return 1;
    }

    std::cout << "Component depth/readability regression passed.\n";
    return 0;
}
