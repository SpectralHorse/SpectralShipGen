#include "MaterialCompositionGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "GenerationMath.h"
#include "PixelMaskUtils.h"
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipVisualAnchorType.h>

namespace SpectralShipGen
{
    namespace
    {
        constexpr uint64_t MaterialCompositionSalt = 0xD6E8FEB86659FD93ull;
        constexpr std::size_t MaterialZoneTypeCount = static_cast<std::size_t>(ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END);

        uint32_t scalePercent(uint32_t value, uint32_t percent)
        {
            return static_cast<uint32_t>((static_cast<uint64_t>(value) * percent + 50u) / 100u);
        }

        uint32_t getScaleChancePercent(const GenerationScaleTraits& traits)
        {
            switch (traits.Tier)
            {
            case GenerationScaleTier::TINY: return 62u;
            case GenerationScaleTier::SMALL: return 82u;
            case GenerationScaleTier::MEDIUM: return 100u;
            case GenerationScaleTier::LARGE: return 112u;
            default: return 100u;
            }
        }

        uint32_t getScaleMaximumZones(const GenerationScaleTraits& traits)
        {
            switch (traits.Tier)
            {
            case GenerationScaleTier::TINY: return 1u;
            case GenerationScaleTier::SMALL: return 2u;
            case GenerationScaleTier::MEDIUM: return 3u;
            case GenerationScaleTier::LARGE: return 4u;
            default: return 1u;
            }
        }

        uint32_t getAnchorWeightPercent(const VisualHierarchyPlan& hierarchy, ShipMaterialZoneType type)
        {
            if (!hierarchy.InfluenceEnabled) { return 100u; }
            const ShipVisualAnchorType anchor = hierarchy.PrimaryAnchor;
            if (anchor == ShipVisualAnchorType::COCKPIT && type == ShipMaterialZoneType::COCKPIT_COLLAR) { return 185u; }
            if (anchor == ShipVisualAnchorType::WINGS && (type == ShipMaterialZoneType::WING_SURFACE || type == ShipMaterialZoneType::SHOULDER_SURFACE)) { return 175u; }
            if (anchor == ShipVisualAnchorType::ENGINES && type == ShipMaterialZoneType::REAR_MECHANICAL) { return 190u; }
            if (anchor == ShipVisualAnchorType::WEAPONS && type == ShipMaterialZoneType::HARDPOINT_SURROUND) { return 190u; }
            if (anchor == ShipVisualAnchorType::CENTRAL_CORE && type == ShipMaterialZoneType::AXIAL_BAND) { return 175u; }
            if (anchor == ShipVisualAnchorType::HULL_LAYERS && (type == ShipMaterialZoneType::SHOULDER_SURFACE || type == ShipMaterialZoneType::WING_SURFACE)) { return 155u; }
            if (anchor == ShipVisualAnchorType::SILHOUETTE && type == ShipMaterialZoneType::AXIAL_BAND) { return 135u; }
            if (anchor == ShipVisualAnchorType::NEGATIVE_SPACE && (type == ShipMaterialZoneType::WING_SURFACE || type == ShipMaterialZoneType::REAR_MECHANICAL)) { return 140u; }
            return 78u;
        }

        uint32_t getMinimumZonePixels(const GenerationScaleTraits& traits)
        {
            switch (traits.Tier)
            {
            case GenerationScaleTier::TINY: return 4u;
            case GenerationScaleTier::SMALL: return 6u;
            case GenerationScaleTier::MEDIUM: return 10u;
            case GenerationScaleTier::LARGE: return 16u;
            default: return 4u;
            }
        }

        bool withinDistance(const PixelMask& mask, uint32_t x, uint32_t y, uint32_t distance)
        {
            const int32_t px = static_cast<int32_t>(x);
            const int32_t py = static_cast<int32_t>(y);
            for (int32_t oy = -static_cast<int32_t>(distance); oy <= static_cast<int32_t>(distance); ++oy)
            {
                for (int32_t ox = -static_cast<int32_t>(distance); ox <= static_cast<int32_t>(distance); ++ox)
                {
                    if (std::abs(ox) + std::abs(oy) > static_cast<int32_t>(distance)) { continue; }
                    if (PixelMaskUtils::isMaskPixel(mask, px + ox, py + oy)) { return true; }
                }
            }
            return false;
        }
    }

