#include "ShipPainter.h"

#include <algorithm>
#include <cstdint>
#include <limits>

#include "GenerationMath.h"
#include "PixelMaskUtils.h"


namespace
{
    struct HorizontalMaskSpan
    {
        uint32_t StartX = 0u;
        uint32_t EndX = 0u;
        uint32_t Width = 0u;
    };

    HorizontalMaskSpan getHorizontalMaskSpan(const PixelShipGenerator::PixelMask& mask, uint32_t x, uint32_t y)
    {
        HorizontalMaskSpan span;
        span.StartX = x;
        span.EndX = x;

        while (span.StartX > 0u && mask.get(span.StartX - 1u, y)) { --span.StartX; }
        while (span.EndX + 1u < mask.getWidth() && mask.get(span.EndX + 1u, y)) { ++span.EndX; }

        span.Width = span.EndX - span.StartX + 1u;
        return span;
    }

    PixelShipGenerator::Color getHullSurfaceToneColor(PixelShipGenerator::ShipHullSurfaceTone tone, const PixelShipGenerator::ShipPalette& palette)
    {
        switch (tone)
        {
        case PixelShipGenerator::ShipHullSurfaceTone::BASE: return palette.HullBase;
        case PixelShipGenerator::ShipHullSurfaceTone::HIGHLIGHT: return palette.HullHighlight;
        case PixelShipGenerator::ShipHullSurfaceTone::SECONDARY:
        default: return palette.HullSecondary;
        }
    }

    PixelShipGenerator::Color resolveFactionPaintColor(PixelShipGenerator::ShipFactionPaintColorRole role, const PixelShipGenerator::Color& defaultColor, const PixelShipGenerator::ShipPalette& palette)
    {
        switch (role)
        {
        case PixelShipGenerator::ShipFactionPaintColorRole::HULL_BASE: return palette.HullBase;
        case PixelShipGenerator::ShipFactionPaintColorRole::HULL_SECONDARY: return palette.HullSecondary;
        case PixelShipGenerator::ShipFactionPaintColorRole::HULL_HIGHLIGHT: return palette.HullHighlight;
        case PixelShipGenerator::ShipFactionPaintColorRole::HULL_ACCENT: return palette.HullAccent;
        case PixelShipGenerator::ShipFactionPaintColorRole::HULL_ACCENT_HIGHLIGHT: return palette.HullAccentHighlight;
        case PixelShipGenerator::ShipFactionPaintColorRole::MECHANICAL_BASE: return palette.MechanicalBase;
        case PixelShipGenerator::ShipFactionPaintColorRole::ENGINE_BASE: return palette.EngineBase;
        case PixelShipGenerator::ShipFactionPaintColorRole::ENGINE_HIGHLIGHT: return palette.EngineHighlight;
        case PixelShipGenerator::ShipFactionPaintColorRole::ENGINE_HOT_CORE: return palette.EngineHotCore;
        case PixelShipGenerator::ShipFactionPaintColorRole::LIGHT_BASE: return palette.LightBase;
        case PixelShipGenerator::ShipFactionPaintColorRole::LIGHT_HIGHLIGHT: return palette.LightHighlight;
        case PixelShipGenerator::ShipFactionPaintColorRole::PROFILE_DEFAULT:
        default: return defaultColor;
        }
    }
}

namespace PixelShipGenerator
{
    void ShipPainter::paint(ShipGenerationContext& context) const
    {
        GeneratedShip& ship = context.Ship;
        const ShipPalette& palette = ship.Palette;
        const ShipGenerationProfile& profile = context.Profile;

        paintHull(context, ship, palette, profile);
        paintMaterialComposition(context, ship, palette);
        paintCoreTreatment(context, ship, palette);
        paintHullLayers(context, ship, palette);
        paintMajorFeatures(context, ship, palette);
        paintWeapons(context, ship, palette);
        paintAttachments(ship, palette, context.ScaleTraits);
        paintLivery(context, ship, palette);
        paintDetails(ship, palette);
        paintCockpit(context, ship, palette, context.ScaleTraits);
        paintEngines(context, ship, palette, context.ScaleTraits);
        paintComponentDepthReadability(context, ship, palette);
    }