    void MaterialCompositionGenerator::generate(ShipGenerationContext& context) const
    {
        MaterialCompositionData& data = context.MaterialComposition;
        data.reset(context.Ship.HullMask.getWidth(), context.Ship.HullMask.getHeight());

        // Legacy recipes preserve their historical painting behavior. Current
        // recipes derive zone layout from the HULL domain, never PALETTE.
        if (context.Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS) { context.updateMaterialCompositionDebugInfo(); return; }

        const uint32_t chance = std::min(100u, scalePercent(context.Profile.MaterialCompositionChance, getScaleChancePercent(context.ScaleTraits)));
        std::mt19937_64 randomGenerator(mixGenerationSeed64(context.DomainSeeds.get(GenerationDomain::HULL) ^ MaterialCompositionSalt));
        if (getRandomUInt(randomGenerator, 0u, 99u) >= chance) { context.updateMaterialCompositionDebugInfo(); return; }

        const uint32_t maximumZones = std::min(context.Profile.MaximumMaterialZones, getScaleMaximumZones(context.ScaleTraits));
        if (maximumZones == 0u) { context.updateMaterialCompositionDebugInfo(); return; }

        data.TargetZoneCount = maximumZones == 1u ? 1u : getRandomUInt(randomGenerator, 1u, maximumZones);
        if (context.ScaleTraits.Tier >= GenerationScaleTier::MEDIUM && maximumZones >= 2u && data.TargetZoneCount == 1u) { data.TargetZoneCount = 2u; }
        data.ContrastStrengthPercent = context.Profile.MaterialSecondaryContrastPercent;

        std::array<bool, MaterialZoneTypeCount> usedTypes = {};
        for (uint32_t zoneIndex = 0u; zoneIndex < data.TargetZoneCount; ++zoneIndex)
        {
            bool placed = false;
            for (std::size_t fallback = 0u; fallback < MaterialZoneTypeCount; ++fallback)
            {
                const ShipMaterialZoneType type = selectZoneType(context, randomGenerator, usedTypes);
                if (type == ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END) { break; }
                usedTypes[static_cast<std::size_t>(type)] = true;
                if (addZone(context, type, buildZoneMask(context, type))) { placed = true; break; }
            }
            if (!placed) { break; }
        }

        context.updateMaterialCompositionDebugInfo();
    }

    uint32_t MaterialCompositionGenerator::getRandomUInt(std::mt19937_64& randomGenerator, uint32_t minimum, uint32_t maximum) const
    {
        if (minimum == maximum) { return minimum; }
        std::uniform_int_distribution<uint32_t> distribution(minimum, maximum);
        return distribution(randomGenerator);
    }

    uint32_t MaterialCompositionGenerator::getZoneWeight(const ShipGenerationContext& context, ShipMaterialZoneType type) const
    {
        uint32_t weight = context.Profile.MaterialZoneWeights.getWeight(type);
        weight = scalePercent(weight, context.FactionProfile.Materials.ZoneWeightMultipliersPercent.getWeightPercent(type));
        weight = scalePercent(weight, getAnchorWeightPercent(context.VisualHierarchy, type));
        return weight;
    }

    ShipMaterialZoneType MaterialCompositionGenerator::selectZoneType(const ShipGenerationContext& context, std::mt19937_64& randomGenerator, const std::array<bool, MaterialZoneTypeCount>& usedTypes) const
    {
        uint64_t totalWeight = 0u;
        for (std::size_t index = 0u; index < MaterialZoneTypeCount; ++index)
        {
            if (usedTypes[index]) { continue; }
            totalWeight += getZoneWeight(context, static_cast<ShipMaterialZoneType>(index));
        }
        if (totalWeight == 0u) { return ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END; }

        const uint64_t roll = std::uniform_int_distribution<uint64_t>(0u, totalWeight - 1u)(randomGenerator);
        uint64_t cursor = 0u;
        for (std::size_t index = 0u; index < MaterialZoneTypeCount; ++index)
        {
            if (usedTypes[index]) { continue; }
            cursor += getZoneWeight(context, static_cast<ShipMaterialZoneType>(index));
            if (roll < cursor) { return static_cast<ShipMaterialZoneType>(index); }
        }
        return ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END;
    }

    PixelMask MaterialCompositionGenerator::buildZoneMask(const ShipGenerationContext& context, ShipMaterialZoneType type) const
    {
        switch (type)
        {
        case ShipMaterialZoneType::WING_SURFACE: return buildWingSurfaceMask(context);
        case ShipMaterialZoneType::SHOULDER_SURFACE: return buildShoulderSurfaceMask(context);
        case ShipMaterialZoneType::AXIAL_BAND: return buildAxialBandMask(context);
        case ShipMaterialZoneType::REAR_MECHANICAL: return buildRearMechanicalMask(context);
        case ShipMaterialZoneType::COCKPIT_COLLAR: return buildCockpitCollarMask(context);
        case ShipMaterialZoneType::HARDPOINT_SURROUND: return buildHardpointSurroundMask(context);
        default: return PixelMask(context.Ship.HullMask.getWidth(), context.Ship.HullMask.getHeight(), false);
        }
    }

    PixelMask MaterialCompositionGenerator::buildWingSurfaceMask(const ShipGenerationContext& context) const
    {
        PixelMask mask(context.Ship.HullMask.getWidth(), context.Ship.HullMask.getHeight(), false);
        if (!context.WingRegions.hasWings()) { return mask; }
        const bool broadZone = context.Profile.MaterialWingSurfaceUsesFullWing;
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
                if (broadZone ? context.WingRegions.WingMask.get(x, y) : context.WingRegions.OuterWingMask.get(x, y)) { mask.set(x, y); }
        return mask;
    }