    void ShipPainter::paintIdleWeaponLayer(Image& image, const GeneratedShip& ship, const PixelMask& occupiedMask, const PixelMask& movableMask, const PixelMask& muzzleMask, const PixelMask& emissiveMask, const PixelMask& affectedMask) const
    {
        const ShipPalette& palette = ship.Palette;

        for (uint32_t y = 0u; y < affectedMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < affectedMask.getWidth(); ++x)
            {
                if (!affectedMask.get(x, y))
                {
                    continue;
                }

                if (occupiedMask.get(x, y) && movableMask.get(x, y))
                {
                    if (emissiveMask.get(x, y))
                    {
                        image.setPixel(x, y, muzzleMask.get(x, y) ? palette.LightHighlight : palette.LightBase);
                    }
                    else if (muzzleMask.get(x, y))
                    {
                        image.setPixel(x, y, ship.Faction == ShipFactionType::RELIC ? palette.LightHighlight
                            : (ship.Faction == ShipFactionType::ASCENDANT || ship.Faction == ShipFactionType::XENO ? palette.HullAccentHighlight : palette.EngineHighlight));
                    }
                    else
                    {
                        image.setPixel(x, y, getDirectionalMaskColor(movableMask, x, y, palette.MechanicalDark, palette.EngineDark, palette.EngineBase, palette.EngineHighlight, palette.EngineHighlight));
                    }

                    continue;
                }

                if (occupiedMask.get(x, y))
                {
                    continue;
                }

                if (ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y) || ship.AttachmentMask.get(x, y))
                {
                    continue;
                }

                image.setPixel(x, y, PixelMaskUtils::hasNeighbouringMaskPixel(occupiedMask, static_cast<int32_t>(x), static_cast<int32_t>(y)) ? palette.Outline : palette.Transparent);
            }
        }
    }

    void ShipPainter::paintHull(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette, const ShipGenerationProfile& profile) const
    {
        ship.FinalImage.clear(palette.Transparent);

        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                if (isShipStructurePixel(ship, static_cast<int32_t>(x), static_cast<int32_t>(y)))
                {
                    continue;
                }

                if (!hasNeighbouringShipStructurePixel(ship, static_cast<int32_t>(x), static_cast<int32_t>(y)))
                {
                    continue;
                }

                ship.FinalImage.setPixel(x, y, palette.Outline);
            }
        }

        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                if (!ship.HullMask.get(x, y))
                {
                    continue;
                }

                ship.FinalImage.setPixel(x, y, getHullPixelColor(context, ship, x, y, palette, profile));
            }
        }
    }

    void ShipPainter::paintMaterialComposition(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        const MaterialCompositionData& material = context.MaterialComposition;
        if (material.empty()) { return; }

        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                if (material.MechanicalMask.get(x, y))
                {
                    ship.FinalImage.setPixel(x, y, getDirectionalMaskColor(material.MechanicalMask, x, y, palette.MechanicalDark, palette.MechanicalDark, palette.MechanicalBase, palette.EngineHighlight, palette.EngineHighlight));
                }
                else if (material.SecondaryHullMask.get(x, y))
                {
                    ship.FinalImage.setPixel(x, y, getDirectionalMaskColor(material.SecondaryHullMask, x, y, palette.HullDeepShadow, palette.HullShadow, palette.HullSecondary, palette.HullHighlight, palette.HullEdgeHighlight));
                }
            }
        }
    }

    void ShipPainter::paintCoreTreatment(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        const CoreTreatmentData& core = context.CoreTreatment;
        if (core.empty()) { return; }

        const uint32_t shadingComplexity = context.ScaleTraits.ShadingComplexity;
        const bool strongBevel = shadingComplexity >= 50u;
        const bool reflectedRecessEdge = shadingComplexity >= 28u;
        if (shadingComplexity >= 25u && PixelMaskUtils::getMaskPixelCount(core.RaisedMask) > 0u)
        {
            paintLowerRightContactShadow(ship, core.RaisedMask, ship.HullMask, palette.HullDeepShadow, shadingComplexity >= 90u ? 2u : 1u);
        }

        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                if (core.RecessedMask.get(x, y))
                {
                    ship.FinalImage.setPixel(x, y, getRecessedMaskColor(core.RecessedMask, x, y, palette.MechanicalDark, palette, reflectedRecessEdge));
                }
                else if (core.SecondaryMaterialMask.get(x, y))
                {
                    const Color material = resolveFactionPaintColor(context.FactionProfile.Finish.CoreSecondaryMaterialRole, palette.HullSecondary, palette);
                    ship.FinalImage.setPixel(x, y, getDirectionalMaskColor(core.SecondaryMaterialMask, x, y, palette.HullDeepShadow, palette.HullShadow, material, palette.HullHighlight, palette.HullEdgeHighlight));
                }
            }
        }

        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                if (!core.RaisedMask.get(x, y)) { continue; }
                const Color defaultInterior = getHullSurfaceToneColor(context.Profile.CoreRaisedSurfaceTone, palette);
                const Color interior = resolveFactionPaintColor(context.FactionProfile.Finish.CoreRaisedRole, defaultInterior, palette);
                ship.FinalImage.setPixel(x, y, getRaisedMaskColor(core.RaisedMask, x, y, interior, palette, strongBevel));
            }
        }

        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                if (!core.LuminousMask.get(x, y)) { continue; }
                const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(core.LuminousMask, x, y);
                Color color = resolveFactionPaintColor(context.FactionProfile.Finish.CoreLuminousRole, palette.LightBase, palette);
                if (exposure.Top || exposure.Left)
                {
                    color = resolveFactionPaintColor(context.FactionProfile.Finish.CoreLuminousHighlightRole, palette.LightHighlight, palette);
                }
                ship.FinalImage.setPixel(x, y, color);
            }
        }
    }

    void ShipPainter::paintHullLayers(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        const HullLayerData& layers = context.HullLayers;
        if (layers.empty()) { return; }

        const uint32_t shadingComplexity = context.ScaleTraits.ShadingComplexity;
        const bool strongBevel = shadingComplexity >= 40u;
        const uint32_t contactShadowDistance = shadingComplexity >= 90u ? 2u : 1u;

        for (uint32_t order = 0u; order <= 1u; ++order)
        {
            for (const HullLayerPlacement& placement : layers.Placements)
            {
                if (placement.Order != order) { continue; }

                if (shadingComplexity >= 20u)
                {
                    paintLowerRightContactShadow(ship, placement.Mask, ship.HullMask, order == 0u ? palette.HullShadow : palette.HullDeepShadow, contactShadowDistance);
                }

                Color interiorColor = order == 0u ? palette.HullSecondary : palette.HullBase;
                if (placement.Type == ShipHullLayerType::CENTRAL_DORSAL_PLATE)
                {
                    const Color defaultInterior = getHullSurfaceToneColor(context.Profile.CentralDorsalPlateTone, palette);
                    interiorColor = resolveFactionPaintColor(context.FactionProfile.Finish.CentralDorsalPlateRole, defaultInterior, palette);
                }
                else if (placement.Type == ShipHullLayerType::REAR_ENGINE_COVER)
                {
                    interiorColor = palette.HullShadow;
                }

                for (uint32_t y = placement.MinY; y <= placement.MaxY; ++y)
                {
                    for (uint32_t x = placement.MinX; x <= placement.MaxX; ++x)
                    {
                        if (!placement.Mask.get(x, y)) { continue; }
                        ship.FinalImage.setPixel(x, y, getRaisedMaskColor(placement.Mask, x, y, interiorColor, palette, strongBevel));
                    }
                }
            }
        }
    }

    void ShipPainter::paintMajorFeatures(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        const MajorFeatureData& features = context.MajorFeatures;
        const uint32_t shadingComplexity = context.ScaleTraits.ShadingComplexity;
        const bool strongBevel = shadingComplexity >= 40u;
        const bool reflectedRecessEdge = shadingComplexity >= 20u;
        const uint32_t contactShadowDistance = shadingComplexity >= 90u ? 2u : 1u;

        if (shadingComplexity >= 20u)
        {
            paintLowerRightContactShadow(ship, features.RaisedMask, ship.HullMask, palette.HullDeepShadow, contactShadowDistance);
        }

        for (uint32_t y = 0u; y < features.OccupiedMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < features.OccupiedMask.getWidth(); ++x)
            {
                if (!features.OccupiedMask.get(x, y))
                {
                    continue;
                }

                const MajorFeaturePlacement* placement = getMajorFeaturePlacementForPixel(features, x, y);

                if (features.EmissiveMask.get(x, y))
                {
                    const uint32_t maximumDepth = shadingComplexity >= 80u ? 2u : 1u;
                    const uint32_t depth = PixelMaskUtils::getMaskDepth(features.EmissiveMask, x, y, maximumDepth);
                    ship.FinalImage.setPixel(x, y, depth >= maximumDepth ? palette.LightHighlight : palette.LightBase);
                    continue;
                }

                if (features.MechanicalMask.get(x, y))
                {
                    const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(features.MechanicalMask, x, y);
                    Color mechanicalColor = palette.MechanicalDark;

                    if (exposure.getShadowExposure() > exposure.getHighlightExposure())
                    {
                        mechanicalColor = palette.MechanicalBase;
                    }

                    ship.FinalImage.setPixel(x, y, mechanicalColor);
                    continue;
                }

                if (features.RecessedMask.get(x, y))
                {
                    ship.FinalImage.setPixel(x, y, getRecessedMaskColor(features.RecessedMask, x, y, palette.HullDeepShadow, palette, reflectedRecessEdge));
                    continue;
                }

                if (features.RaisedMask.get(x, y))
                {
                    Color interiorColor = palette.HullSecondary;

                    if (placement != nullptr && placement->Type == ShipMajorFeatureType::CENTRAL_SPINE)
                    {
                        interiorColor = palette.HullHighlight;
                    }
                    else if (placement != nullptr && placement->Type == ShipMajorFeatureType::WING_PLATE && shadingComplexity < 40u)
                    {
                        interiorColor = palette.HullBase;
                    }

                    ship.FinalImage.setPixel(x, y, getRaisedMaskColor(features.RaisedMask, x, y, interiorColor, palette, strongBevel));
                }
            }
        }
    }

    void ShipPainter::paintWeapons(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        const WeaponData& weapons = context.Weapons;
        const uint32_t shadingComplexity = context.ScaleTraits.ShadingComplexity;
        const bool strongBevel = shadingComplexity >= 40u;
        const uint32_t contactShadowDistance = shadingComplexity >= 90u ? 2u : 1u;

        if (shadingComplexity >= 20u)
        {
            paintLowerRightContactShadow(ship, weapons.BodyMask, ship.HullMask, palette.HullDeepShadow, contactShadowDistance);
            paintLowerRightContactShadow(ship, weapons.RootMask, ship.HullMask, palette.MechanicalDark, 1u);
        }

        for (uint32_t y = 0u; y < weapons.OccupiedMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < weapons.OccupiedMask.getWidth(); ++x)
            {
                if (weapons.OccupiedMask.get(x, y))
                {
                    continue;
                }

                if (ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y) || ship.AttachmentMask.get(x, y))
                {
                    continue;
                }

                if (PixelMaskUtils::hasNeighbouringMaskPixel(weapons.OccupiedMask, static_cast<int32_t>(x), static_cast<int32_t>(y)))
                {
                    ship.FinalImage.setPixel(x, y, palette.Outline);
                }
            }
        }

        for (uint32_t y = 0u; y < weapons.OccupiedMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < weapons.OccupiedMask.getWidth(); ++x)
            {
                if (!weapons.OccupiedMask.get(x, y))
                {
                    continue;
                }

                if (weapons.EmissiveMask.get(x, y))
                {
                    ship.FinalImage.setPixel(x, y, weapons.MuzzleMask.get(x, y) ? palette.LightHighlight : palette.LightBase);
                    continue;
                }

                if (weapons.RootMask.get(x, y))
                {
                    const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(weapons.RootMask, x, y);
                    ship.FinalImage.setPixel(x, y, exposure.getShadowExposure() > exposure.getHighlightExposure() ? palette.MechanicalBase : palette.MechanicalDark);
                    continue;
                }

                if (weapons.MuzzleMask.get(x, y))
                {
                    ship.FinalImage.setPixel(x, y, resolveFactionPaintColor(context.FactionProfile.Finish.WeaponMuzzleRole, palette.EngineHighlight, palette));
                    continue;
                }

                if (weapons.BarrelMask.get(x, y))
                {
                    ship.FinalImage.setPixel(x, y, getDirectionalMaskColor(weapons.BarrelMask, x, y, palette.MechanicalDark, palette.EngineDark, palette.EngineBase, palette.EngineHighlight, palette.EngineHighlight));
                    continue;
                }

                if (weapons.BodyMask.get(x, y))
                {
                    const Color interiorColor = resolveFactionPaintColor(context.FactionProfile.Finish.WeaponBodyRole, palette.EngineBase, palette);
                    Color bodyColor = getRaisedMaskColor(weapons.BodyMask, x, y, interiorColor, palette, strongBevel);

                    if (bodyColor == palette.HullHighlight)
                    {
                        bodyColor = resolveFactionPaintColor(context.FactionProfile.Finish.WeaponRaisedHighlightRole, palette.HullHighlight, palette);
                    }

                    ship.FinalImage.setPixel(x, y, bodyColor);
                }
            }
        }
    }

    void ShipPainter::paintAttachments(GeneratedShip& ship, const ShipPalette& palette, const GenerationScaleTraits& scaleTraits) const
    {
        for (uint32_t y = 0u; y < ship.AttachmentMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.AttachmentMask.getWidth(); ++x)
            {
                if (!ship.AttachmentMask.get(x, y))
                {
                    continue;
                }

                const ShipAttachmentPlacement* placement = getAttachmentPlacementForPixel(ship, x, y);

                if (placement == nullptr)
                {
                    ship.FinalImage.setPixel(x, y, getDirectionalStructureColor(ship, x, y, palette.HullDeepShadow, palette.HullShadow, palette.HullSecondary, palette.HullHighlight, palette.HullEdgeHighlight));
                    continue;
                }

                ship.FinalImage.setPixel(x, y, getAttachmentPixelColor(ship, *placement, x, y, palette, scaleTraits.ShadingComplexity));
            }
        }
    }

    void ShipPainter::paintCockpit(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette, const GenerationScaleTraits& scaleTraits) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(ship.CockpitMask);
        if (!bounds.Valid) { return; }

        if (context.Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS)
        {
            for (uint32_t y = bounds.MinY; y <= bounds.MaxY; ++y)
            {
                for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
                {
                    if (!ship.CockpitMask.get(x, y)) { continue; }
                    ship.FinalImage.setPixel(x, y, getLegacyCockpitPixelColor(ship.CockpitMask, bounds, x, y, palette, scaleTraits.ShadingComplexity));
                }
            }
            return;
        }

        const bool strongStructure = context.Cockpit.SizeClass >= CockpitSizeClass::LARGE || scaleTraits.ShadingComplexity >= 65u;
        const uint32_t shadowDistance = context.Cockpit.SizeClass == CockpitSizeClass::MASSIVE && scaleTraits.ShadingComplexity >= 60u ? 2u : 1u;
        if (scaleTraits.ShadingComplexity >= 20u)
        {
            paintLowerRightContactShadow(ship, ship.CockpitMask, ship.HullMask, palette.HullDeepShadow, shadowDistance);
        }

        // Structural collar/base is the lowest cockpit stage.
        for (uint32_t y = bounds.MinY; y <= bounds.MaxY; ++y)
        {
            for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
            {
                if (!context.Cockpit.BaseMask.get(x, y)) { continue; }
                const Color baseInterior = resolveFactionPaintColor(context.FactionProfile.Finish.CockpitBaseRole, palette.HullSecondary, palette);
                ship.FinalImage.setPixel(x, y, getRaisedMaskColor(context.Cockpit.BaseMask, x, y, baseInterior, palette, strongStructure));
            }
        }

        // Raised frame uses the same Task-38 directional bevel semantics as other raised structures.
        for (uint32_t y = bounds.MinY; y <= bounds.MaxY; ++y)
        {
            for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
            {
                if (!context.Cockpit.FrameMask.get(x, y)) { continue; }
                const Color interior = resolveFactionPaintColor(context.FactionProfile.Finish.CockpitFrameRole, palette.HullSecondary, palette);
                ship.FinalImage.setPixel(x, y, getRaisedMaskColor(context.Cockpit.FrameMask, x, y, interior, palette, strongStructure));
            }
        }

        for (uint32_t y = bounds.MinY; y <= bounds.MaxY; ++y)
        {
            for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
            {
                if (!context.Cockpit.GlassMask.get(x, y)) { continue; }
                ship.FinalImage.setPixel(x, y, getCockpitGlassPixelColor(context.Cockpit, bounds, x, y, palette, scaleTraits.ShadingComplexity));
            }
        }

        // Dorsal/layered bridge upper sections are a second discrete height stage.
        // Shadow the lower cockpit surface, then repaint the upper glass so the
        // ordering remains Base Hull -> Cockpit Base/Frame -> Upper Bridge.
        if (PixelMaskUtils::getMaskPixelCount(context.Cockpit.UpperSectionMask) > 0u && scaleTraits.ShadingComplexity >= 35u)
        {
            paintLowerRightContactShadow(ship, context.Cockpit.UpperSectionMask, ship.CockpitMask, palette.CockpitDark, 1u);
            for (uint32_t y = bounds.MinY; y <= bounds.MaxY; ++y)
            {
                for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
                {
                    if (!context.Cockpit.UpperSectionMask.get(x, y)) { continue; }
                    Color color = getCockpitGlassPixelColor(context.Cockpit, bounds, x, y, palette, scaleTraits.ShadingComplexity);
                    const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(context.Cockpit.UpperSectionMask, x, y);
                    if (exposure.Top || exposure.Left) { color = palette.CockpitGlint; }
                    else if (!exposure.Bottom && !exposure.Right) { color = palette.CockpitHighlight; }
                    ship.FinalImage.setPixel(x, y, color);
                }
            }
        }
    }

    void ShipPainter::paintEngines(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette, const GenerationScaleTraits& scaleTraits) const
    {
        for (uint32_t y = 0u; y < ship.EngineExhaustMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.EngineExhaustMask.getWidth(); ++x)
            {
                if (!ship.EngineExhaustMask.get(x, y))
                {
                    continue;
                }

                const HorizontalMaskSpan span = getHorizontalMaskSpan(ship.EngineExhaustMask, x, y);
                const uint32_t distanceFromLeft = x - span.StartX;
                const uint32_t distanceFromRight = span.EndX - x;
                const uint32_t edgeDistance = std::min(distanceFromLeft, distanceFromRight);
                const bool engineAbove = PixelMaskUtils::isMaskPixel(ship.EngineMask, static_cast<int32_t>(x), static_cast<int32_t>(y) - 1);
                const bool centerPixel = distanceFromLeft == distanceFromRight || (span.Width % 2u == 0u && (distanceFromLeft + 1u == distanceFromRight || distanceFromRight + 1u == distanceFromLeft));

                if (engineAbove || span.Width == 1u || (centerPixel && edgeDistance >= 1u))
                {
                    ship.FinalImage.setPixel(x, y, palette.ExhaustHotCore);
                }
                else if (edgeDistance >= 1u || centerPixel)
                {
                    ship.FinalImage.setPixel(x, y, palette.ExhaustHighlight);
                }
                else
                {
                    ship.FinalImage.setPixel(x, y, palette.ExhaustBase);
                }
            }
        }

        for (uint32_t y = 0u; y < ship.EngineMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.EngineMask.getWidth(); ++x)
            {
                if (ship.EngineMask.get(x, y) || ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.AttachmentMask.get(x, y) || ship.EngineExhaustMask.get(x, y))
                {
                    continue;
                }

                if (PixelMaskUtils::hasNeighbouringMaskPixel(ship.EngineMask, static_cast<int32_t>(x), static_cast<int32_t>(y)))
                {
                    ship.FinalImage.setPixel(x, y, palette.Outline);
                }
            }
        }

        for (uint32_t y = 0u; y < ship.EngineMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.EngineMask.getWidth(); ++x)
            {
                if (!ship.EngineMask.get(x, y))
                {
                    continue;
                }

                ship.FinalImage.setPixel(x, y, getEnginePixelColor(ship, x, y, palette, scaleTraits.ShadingComplexity, context.FactionProfile.Finish));
            }
        }
    }


    void ShipPainter::paintLivery(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        if (context.Livery.empty()) { return; }

        const auto chooseContrastColor = [&](const Color& background, bool secondary)
            {
                const Color candidates[3] = { palette.HullAccentDark, palette.HullAccent, palette.HullAccentHighlight };
                const uint32_t backgroundLuma = getColorLuma(background);
                uint32_t bestIndex = secondary ? 0u : 1u;
                uint32_t bestDifference = 0u;
                for (uint32_t index = 0u; index < 3u; ++index)
                {
                    const uint32_t candidateLuma = getColorLuma(candidates[index]);
                    uint32_t difference = backgroundLuma > candidateLuma ? backgroundLuma - candidateLuma : candidateLuma - backgroundLuma;
                    if (secondary && index == 1u) { difference = difference * 85u / 100u; }
                    if (difference > bestDifference)
                    {
                        bestDifference = difference;
                        bestIndex = index;
                    }
                }
                return candidates[bestIndex];
            };

        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                const bool primary = context.Livery.PrimaryMarkingMask.get(x, y);
                const bool secondary = context.Livery.SecondaryMarkingMask.get(x, y);
                if (!primary && !secondary) { continue; }
                ship.FinalImage.setPixel(x, y, chooseContrastColor(ship.FinalImage.getPixel(x, y), secondary && !primary));
            }
        }
    }

    void ShipPainter::paintDetails(GeneratedShip& ship, const ShipPalette& palette) const
    {
        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                if (ship.AccentMask.get(x, y))
                {
                    const Color backgroundColor = ship.FinalImage.getPixel(x, y);
                    Color accentColor = getAccentPixelColor(ship.AccentMask, x, y, palette);
                    const uint32_t backgroundLuma = getColorLuma(backgroundColor);
                    const uint32_t accentLuma = getColorLuma(accentColor);
                    const uint32_t lumaDifference = backgroundLuma > accentLuma ? backgroundLuma - accentLuma : accentLuma - backgroundLuma;

                    if (lumaDifference < 24u)
                    {
                        const uint32_t darkLuma = getColorLuma(palette.HullAccentDark);
                        const uint32_t highlightLuma = getColorLuma(palette.HullAccentHighlight);
                        const uint32_t darkDifference = backgroundLuma > darkLuma ? backgroundLuma - darkLuma : darkLuma - backgroundLuma;
                        const uint32_t highlightDifference = backgroundLuma > highlightLuma ? backgroundLuma - highlightLuma : highlightLuma - backgroundLuma;
                        accentColor = highlightDifference >= darkDifference ? palette.HullAccentHighlight : palette.HullAccentDark;
                    }

                    ship.FinalImage.setPixel(x, y, accentColor);
                }
                else if (ship.MechanicalDetailMask.get(x, y))
                {
                    ship.FinalImage.setPixel(x, y, getMechanicalPixelColor(ship.MechanicalDetailMask, x, y, palette));
                }
                else if (ship.LightMask.get(x, y))
                {
                    ship.FinalImage.setPixel(x, y, getLightPixelColor(ship.LightMask, x, y, palette));
                }
            }
        }
    }


    void ShipPainter::paintComponentDepthReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        const uint32_t shadingComplexity = context.ScaleTraits.ShadingComplexity;
        if (shadingComplexity < 20u)
        {
            return;
        }

        // Reassert semantic mounting after livery/details have been painted.  This
        // intentionally derives only from existing component geometry and does
        // not change any structural mask or consume RNG.
        paintWeaponMountReadability(context, ship, palette);
        paintAttachmentMountReadability(context, ship, palette);
        paintCockpitReadability(context, ship, palette);
        paintEngineMountReadability(context, ship, palette);

        if (shadingComplexity >= 40u)
        {
            const bool strongShadow = shadingComplexity >= 75u;
            paintSemanticContactShadow(context, ship, context.CoreTreatment.RaisedMask, strongShadow);
            for (const HullLayerPlacement& placement : context.HullLayers.Placements)
            {
                paintSemanticContactShadow(context, ship, placement.Mask, strongShadow && placement.Order > 0u);
            }
            paintSemanticContactShadow(context, ship, context.MajorFeatures.RaisedMask, strongShadow);
            paintSemanticContactShadow(context, ship, ship.EngineMask, false);
            paintWingRootReadability(context, ship, palette);
        }
    }

    void ShipPainter::paintSemanticContactShadow(const ShipGenerationContext& context, GeneratedShip& ship, const PixelMask& elevatedMask, bool strongShadow) const
    {
        if (PixelMaskUtils::getMaskPixelCount(elevatedMask) == 0u)
        {
            return;
        }

        for (uint32_t y = 0u; y < elevatedMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < elevatedMask.getWidth(); ++x)
            {
                if (!elevatedMask.get(x, y))
                {
                    continue;
                }

                const int32_t targets[3][2] =
                {
                    { static_cast<int32_t>(x) + 1, static_cast<int32_t>(y) },
                    { static_cast<int32_t>(x), static_cast<int32_t>(y) + 1 },
                    { static_cast<int32_t>(x) + 1, static_cast<int32_t>(y) + 1 }
                };

                for (const auto& target : targets)
                {
                    if (target[0] < 0 || target[1] < 0 || target[0] >= static_cast<int32_t>(ship.FinalImage.getWidth()) || target[1] >= static_cast<int32_t>(ship.FinalImage.getHeight()))
                    {
                        continue;
                    }

                    const uint32_t targetX = static_cast<uint32_t>(target[0]);
                    const uint32_t targetY = static_cast<uint32_t>(target[1]);
                    if (elevatedMask.get(targetX, targetY) || !isDepthSupportHullPixel(context, targetX, targetY))
                    {
                        continue;
                    }

                    const Color currentColor = ship.FinalImage.getPixel(targetX, targetY);
                    ship.FinalImage.setPixel(targetX, targetY, getDepthShadowColor(currentColor, ship.Palette, strongShadow));
                }
            }
        }
    }

    void ShipPainter::paintWeaponMountReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        const WeaponData& weapons = context.Weapons;
        if (weapons.empty())
        {
            return;
        }

        // Root masks are explicit hull-supported weapon sockets.  Painting them
        // again after livery prevents a marking from flattening the mount back
        // into the hull plane.
        for (uint32_t y = 0u; y < weapons.RootMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < weapons.RootMask.getWidth(); ++x)
            {
                if (!weapons.RootMask.get(x, y))
                {
                    continue;
                }

                const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(weapons.RootMask, x, y);
                if (context.ScaleTraits.ShadingComplexity >= 40u)
                {
                    const uint32_t highlightExposure = exposure.getHighlightExposure();
                    const uint32_t shadowExposure = exposure.getShadowExposure();
                    ship.FinalImage.setPixel(x, y, highlightExposure > shadowExposure ? palette.MechanicalBase
                        : (shadowExposure > highlightExposure ? palette.MechanicalDark : palette.EngineDark));
                }
                else
                {
                    ship.FinalImage.setPixel(x, y, exposure.getShadowExposure() > exposure.getHighlightExposure() ? palette.MechanicalBase : palette.MechanicalDark);
                }
            }
        }

        if (context.ScaleTraits.ShadingComplexity < 40u)
        {
            return;
        }

        // Large enough weapon housings receive a one-pixel support shadow and a
        // raised lip where the body itself overlaps hull pixels.  Interiors are
        // deliberately left untouched so livery can still read through broad
        // structural surfaces rather than being erased wholesale.
        paintSemanticContactShadow(context, ship, weapons.BodyMask, context.ScaleTraits.ShadingComplexity >= 75u);
        const bool strongBevel = context.ScaleTraits.ShadingComplexity >= 65u;
        for (uint32_t y = 0u; y < weapons.BodyMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < weapons.BodyMask.getWidth(); ++x)
            {
                if (!weapons.BodyMask.get(x, y) || !ship.HullMask.get(x, y))
                {
                    continue;
                }

                const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(weapons.BodyMask, x, y);
                if (!exposure.isBoundary())
                {
                    continue;
                }

                const Color currentColor = ship.FinalImage.getPixel(x, y);
                Color bodyInterior = currentColor;
                if (currentColor == palette.HullAccent || currentColor == palette.HullAccentHighlight || currentColor == palette.HullAccentDark)
                {
                    bodyInterior = palette.HullAccent;
                }
                ship.FinalImage.setPixel(x, y, getRaisedMaskColor(weapons.BodyMask, x, y, bodyInterior, palette, strongBevel));
            }
        }
    }

    void ShipPainter::paintAttachmentMountReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        const uint32_t shadingComplexity = context.ScaleTraits.ShadingComplexity;
        for (const ShipAttachmentPlacement& placement : ship.AttachmentPlacements)
        {
            const uint32_t maximumOutwardDistance = getAttachmentMaximumOutwardDistance(placement);
            if (maximumOutwardDistance < 2u)
            {
                continue;
            }

            const bool fullRootSocket = shadingComplexity >= 40u;
            for (uint32_t y = placement.MinimumY; y <= placement.MaximumY && y < ship.AttachmentMask.getHeight(); ++y)
            {
                for (uint32_t x = placement.MinimumX; x <= placement.MaximumX && x < ship.AttachmentMask.getWidth(); ++x)
                {
                    if (!ship.AttachmentMask.get(x, y) || getAttachmentOutwardDistance(placement, x, y) != 1u)
                    {
                        continue;
                    }

                    if (!fullRootSocket && getAttachmentTangentCoordinate(placement, x, y) != 0)
                    {
                        continue;
                    }

                    int32_t supportX = static_cast<int32_t>(x);
                    int32_t supportY = static_cast<int32_t>(y);
                    switch (placement.Direction)
                    {
                    case ShipAttachmentDirection::LEFT: ++supportX; break;
                    case ShipAttachmentDirection::RIGHT: --supportX; break;
                    case ShipAttachmentDirection::UP: ++supportY; break;
                    case ShipAttachmentDirection::DOWN: --supportY; break;
                    default: break;
                    }

                    if (supportX < 0 || supportY < 0 || supportX >= static_cast<int32_t>(ship.FinalImage.getWidth()) || supportY >= static_cast<int32_t>(ship.FinalImage.getHeight()))
                    {
                        continue;
                    }

                    const uint32_t targetX = static_cast<uint32_t>(supportX);
                    const uint32_t targetY = static_cast<uint32_t>(supportY);
                    if (!isDepthSupportHullPixel(context, targetX, targetY))
                    {
                        continue;
                    }

                    const Color currentColor = ship.FinalImage.getPixel(targetX, targetY);
                    if (currentColor == palette.HullAccent || currentColor == palette.HullAccentHighlight || currentColor == palette.HullAccentDark)
                    {
                        ship.FinalImage.setPixel(targetX, targetY, palette.HullAccentDark);
                    }
                    else
                    {
                        ship.FinalImage.setPixel(targetX, targetY, fullRootSocket ? palette.MechanicalDark : palette.HullDeepShadow);
                    }
                }
            }
        }
    }

    void ShipPainter::paintCockpitReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        if (context.Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS || context.ScaleTraits.ShadingComplexity < 60u)
        {
            return;
        }

        // A dark inner edge on the lower/right side of a structural frame reads
        // as a recess without brightening or enlarging the canopy.
        for (uint32_t y = 0u; y < context.Cockpit.FrameMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < context.Cockpit.FrameMask.getWidth(); ++x)
            {
                if (!context.Cockpit.FrameMask.get(x, y))
                {
                    continue;
                }

                const int32_t pixelX = static_cast<int32_t>(x);
                const int32_t pixelY = static_cast<int32_t>(y);
                const bool glassLeft = PixelMaskUtils::isMaskPixel(context.Cockpit.GlassMask, pixelX - 1, pixelY);
                const bool glassAbove = PixelMaskUtils::isMaskPixel(context.Cockpit.GlassMask, pixelX, pixelY - 1);
                if (glassLeft || glassAbove)
                {
                    ship.FinalImage.setPixel(x, y, palette.HullDeepShadow);
                }
            }
        }
    }

    void ShipPainter::paintEngineMountReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        if (context.ScaleTraits.ShadingComplexity < 40u)
        {
            return;
        }

        // Engines are rear-mounted assemblies.  A one-pixel seam immediately
        // inboard of the housing gives the rear hull a clear join without
        // changing nozzle/exhaust geometry or adding static effects.
        for (uint32_t y = 0u; y < ship.EngineMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.EngineMask.getWidth(); ++x)
            {
                if (!ship.EngineMask.get(x, y) || y == 0u || PixelMaskUtils::isMaskPixel(ship.EngineMask, static_cast<int32_t>(x), static_cast<int32_t>(y) - 1))
                {
                    continue;
                }

                const uint32_t supportY = y - 1u;
                if (!isDepthSupportHullPixel(context, x, supportY))
                {
                    continue;
                }

                const Color currentColor = ship.FinalImage.getPixel(x, supportY);
                ship.FinalImage.setPixel(x, supportY, currentColor == palette.HullAccent || currentColor == palette.HullAccentHighlight || currentColor == palette.HullAccentDark
                    ? palette.HullAccentDark : palette.HullDeepShadow);
            }
        }
    }

    void ShipPainter::paintWingRootReadability(const ShipGenerationContext& context, GeneratedShip& ship, const ShipPalette& palette) const
    {
        if (!context.WingRegions.hasWings())
        {
            return;
        }

        for (uint32_t y = 0u; y < ship.HullMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < ship.HullMask.getWidth(); ++x)
            {
                if (!isWingInnerBoundaryPixel(context, x, y) || !isDepthSupportHullPixel(context, x, y))
                {
                    continue;
                }

                const Color currentColor = ship.FinalImage.getPixel(x, y);
                ship.FinalImage.setPixel(x, y, getDepthShadowColor(currentColor, palette, context.ScaleTraits.ShadingComplexity >= 75u));
            }
        }
    }

    void ShipPainter::paintLowerRightContactShadow(GeneratedShip& ship, const PixelMask& elevatedMask, const PixelMask& supportMask, const Color& shadowColor, uint32_t shadowDistance) const
    {
        if (shadowDistance == 0u)
        {
            return;
        }

        for (uint32_t y = 0u; y < elevatedMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < elevatedMask.getWidth(); ++x)
            {
                if (!elevatedMask.get(x, y))
                {
                    continue;
                }

                for (uint32_t distance = 1u; distance <= shadowDistance; ++distance)
                {
                    const int32_t targets[3][2] =
                    {
                        { static_cast<int32_t>(x) + static_cast<int32_t>(distance), static_cast<int32_t>(y) },
                        { static_cast<int32_t>(x), static_cast<int32_t>(y) + static_cast<int32_t>(distance) },
                        { static_cast<int32_t>(x) + static_cast<int32_t>(distance), static_cast<int32_t>(y) + static_cast<int32_t>(distance) }
                    };

                    for (const auto& target : targets)
                    {
                        if (target[0] < 0 || target[1] < 0 || target[0] >= static_cast<int32_t>(supportMask.getWidth()) || target[1] >= static_cast<int32_t>(supportMask.getHeight()))
                        {
                            continue;
                        }

                        const uint32_t targetX = static_cast<uint32_t>(target[0]);
                        const uint32_t targetY = static_cast<uint32_t>(target[1]);

                        if (!supportMask.get(targetX, targetY) || elevatedMask.get(targetX, targetY))
                        {
                            continue;
                        }

                        ship.FinalImage.setPixel(targetX, targetY, shadowColor);
                    }
                }
            }
        }
    }

    bool ShipPainter::isShipStructurePixel(const GeneratedShip& ship, int32_t x, int32_t y) const
    {
        if (x < 0 || y < 0)
        {
            return false;
        }

        if (x >= static_cast<int32_t>(ship.HullMask.getWidth()) || y >= static_cast<int32_t>(ship.HullMask.getHeight()))
        {
            return false;
        }

        const uint32_t pixelX = static_cast<uint32_t>(x);
        const uint32_t pixelY = static_cast<uint32_t>(y);

        return ship.HullMask.get(pixelX, pixelY) || ship.AttachmentMask.get(pixelX, pixelY);
    }

    bool ShipPainter::hasNeighbouringShipStructurePixel(const GeneratedShip& ship, int32_t x, int32_t y) const
    {
        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
        {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
            {
                if (offsetX == 0 && offsetY == 0)
                {
                    continue;
                }

                if (isShipStructurePixel(ship, x + offsetX, y + offsetY))
                {
                    return true;
                }
            }
        }

        return false;
    }

    uint32_t ShipPainter::getShipStructureNeighbourCount(const GeneratedShip& ship, int32_t x, int32_t y) const
    {
        uint32_t count = 0u;

        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
        {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
            {
                if (offsetX == 0 && offsetY == 0)
                {
                    continue;
                }

                if (isShipStructurePixel(ship, x + offsetX, y + offsetY))
                {
                    ++count;
                }
            }
        }

        return count;
    }

    bool ShipPainter::isWingInnerBoundaryPixel(const ShipGenerationContext& context, uint32_t x, uint32_t y) const
    {
        const WingRegionData& wings = context.WingRegions;

        if (!wings.hasWings() || !wings.WingMask.get(x, y))
        {
            return false;
        }

        const uint32_t leftCenter = (wings.WingMask.getWidth() - 1u) / 2u;
        const int32_t inwardX = x <= leftCenter ? static_cast<int32_t>(x) + 1 : static_cast<int32_t>(x) - 1;
        return PixelMaskUtils::isMaskPixel(context.Ship.HullMask, inwardX, static_cast<int32_t>(y)) && !PixelMaskUtils::isMaskPixel(wings.WingMask, inwardX, static_cast<int32_t>(y));
    }

    bool ShipPainter::isFuselageWingBoundaryPixel(const ShipGenerationContext& context, uint32_t x, uint32_t y) const
    {
        const WingRegionData& wings = context.WingRegions;

        if (!wings.hasWings() || wings.WingMask.get(x, y) || !context.Ship.HullMask.get(x, y))
        {
            return false;
        }

        return PixelMaskUtils::isMaskPixel(wings.WingMask, static_cast<int32_t>(x) - 1, static_cast<int32_t>(y)) || PixelMaskUtils::isMaskPixel(wings.WingMask, static_cast<int32_t>(x) + 1, static_cast<int32_t>(y));
    }

    bool ShipPainter::isCentralRidgePixel(const ShipGenerationContext& context, uint32_t x, uint32_t y, uint32_t bandWidth) const
    {
        if (bandWidth == 0u || y >= context.WingRegions.FuselageHalfWidths.size() || context.WingRegions.FuselageHalfWidths[y] == 0u || context.WingRegions.WingMask.get(x, y))
        {
            return false;
        }

        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t leftRidge = leftCenter >= bandWidth - 1u ? leftCenter - (bandWidth - 1u) : 0u;
        return x >= leftRidge && x <= leftCenter;
    }

    Color ShipPainter::getDirectionalStructureColor(const GeneratedShip& ship, uint32_t x, uint32_t y, const Color& deepShadow, const Color& shadow, const Color& base, const Color& highlight, const Color& edgeHighlight) const
    {
        const int32_t pixelX = static_cast<int32_t>(x);
        const int32_t pixelY = static_cast<int32_t>(y);

        const bool leftExposed = !isShipStructurePixel(ship, pixelX - 1, pixelY);
        const bool rightExposed = !isShipStructurePixel(ship, pixelX + 1, pixelY);
        const bool topExposed = !isShipStructurePixel(ship, pixelX, pixelY - 1);
        const bool bottomExposed = !isShipStructurePixel(ship, pixelX, pixelY + 1);
        const bool topLeftExposed = !isShipStructurePixel(ship, pixelX - 1, pixelY - 1);
        const bool bottomRightExposed = !isShipStructurePixel(ship, pixelX + 1, pixelY + 1);

        const uint32_t highlightExposure = static_cast<uint32_t>(leftExposed) + static_cast<uint32_t>(topExposed) + static_cast<uint32_t>(topLeftExposed);
        const uint32_t shadowExposure = static_cast<uint32_t>(rightExposed) + static_cast<uint32_t>(bottomExposed) + static_cast<uint32_t>(bottomRightExposed);
        const uint32_t neighbourCount = getShipStructureNeighbourCount(ship, pixelX, pixelY);

        if (highlightExposure >= 2u && shadowExposure == 0u)
        {
            return edgeHighlight;
        }

        if (shadowExposure >= 2u && neighbourCount <= 5u)
        {
            return deepShadow;
        }

        if (highlightExposure > shadowExposure)
        {
            return highlight;
        }

        if (shadowExposure > highlightExposure)
        {
            return shadow;
        }

        return base;
    }

    Color ShipPainter::getDirectionalMaskColor(const PixelMask& mask, uint32_t x, uint32_t y, const Color& deepShadow, const Color& shadow, const Color& base, const Color& highlight, const Color& edgeHighlight) const
    {
        const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(mask, x, y);
        const uint32_t highlightExposure = exposure.getHighlightExposure();
        const uint32_t shadowExposure = exposure.getShadowExposure();

        if (highlightExposure >= 2u && shadowExposure == 0u)
        {
            return edgeHighlight;
        }

        if (shadowExposure >= 2u && highlightExposure == 0u)
        {
            return deepShadow;
        }

        if (highlightExposure > shadowExposure)
        {
            return highlight;
        }

        if (shadowExposure > highlightExposure)
        {
            return shadow;
        }

        return base;
    }

    Color ShipPainter::getRaisedMaskColor(const PixelMask& mask, uint32_t x, uint32_t y, const Color& interiorColor, const ShipPalette& palette, bool strongBevel) const
    {
        const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(mask, x, y);

        if (!exposure.isBoundary())
        {
            return interiorColor;
        }

        const uint32_t highlightExposure = exposure.getHighlightExposure();
        const uint32_t shadowExposure = exposure.getShadowExposure();

        if (highlightExposure > shadowExposure)
        {
            return strongBevel ? palette.HullEdgeHighlight : palette.HullHighlight;
        }

        if (shadowExposure > highlightExposure)
        {
            return strongBevel ? palette.HullDeepShadow : palette.HullShadow;
        }

        if ((exposure.Top || exposure.Left) && !(exposure.Bottom || exposure.Right))
        {
            return palette.HullHighlight;
        }

        if ((exposure.Bottom || exposure.Right) && !(exposure.Top || exposure.Left))
        {
            return palette.HullShadow;
        }

        return interiorColor;
    }

    Color ShipPainter::getRecessedMaskColor(const PixelMask& mask, uint32_t x, uint32_t y, const Color& interiorColor, const ShipPalette& palette, bool reflectedEdge) const
    {
        const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(mask, x, y);

        if (!exposure.isBoundary())
        {
            return interiorColor;
        }

        const uint32_t highlightExposure = exposure.getHighlightExposure();
        const uint32_t shadowExposure = exposure.getShadowExposure();

        if (highlightExposure > shadowExposure)
        {
            return palette.HullDeepShadow;
        }

        if (shadowExposure > highlightExposure)
        {
            return reflectedEdge ? palette.HullBase : palette.HullShadow;
        }

        if ((exposure.Top || exposure.Left) && !(exposure.Bottom || exposure.Right))
        {
            return palette.HullDeepShadow;
        }

        if (reflectedEdge && (exposure.Bottom || exposure.Right) && !(exposure.Top || exposure.Left))
        {
            return palette.HullBase;
        }

        return interiorColor;
    }

    const MajorFeaturePlacement* ShipPainter::getMajorFeaturePlacementForPixel(const MajorFeatureData& features, uint32_t x, uint32_t y) const
    {
        for (const MajorFeaturePlacement& placement : features.Placements)
        {
            if (x >= placement.MinX && x <= placement.MaxX && y >= placement.MinY && y <= placement.MaxY)
            {
                return &placement;
            }
        }

        return nullptr;
    }

    const ShipAttachmentPlacement* ShipPainter::getAttachmentPlacementForPixel(const GeneratedShip& ship, uint32_t x, uint32_t y) const
    {
        const ShipAttachmentPlacement* bestPlacement = nullptr;
        uint32_t bestScore = std::numeric_limits<uint32_t>::max();

        for (const ShipAttachmentPlacement& placement : ship.AttachmentPlacements)
        {
            if (x < placement.MinimumX || x > placement.MaximumX || y < placement.MinimumY || y > placement.MaximumY)
            {
                continue;
            }

            const uint32_t outwardDistance = getAttachmentOutwardDistance(placement, x, y);

            if (outwardDistance == 0u)
            {
                continue;
            }

            const int32_t tangentCoordinate = getAttachmentTangentCoordinate(placement, x, y);
            const uint32_t absoluteTangentCoordinate = static_cast<uint32_t>(tangentCoordinate < 0 ? -tangentCoordinate : tangentCoordinate);
            const uint32_t score = outwardDistance + absoluteTangentCoordinate;

            if (score >= bestScore)
            {
                continue;
            }

            bestScore = score;
            bestPlacement = &placement;
        }

        return bestPlacement;
    }

    uint32_t ShipPainter::getAttachmentOutwardDistance(const ShipAttachmentPlacement& placement, uint32_t x, uint32_t y) const
    {
        switch (placement.Direction)
        {
        case ShipAttachmentDirection::LEFT: return placement.AnchorX >= x ? placement.AnchorX - x : 0u;
        case ShipAttachmentDirection::RIGHT: return x >= placement.AnchorX ? x - placement.AnchorX : 0u;
        case ShipAttachmentDirection::UP: return placement.AnchorY >= y ? placement.AnchorY - y : 0u;
        case ShipAttachmentDirection::DOWN: return y >= placement.AnchorY ? y - placement.AnchorY : 0u;
        default: return 0u;
        }
    }

    uint32_t ShipPainter::getAttachmentMaximumOutwardDistance(const ShipAttachmentPlacement& placement) const
    {
        switch (placement.Direction)
        {
        case ShipAttachmentDirection::LEFT: return placement.AnchorX - placement.MinimumX;
        case ShipAttachmentDirection::RIGHT: return placement.MaximumX - placement.AnchorX;
        case ShipAttachmentDirection::UP: return placement.AnchorY - placement.MinimumY;
        case ShipAttachmentDirection::DOWN: return placement.MaximumY - placement.AnchorY;
        default: return 0u;
        }
    }

    int32_t ShipPainter::getAttachmentTangentCoordinate(const ShipAttachmentPlacement& placement, uint32_t x, uint32_t y) const
    {
        if (placement.Direction == ShipAttachmentDirection::LEFT || placement.Direction == ShipAttachmentDirection::RIGHT)
        {
            return static_cast<int32_t>(y) - static_cast<int32_t>(placement.AnchorY);
        }

        return static_cast<int32_t>(x) - static_cast<int32_t>(placement.AnchorX);
    }

    bool ShipPainter::isSecondaryHullTonePixel(const GeneratedShip& ship, uint32_t x, uint32_t y, const ShipGenerationProfile& profile) const
    {
        if (profile.SecondaryHullToneCoveragePercent == 0u)
        {
            return false;
        }

        const uint32_t width = ship.HullMask.getWidth();
        const uint32_t height = ship.HullMask.getHeight();
        const uint32_t cellWidth = GenerationMath::scalePixelsFrom64(4u, width);
        const uint32_t cellHeight = GenerationMath::scalePixelsFrom64(3u, height);
        const uint32_t mirroredX = std::min(x, width - 1u - x);
        const uint32_t cellX = mirroredX / cellWidth;
        const uint32_t cellY = y / cellHeight;
        const uint64_t hash = getDeterministicPaintHash(ship.DomainSeeds.get(GenerationDomain::PALETTE), cellX, cellY, 0x48E5A31D77C4B991ull);

        return hash % 100u < profile.SecondaryHullToneCoveragePercent;
    }

    uint64_t ShipPainter::getDeterministicPaintHash(uint64_t seed, uint32_t x, uint32_t y, uint64_t salt) const
    {
        uint64_t value = seed;
        value ^= static_cast<uint64_t>(x) * 0x9E3779B185EBCA87ull;
        value ^= static_cast<uint64_t>(y) * 0xC2B2AE3D27D4EB4Full;
        value ^= salt;
        value ^= value >> 30u;
        value *= 0xBF58476D1CE4E5B9ull;
        value ^= value >> 27u;
        value *= 0x94D049BB133111EBull;
        value ^= value >> 31u;
        return value;
    }

    uint32_t ShipPainter::getColorLuma(const Color& color) const
    {
        return (static_cast<uint32_t>(color.R) * 54u + static_cast<uint32_t>(color.G) * 183u + static_cast<uint32_t>(color.B) * 19u) >> 8u;
    }


    bool ShipPainter::isDepthSupportHullPixel(const ShipGenerationContext& context, uint32_t x, uint32_t y) const
    {
        const GeneratedShip& ship = context.Ship;
        if (!ship.HullMask.get(x, y) || context.StructuralNegativeSpace.ReservedMask.get(x, y))
        {
            return false;
        }

        // Do not replace another semantic component or Task-61 detail motif with
        // a generic depth cue.  Livery/material color is intentionally allowed
        // and is darkened through its own palette family below.
        return !ship.CockpitMask.get(x, y)
            && !ship.EngineMask.get(x, y)
            && !ship.EngineExhaustMask.get(x, y)
            && !ship.AttachmentMask.get(x, y)
            && !context.Weapons.OccupiedMask.get(x, y)
            && !context.CoreTreatment.RaisedMask.get(x, y)
            && !context.CoreTreatment.RecessedMask.get(x, y)
            && !context.HullLayers.OccupiedMask.get(x, y)
            && !context.MajorFeatures.OccupiedMask.get(x, y)
            && !ship.AccentMask.get(x, y)
            && !ship.MechanicalDetailMask.get(x, y)
            && !ship.LightMask.get(x, y);
    }

    Color ShipPainter::getDepthShadowColor(const Color& currentColor, const ShipPalette& palette, bool strongShadow) const
    {
        if (currentColor == palette.HullAccentHighlight || currentColor == palette.HullAccent || currentColor == palette.HullAccentDark)
        {
            return palette.HullAccentDark;
        }

        if (currentColor == palette.MechanicalBase || currentColor == palette.MechanicalDark)
        {
            return palette.MechanicalDark;
        }

        if (currentColor == palette.EngineHighlight || currentColor == palette.EngineBase || currentColor == palette.EngineDark)
        {
            return palette.EngineDark;
        }

        if (currentColor == palette.Outline || currentColor.A == 0u)
        {
            return currentColor;
        }

        if (currentColor == palette.HullEdgeHighlight || currentColor == palette.HullHighlight || currentColor == palette.HullBase || currentColor == palette.HullSecondary || currentColor == palette.HullShadow || currentColor == palette.HullDeepShadow)
        {
            return strongShadow ? palette.HullDeepShadow : palette.HullShadow;
        }

        return currentColor;
    }

    Color ShipPainter::getHullPixelColor(const ShipGenerationContext& context, const GeneratedShip& ship, uint32_t x, uint32_t y, const ShipPalette& palette, const ShipGenerationProfile& profile) const
    {
        const uint32_t shadingComplexity = context.ScaleTraits.ShadingComplexity;
        const Color directionalColor = getDirectionalStructureColor(ship, x, y, palette.HullDeepShadow, palette.HullShadow, palette.HullBase, palette.HullHighlight, palette.HullEdgeHighlight);

        if (directionalColor != palette.HullBase)
        {
            return directionalColor;
        }

        if (context.WingRegions.hasWings())
        {
            if (isWingInnerBoundaryPixel(context, x, y))
            {
                return shadingComplexity >= 40u ? palette.HullDeepShadow : palette.HullShadow;
            }

            if (isFuselageWingBoundaryPixel(context, x, y) && shadingComplexity >= 20u)
            {
                return shadingComplexity >= 60u ? palette.HullEdgeHighlight : palette.HullHighlight;
            }

            if (context.WingRegions.WingMask.get(x, y) && shadingComplexity >= 20u)
            {
                const uint32_t maximumDepth = shadingComplexity >= 80u ? 2u : 1u;
                const uint32_t depth = PixelMaskUtils::getMaskDepth(context.WingRegions.WingMask, x, y, maximumDepth);

                if (depth >= maximumDepth || context.WingRegions.OuterWingMask.get(x, y))
                {
                    return palette.HullSecondary;
                }
            }
        }

        if (shadingComplexity >= 40u)
        {
            const uint32_t ridgeBandWidth = shadingComplexity >= 90u ? std::min(3u, GenerationMath::scalePixelsFrom64(2u, ship.HullMask.getWidth())) : 1u;
            const uint32_t ridgeMaximumDepth = shadingComplexity >= 80u ? 2u : 1u;

            if (isCentralRidgePixel(context, x, y, ridgeBandWidth) && PixelMaskUtils::getMaskDepth(ship.HullMask, x, y, ridgeMaximumDepth) >= ridgeMaximumDepth)
            {
                if (context.Profile.AxialRidgeUsesEdgeHighlight || context.FactionProfile.Finish.ForceAxialRidgeEdgeHighlight)
                {
                    return shadingComplexity >= 60u ? palette.HullEdgeHighlight : palette.HullHighlight;
                }

                return palette.HullHighlight;
            }
        }

        const uint32_t maximumDepth = shadingComplexity >= 80u ? 2u : 1u;
        const uint32_t depth = PixelMaskUtils::getMaskDepth(ship.HullMask, x, y, maximumDepth);

        if (shadingComplexity >= 60u && y < context.WingRegions.FuselageHalfWidths.size())
        {
            const uint32_t fuselageHalfWidth = context.WingRegions.FuselageHalfWidths[y];

            if (fuselageHalfWidth >= 4u && !context.WingRegions.WingMask.get(x, y) && depth >= maximumDepth)
            {
                const uint32_t leftCenter = (ship.HullMask.getWidth() - 1u) / 2u;
                const uint32_t rightCenter = ship.HullMask.getWidth() / 2u;
                const uint32_t distanceFromCenter = x <= leftCenter ? leftCenter - x : x - rightCenter;

                if (distanceFromCenter * 100u >= fuselageHalfWidth * 72u)
                {
                    return palette.HullSecondary;
                }
            }
        }

        if (depth >= maximumDepth && isSecondaryHullTonePixel(ship, x, y, profile))
        {
            return palette.HullSecondary;
        }

        return palette.HullBase;
    }

    Color ShipPainter::getAttachmentPixelColor(const GeneratedShip& ship, const ShipAttachmentPlacement& placement, uint32_t x, uint32_t y, const ShipPalette& palette, uint32_t shadingComplexity) const
    {
        const uint32_t outwardDistance = getAttachmentOutwardDistance(placement, x, y);
        const uint32_t maximumOutwardDistance = std::max(1u, getAttachmentMaximumOutwardDistance(placement));
        const int32_t tangentCoordinate = getAttachmentTangentCoordinate(placement, x, y);
        const uint32_t absoluteTangentCoordinate = static_cast<uint32_t>(tangentCoordinate < 0 ? -tangentCoordinate : tangentCoordinate);


        if (placement.Type == ShipAttachmentType::WEAPON_MOUNT)
        {
            const uint32_t mountingLength = std::max(1u, maximumOutwardDistance / 2u);

            if (outwardDistance == 1u)
            {
                return palette.HullShadow;
            }

            if (outwardDistance <= mountingLength)
            {
                return getDirectionalStructureColor(ship, x, y, palette.HullDeepShadow, palette.HullShadow, palette.HullSecondary, palette.HullHighlight, palette.HullEdgeHighlight);
            }

            return getDirectionalStructureColor(ship, x, y, palette.EngineDark, palette.EngineDark, palette.EngineBase, palette.EngineHighlight, palette.EngineHighlight);
        }

        if (placement.Type == ShipAttachmentType::SENSOR_ARRAY)
        {
            const uint64_t variation = getDeterministicPaintHash(ship.DomainSeeds.get(GenerationDomain::ATTACHMENTS), placement.AnchorX, placement.AnchorY, 0x86157A932BF018C1ull);
            const bool luminousTip = variation % 100u < 65u;
            const bool centerPixel = tangentCoordinate == 0;
            const bool tipPixel = outwardDistance >= maximumOutwardDistance;

            if (outwardDistance == 1u)
            {
                return palette.MechanicalDark;
            }

            if (tipPixel && centerPixel)
            {
                return luminousTip ? palette.LightHighlight : palette.HullAccentHighlight;
            }

            if (luminousTip && centerPixel && maximumOutwardDistance >= 4u && outwardDistance + 1u == maximumOutwardDistance)
            {
                return palette.LightBase;
            }

            return getDirectionalStructureColor(ship, x, y, palette.MechanicalDark, palette.MechanicalDark, palette.MechanicalBase, palette.EngineHighlight, palette.EngineHighlight);
        }

        if (placement.Type == ShipAttachmentType::AUXILIARY_POD)
        {
            if (outwardDistance == 1u)
            {
                return palette.MechanicalDark;
            }

            const uint64_t variation = getDeterministicPaintHash(ship.DomainSeeds.get(GenerationDomain::ATTACHMENTS), placement.AnchorX, placement.AnchorY, 0xF15CB38A4970D263ull);
            const bool hasIdentificationBand = variation % 100u < 50u;
            const uint32_t bandDistance = std::max(2u, (maximumOutwardDistance * 2u) / 3u);
            const uint32_t bandThickness = shadingComplexity >= 80u ? 2u : 1u;

            if (hasIdentificationBand && outwardDistance >= bandDistance && outwardDistance < bandDistance + bandThickness)
            {
                return getDirectionalStructureColor(ship, x, y, palette.HullAccentDark, palette.HullAccentDark, palette.HullAccent, palette.HullAccentHighlight, palette.HullAccentHighlight);
            }

            return getDirectionalStructureColor(ship, x, y, palette.HullDeepShadow, palette.HullShadow, palette.HullSecondary, palette.HullHighlight, palette.HullEdgeHighlight);
        }

        if (placement.Type == ShipAttachmentType::RADIATOR)
        {
            if (outwardDistance == 1u)
            {
                return palette.MechanicalDark;
            }

            const uint32_t stripeWidth = shadingComplexity >= 80u ? 2u : 1u;
            const uint32_t stripeIndex = absoluteTangentCoordinate / stripeWidth;

            if (stripeIndex % 2u != 0u)
            {
                return palette.MechanicalDark;
            }

            return getDirectionalStructureColor(ship, x, y, palette.MechanicalDark, palette.MechanicalDark, palette.MechanicalBase, palette.EngineHighlight, palette.EngineHighlight);
        }

        if (placement.Type == ShipAttachmentType::ARMOR_FIN)
        {
            if (outwardDistance == 1u)
            {
                return palette.HullShadow;
            }

            return getDirectionalStructureColor(ship, x, y, palette.HullDeepShadow, palette.HullShadow, palette.HullBase, palette.HullHighlight, palette.HullEdgeHighlight);
        }

        if (placement.Type == ShipAttachmentType::TECHNOLOGY_NODE)
        {
            if (outwardDistance == 1u)
            {
                return palette.HullAccentDark;
            }

            const uint64_t variation = getDeterministicPaintHash(ship.DomainSeeds.get(GenerationDomain::ATTACHMENTS), placement.AnchorX, placement.AnchorY, 0x35CBAFA782DF6194ull);
            const bool extendedCore = variation % 100u < 60u;
            const bool centerPixel = tangentCoordinate == 0;

            if (centerPixel && outwardDistance == maximumOutwardDistance)
            {
                return palette.LightHighlight;
            }

            if (extendedCore && centerPixel && maximumOutwardDistance >= 4u && outwardDistance + 1u == maximumOutwardDistance)
            {
                return palette.LightBase;
            }

            return getDirectionalStructureColor(ship, x, y, palette.HullAccentDark, palette.HullAccentDark, palette.HullAccent, palette.HullAccentHighlight, palette.HullAccentHighlight);
        }

        return palette.HullSecondary;
    }

    Color ShipPainter::getLegacyCockpitPixelColor(const PixelMask& cockpitMask, const PixelMaskUtils::MaskBounds& bounds, uint32_t x, uint32_t y, const ShipPalette& palette, uint32_t shadingComplexity) const
    {
        const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(cockpitMask, x, y);
        const uint32_t cockpitWidth = bounds.MaxX - bounds.MinX + 1u;
        const uint32_t cockpitHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t relativeX = x - bounds.MinX;
        const uint32_t relativeY = y - bounds.MinY;
        const bool reflectionRegion = relativeX * 100u <= cockpitWidth * 58u && relativeY * 100u <= cockpitHeight * 45u;
        const uint32_t maximumDepth = shadingComplexity >= 80u ? 2u : 1u;
        const uint32_t depth = PixelMaskUtils::getMaskDepth(cockpitMask, x, y, maximumDepth);

        if (exposure.isBoundary())
        {
            if (reflectionRegion && (exposure.Top || exposure.Left) && exposure.getHighlightExposure() >= exposure.getShadowExposure())
            {
                return palette.CockpitGlint;
            }
            return palette.CockpitDark;
        }
        if (reflectionRegion && depth >= maximumDepth)
        {
            return palette.CockpitHighlight;
        }
        return palette.CockpitBase;
    }

    Color ShipPainter::getCockpitGlassPixelColor(const CockpitData& cockpit, const PixelMaskUtils::MaskBounds& bounds, uint32_t x, uint32_t y, const ShipPalette& palette, uint32_t shadingComplexity) const
    {
        const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(cockpit.GlassMask, x, y);
        const uint32_t cockpitWidth = bounds.MaxX - bounds.MinX + 1u;
        const uint32_t cockpitHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t relativeX = x - bounds.MinX;
        const uint32_t relativeY = y - bounds.MinY;
        const uint32_t maximumDepth = shadingComplexity >= 75u ? 3u : (shadingComplexity >= 35u ? 2u : 1u);
        const uint32_t depth = PixelMaskUtils::getMaskDepth(cockpit.GlassMask, x, y, maximumDepth);

        // Hard reflection bands keep large glass surfaces structured without gradients.
        const bool upperReflectionBand = relativeY * 100u <= cockpitHeight * 30u;
        const bool leftReflectionZone = relativeX * 100u <= cockpitWidth * 55u;
        const bool narrowGlintBand = relativeY * 100u >= cockpitHeight * 18u && relativeY * 100u <= cockpitHeight * 28u;

        if (exposure.isBoundary())
        {
            if ((exposure.Top || exposure.Left) && leftReflectionZone && shadingComplexity >= 20u)
            {
                return palette.CockpitGlint;
            }
            return palette.CockpitDark;
        }

        if (cockpit.UpperSectionMask.get(x, y))
        {
            return upperReflectionBand || depth >= 2u ? palette.CockpitHighlight : palette.CockpitBase;
        }

        if (narrowGlintBand && leftReflectionZone && shadingComplexity >= 45u)
        {
            return palette.CockpitHighlight;
        }

        if (upperReflectionBand && depth >= std::min(2u, maximumDepth))
        {
            return palette.CockpitHighlight;
        }

        return depth == 0u ? palette.CockpitDark : palette.CockpitBase;
    }

    Color ShipPainter::getEnginePixelColor(const GeneratedShip& ship, uint32_t x, uint32_t y, const ShipPalette& palette, uint32_t shadingComplexity, const ShipFactionFinishProfile& finishProfile) const
    {
        const PixelMask& engineMask = ship.EngineMask;
        const int32_t pixelX = static_cast<int32_t>(x);
        const int32_t pixelY = static_cast<int32_t>(y);
        const bool embeddedRoot = ship.HullMask.get(x, y);
        const bool exhaustBelow = PixelMaskUtils::isMaskPixel(ship.EngineExhaustMask, pixelX, pixelY + 1);
        const PixelMaskUtils::DirectionalMaskExposure exposure = PixelMaskUtils::getDirectionalMaskExposure(engineMask, x, y);

        if (exhaustBelow)
        {
            const HorizontalMaskSpan span = getHorizontalMaskSpan(engineMask, x, y);
            const uint32_t distanceFromLeft = x - span.StartX;
            const uint32_t distanceFromRight = span.EndX - x;
            const bool centerPixel = distanceFromLeft == distanceFromRight || (span.Width % 2u == 0u && (distanceFromLeft + 1u == distanceFromRight || distanceFromRight + 1u == distanceFromLeft));
            return centerPixel ? resolveFactionPaintColor(finishProfile.EngineHotCoreRole, palette.EngineHotCore, palette) : palette.EngineDark;
        }

        if (embeddedRoot)
        {
            if (exposure.Top || exposure.Left)
            {
                return palette.MechanicalDark;
            }

            if (shadingComplexity >= 40u && (exposure.Bottom || exposure.Right))
            {
                return palette.EngineBase;
            }

            return palette.EngineDark;
        }

        if (exposure.isBoundary())
        {
            return getDirectionalMaskColor(engineMask, x, y, palette.EngineDark, palette.EngineDark, palette.EngineBase, palette.EngineHighlight, palette.EngineHighlight);
        }

        const HorizontalMaskSpan span = getHorizontalMaskSpan(engineMask, x, y);
        const uint32_t distanceFromLeft = x - span.StartX;
        const uint32_t distanceFromRight = span.EndX - x;
        const bool centerPixel = distanceFromLeft == distanceFromRight || (span.Width % 2u == 0u && (distanceFromLeft + 1u == distanceFromRight || distanceFromRight + 1u == distanceFromLeft));
        const uint32_t maximumDepth = shadingComplexity >= 80u ? 2u : 1u;
        const uint32_t depth = PixelMaskUtils::getMaskDepth(engineMask, x, y, maximumDepth);

        if (centerPixel && span.Width >= 5u && depth >= maximumDepth)
        {
            return resolveFactionPaintColor(finishProfile.EngineInteriorHighlightRole, palette.EngineHighlight, palette);
        }

        if (shadingComplexity >= 60u && depth == 1u && !centerPixel)
        {
            return palette.EngineBase;
        }

        return palette.EngineBase;
    }

    Color ShipPainter::getMechanicalPixelColor(const PixelMask& mechanicalMask, uint32_t x, uint32_t y, const ShipPalette& palette) const
    {
        const int32_t pixelX = static_cast<int32_t>(x);
        const int32_t pixelY = static_cast<int32_t>(y);

        const bool leftInside = PixelMaskUtils::isMaskPixel(mechanicalMask, pixelX - 1, pixelY);
        const bool rightInside = PixelMaskUtils::isMaskPixel(mechanicalMask, pixelX + 1, pixelY);
        const bool topInside = PixelMaskUtils::isMaskPixel(mechanicalMask, pixelX, pixelY - 1);
        const bool bottomInside = PixelMaskUtils::isMaskPixel(mechanicalMask, pixelX, pixelY + 1);

        if (leftInside && rightInside && topInside && bottomInside)
        {
            return palette.MechanicalBase;
        }

        return palette.MechanicalDark;
    }

    Color ShipPainter::getAccentPixelColor(const PixelMask& accentMask, uint32_t x, uint32_t y, const ShipPalette& palette) const
    {
        const int32_t pixelX = static_cast<int32_t>(x);
        const int32_t pixelY = static_cast<int32_t>(y);
        uint32_t highlightExposure = 0u;
        uint32_t shadowExposure = 0u;

        if (!PixelMaskUtils::isMaskPixel(accentMask, pixelX - 1, pixelY))
        {
            ++highlightExposure;
        }

        if (!PixelMaskUtils::isMaskPixel(accentMask, pixelX, pixelY - 1))
        {
            ++highlightExposure;
        }

        if (!PixelMaskUtils::isMaskPixel(accentMask, pixelX + 1, pixelY))
        {
            ++shadowExposure;
        }

        if (!PixelMaskUtils::isMaskPixel(accentMask, pixelX, pixelY + 1))
        {
            ++shadowExposure;
        }

        if (highlightExposure > shadowExposure)
        {
            return palette.HullAccentHighlight;
        }

        if (highlightExposure < shadowExposure)
        {
            return palette.HullAccentDark;
        }

        return palette.HullAccent;
    }

    Color ShipPainter::getLightPixelColor(const PixelMask& lightMask, uint32_t x, uint32_t y, const ShipPalette& palette) const
    {
        const int32_t pixelX = static_cast<int32_t>(x);
        const int32_t pixelY = static_cast<int32_t>(y);
        uint32_t highlightExposure = 0u;
        uint32_t shadowExposure = 0u;

        if (!PixelMaskUtils::isMaskPixel(lightMask, pixelX - 1, pixelY))
        {
            ++highlightExposure;
        }

        if (!PixelMaskUtils::isMaskPixel(lightMask, pixelX, pixelY - 1))
        {
            ++highlightExposure;
        }

        if (!PixelMaskUtils::isMaskPixel(lightMask, pixelX + 1, pixelY))
        {
            ++shadowExposure;
        }

        if (!PixelMaskUtils::isMaskPixel(lightMask, pixelX, pixelY + 1))
        {
            ++shadowExposure;
        }

        if (highlightExposure >= shadowExposure)
        {
            return palette.LightHighlight;
        }

        return palette.LightBase;
    }
}