    PixelMask MaterialCompositionGenerator::buildShoulderSurfaceMask(const ShipGenerationContext& context) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        PixelMask mask(width, height, false);
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return mask; }
        uint32_t startY = bounds.MinY + ((bounds.MaxY - bounds.MinY + 1u) * 28u) / 100u;
        uint32_t endY = bounds.MinY + ((bounds.MaxY - bounds.MinY + 1u) * 62u) / 100u;
        if (context.WingRegions.hasWings()) { startY = std::min(startY, context.WingRegions.StartY); endY = std::max(endY, context.WingRegions.PeakY); }
        endY = std::min(endY, bounds.MaxY);
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t rightCenter = width / 2u;
        for (uint32_t y = startY; y <= endY && y < height; ++y)
        {
            const uint32_t halfWidth = y < context.WingRegions.FuselageHalfWidths.size() ? context.WingRegions.FuselageHalfWidths[y] : 0u;
            if (halfWidth < 2u) { continue; }
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (!context.Ship.HullMask.get(x, y) || context.WingRegions.WingMask.get(x, y)) { continue; }
                const uint32_t distance = x <= leftCenter ? leftCenter - x : x - rightCenter;
                if (distance * 100u >= halfWidth * 48u) { mask.set(x, y); }
            }
        }
        return mask;
    }

    PixelMask MaterialCompositionGenerator::buildAxialBandMask(const ShipGenerationContext& context) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        PixelMask mask(width, context.Ship.HullMask.getHeight(), false);
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return mask; }
        const uint32_t startY = bounds.MinY + ((bounds.MaxY - bounds.MinY + 1u) * 15u) / 100u;
        const uint32_t endY = bounds.MinY + ((bounds.MaxY - bounds.MinY + 1u) * 86u) / 100u;
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t rightCenter = width / 2u;
        for (uint32_t y = startY; y <= endY && y < mask.getHeight(); ++y)
        {
            const uint32_t fuselageHalfWidth = y < context.WingRegions.FuselageHalfWidths.size() ? context.WingRegions.FuselageHalfWidths[y] : 0u;
            if (fuselageHalfWidth == 0u) { continue; }
            const uint32_t bandHalfWidth = std::max(1u, (fuselageHalfWidth * context.Profile.MaterialAxialBandWidthPercent + 99u) / 100u);
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (!context.Ship.HullMask.get(x, y) || context.WingRegions.WingMask.get(x, y)) { continue; }
                const uint32_t distance = x <= leftCenter ? leftCenter - x : x - rightCenter;
                if (distance < bandHalfWidth) { mask.set(x, y); }
            }
        }
        return mask;
    }

    PixelMask MaterialCompositionGenerator::buildRearMechanicalMask(const ShipGenerationContext& context) const
    {
        PixelMask mask(context.Ship.HullMask.getWidth(), context.Ship.HullMask.getHeight(), false);
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return mask; }
        const uint32_t radius = std::max(1u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.Tier >= GenerationScaleTier::MEDIUM ? 3u : 2u, context.Ship.HullMask.getWidth()));
        const bool hasEnginePixels = PixelMaskUtils::getMaskPixelCount(context.Ship.EngineMask) != 0u;
        const uint32_t rearStart = bounds.MinY + ((bounds.MaxY - bounds.MinY + 1u) * 67u) / 100u;
        for (uint32_t y = rearStart; y <= bounds.MaxY; ++y)
        {
            for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
            {
                if (!context.Ship.HullMask.get(x, y)) { continue; }
                if (!hasEnginePixels || withinDistance(context.Ship.EngineMask, x, y, radius)) { mask.set(x, y); }
            }
        }
        return mask;
    }

    PixelMask MaterialCompositionGenerator::buildCockpitCollarMask(const ShipGenerationContext& context) const
    {
        PixelMask mask(context.Ship.HullMask.getWidth(), context.Ship.HullMask.getHeight(), false);
        if (PixelMaskUtils::getMaskPixelCount(context.Ship.CockpitMask) == 0u) { return mask; }
        const uint32_t radius = context.ScaleTraits.Tier >= GenerationScaleTier::MEDIUM ? 2u : 1u;
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
                if (context.Ship.HullMask.get(x, y) && !context.Ship.CockpitMask.get(x, y) && withinDistance(context.Ship.CockpitMask, x, y, radius)) { mask.set(x, y); }
        return mask;
    }

    PixelMask MaterialCompositionGenerator::buildHardpointSurroundMask(const ShipGenerationContext& context) const
    {
        PixelMask mask(context.Ship.HullMask.getWidth(), context.Ship.HullMask.getHeight(), false);
        if (context.Weapons.empty()) { return mask; }
        const uint32_t radius = context.ScaleTraits.Tier >= GenerationScaleTier::LARGE ? 2u : 1u;
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
                if (context.Ship.HullMask.get(x, y) && (withinDistance(context.Weapons.RootMask, x, y, radius) || withinDistance(context.Weapons.BodyMask, x, y, radius))) { mask.set(x, y); }
        return mask;
    }

    void MaterialCompositionGenerator::sanitizeZoneMask(const ShipGenerationContext& context, PixelMask& mask, const MaterialCompositionData& data) const
    {
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (!mask.get(x, y)) { continue; }
                const bool unavailable = !context.Ship.HullMask.get(x, y)
                    || context.Ship.CockpitMask.get(x, y)
                    || context.Ship.EngineMask.get(x, y)
                    || context.Ship.EngineExhaustMask.get(x, y)
                    || context.Ship.AttachmentMask.get(x, y)
                    || context.Weapons.OccupiedMask.get(x, y)
                    || context.CoreTreatment.RaisedMask.get(x, y)
                    || context.CoreTreatment.RecessedMask.get(x, y)
                    || context.CoreTreatment.SecondaryMaterialMask.get(x, y)
                    || context.CoreTreatment.LuminousMask.get(x, y)
                    || context.HullLayers.OccupiedMask.get(x, y)
                    || context.MajorFeatures.OccupiedMask.get(x, y)
                    || data.SecondaryHullMask.get(x, y)
                    || data.MechanicalMask.get(x, y);
                if (unavailable) { mask.set(x, y, false); }
            }
        }
    }

    bool MaterialCompositionGenerator::addZone(ShipGenerationContext& context, ShipMaterialZoneType type, PixelMask mask) const
    {
        MaterialCompositionData& data = context.MaterialComposition;
        sanitizeZoneMask(context, mask, data);
        if (PixelMaskUtils::getMaskPixelCount(mask) < getMinimumZonePixels(context.ScaleTraits)) { return false; }

        MaterialZonePlacement placement;
        placement.Type = type;
        placement.Mechanical = isMechanicalZone(type);
        placement.Mask = mask;
        data.Placements.push_back(placement);
        ++data.TypeCounts[static_cast<std::size_t>(type)];
        PixelMaskUtils::mergeMask(placement.Mechanical ? data.MechanicalMask : data.SecondaryHullMask, mask);
        return true;
    }

    bool MaterialCompositionGenerator::isMechanicalZone(ShipMaterialZoneType type) const
    {
        return type == ShipMaterialZoneType::REAR_MECHANICAL || type == ShipMaterialZoneType::HARDPOINT_SURROUND;
    }
}
