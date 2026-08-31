#include "LiveryGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
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
        constexpr uint64_t LiverySalt = 0x7C3A4F2D9B61E805ull;
        constexpr uint32_t PrimaryLiveryComplexityCost = 3u;
        constexpr uint32_t SecondaryLiveryComplexityCost = 2u;

        uint32_t scalePercent(uint32_t value, uint32_t percent)
        {
            return static_cast<uint32_t>((static_cast<uint64_t>(value) * percent + 50u) / 100u);
        }

        uint32_t randomUInt(std::mt19937_64& randomGenerator, uint32_t minimum, uint32_t maximum)
        {
            if (maximum <= minimum) { return minimum; }
            return std::uniform_int_distribution<uint32_t>(minimum, maximum)(randomGenerator);
        }

        uint32_t getScaleChancePercent(const GenerationScaleTraits& traits)
        {
            switch (traits.Tier)
            {
            case GenerationScaleTier::TINY: return 42u;
            case GenerationScaleTier::SMALL: return 72u;
            case GenerationScaleTier::MEDIUM: return 100u;
            case GenerationScaleTier::LARGE: return 112u;
            default: return 100u;
            }
        }

        uint32_t getScaleMaximumMarkings(const GenerationScaleTraits& traits)
        {
            switch (traits.Tier)
            {
            case GenerationScaleTier::TINY: return 1u;
            case GenerationScaleTier::SMALL: return 1u;
            case GenerationScaleTier::MEDIUM: return 2u;
            case GenerationScaleTier::LARGE: return 2u;
            default: return 1u;
            }
        }

        uint32_t getMinimumMarkingPixels(const GenerationScaleTraits& traits)
        {
            switch (traits.Tier)
            {
            case GenerationScaleTier::TINY: return 2u;
            case GenerationScaleTier::SMALL: return 4u;
            case GenerationScaleTier::MEDIUM: return 6u;
            case GenerationScaleTier::LARGE: return 9u;
            default: return 2u;
            }
        }

        uint32_t getAnchorWeightPercent(const VisualHierarchyPlan& hierarchy, ShipLiveryType type)
        {
            if (!hierarchy.InfluenceEnabled) { return 100u; }
            switch (hierarchy.PrimaryAnchor)
            {
            case ShipVisualAnchorType::WINGS:
                return type == ShipLiveryType::WING_BAND || type == ShipLiveryType::SHOULDER_BLOCK ? 175u : 75u;
            case ShipVisualAnchorType::COCKPIT:
                return type == ShipLiveryType::NOSE_BAND || type == ShipLiveryType::CHEVRON || type == ShipLiveryType::CENTER_STRIPE ? 150u : 82u;
            case ShipVisualAnchorType::WEAPONS:
                return type == ShipLiveryType::SHOULDER_BLOCK || type == ShipLiveryType::ID_PANEL ? 145u : 85u;
            case ShipVisualAnchorType::HULL_LAYERS:
                return type == ShipLiveryType::SHOULDER_BLOCK || type == ShipLiveryType::WING_BAND ? 145u : 88u;
            case ShipVisualAnchorType::CENTRAL_CORE:
                return type == ShipLiveryType::CENTER_STRIPE || type == ShipLiveryType::DOUBLE_CENTER_STRIPE || type == ShipLiveryType::CHEVRON ? 140u : 82u;
            case ShipVisualAnchorType::SILHOUETTE:
                return type == ShipLiveryType::CENTER_STRIPE || type == ShipLiveryType::NOSE_BAND ? 112u : 72u;
            case ShipVisualAnchorType::ENGINES:
                return type == ShipLiveryType::CENTER_STRIPE || type == ShipLiveryType::ID_PANEL ? 108u : 82u;
            default: return 100u;
            }
        }

    }

    void LiveryGenerator::generate(ShipGenerationContext& context) const
    {
        LiveryData& data = context.Livery;
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        data.reset(width, height);

        if (context.Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS)
        {
            context.updateLiveryDebugInfo();
            return;
        }

        uint32_t chance = scalePercent(context.Profile.LiveryChance, getScaleChancePercent(context.ScaleTraits));
        chance = scalePercent(chance, context.FactionProfile.Livery.ChancePercent);
        if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.PrimaryAnchor == ShipVisualAnchorType::SILHOUETTE)
        {
            chance = scalePercent(chance, 72u);
        }
        chance = std::min(100u, chance);

        std::mt19937_64 randomGenerator(mixGenerationSeed64(context.DomainSeeds.get(GenerationDomain::DETAILS) ^ LiverySalt));
        if (randomUInt(randomGenerator, 0u, 99u) >= chance)
        {
            context.updateLiveryDebugInfo();
            return;
        }

        const uint32_t maximumMarkings = std::min(context.Profile.MaximumLiveryMarkings, getScaleMaximumMarkings(context.ScaleTraits));
        if (maximumMarkings == 0u || !context.ComplexityBudget.canAfford(GenerationComplexityCategory::DETAIL, PrimaryLiveryComplexityCost))
        {
            context.updateLiveryDebugInfo();
            return;
        }

        data.TargetMarkingCount = 1u;
        if (maximumMarkings >= 2u && randomUInt(randomGenerator, 0u, 99u) < context.Profile.SupportingLiveryChance)
        {
            data.TargetMarkingCount = 2u;
        }

        std::array<bool, LiveryTypeCount> usedTypes = {};
        for (uint32_t index = 0u; index < data.TargetMarkingCount; ++index)
        {
            const bool secondary = index > 0u;
            const uint32_t cost = secondary ? SecondaryLiveryComplexityCost : PrimaryLiveryComplexityCost;
            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::DETAIL, cost)) { break; }

            bool placed = false;
            for (uint32_t fallback = 0u; fallback < static_cast<uint32_t>(LiveryTypeCount); ++fallback)
            {
                const ShipLiveryType type = selectType(context, randomGenerator, usedTypes, secondary);
                if (type == ShipLiveryType::SHIP_LIVERY_TYPE_END) { break; }
                usedTypes[static_cast<std::size_t>(type)] = true;

                bool asymmetric = false;
                PixelMask mask = buildMarkingMask(context, type, randomGenerator, asymmetric);
                sanitizeMask(context, mask, data);
                if (!asymmetric)
                {
                    for (uint32_t y = 0u; y < mask.getHeight(); ++y)
                    {
                        for (uint32_t x = 0u; x < mask.getWidth() / 2u; ++x)
                        {
                            const uint32_t mirroredX = mask.getWidth() - 1u - x;
                            if (mask.get(x, y) != mask.get(mirroredX, y))
                            {
                                mask.set(x, y, false);
                                mask.set(mirroredX, y, false);
                            }
                        }
                    }
                }
                const uint32_t pixelCount = PixelMaskUtils::getMaskPixelCount(mask);
                if (pixelCount < getMinimumMarkingPixels(context.ScaleTraits)) { continue; }
                if (secondary && !data.PrimaryMarkingMask.empty())
                {
                    const uint32_t primaryPixels = PixelMaskUtils::getMaskPixelCount(data.PrimaryMarkingMask);
                    if (primaryPixels > 0u && pixelCount * 100u > primaryPixels * getMaximumSupportingPercent(context)) { continue; }
                }

                bool materialPreservationFailure = false;
                if (!validateCoverage(context, mask, materialPreservationFailure))
                {
                    if (materialPreservationFailure) { ++data.MaterialPreservationRejectionCount; }
                    else { ++data.CoverageRejectionCount; }
                    continue;
                }

                if (addMarking(context, type, std::move(mask), secondary, asymmetric))
                {
                    context.ComplexityBudget.tryConsume(GenerationComplexityCategory::DETAIL, cost);
                    placed = true;
                    break;
                }
            }

            if (!placed) { break; }
        }

        context.updateLiveryDebugInfo();
    }

    uint32_t LiveryGenerator::getTypeWeight(const ShipGenerationContext& context, ShipLiveryType type, bool secondary) const
    {
        uint32_t weight = context.Profile.LiveryWeights.getWeight(type);
        weight = scalePercent(weight, context.FactionProfile.Livery.WeightMultipliersPercent.getWeightPercent(type));
        weight = scalePercent(weight, getAnchorWeightPercent(context.VisualHierarchy, type));

        if (context.ScaleTraits.Tier == GenerationScaleTier::TINY)
        {
            if (type == ShipLiveryType::DOUBLE_CENTER_STRIPE || type == ShipLiveryType::CHEVRON || type == ShipLiveryType::GEOMETRIC_INSIGNIA) { return 0u; }
        }
        else if (context.ScaleTraits.Tier == GenerationScaleTier::SMALL && type == ShipLiveryType::GEOMETRIC_INSIGNIA)
        {
            weight = scalePercent(weight, 45u);
        }

        if (type == ShipLiveryType::WING_BAND && !context.WingRegions.hasWings()) { return 0u; }
        if (secondary)
        {
            switch (type)
            {
            case ShipLiveryType::ID_PANEL: weight = scalePercent(weight, 140u); break;
            case ShipLiveryType::NOSE_BAND:
            case ShipLiveryType::GEOMETRIC_INSIGNIA: weight = scalePercent(weight, 110u); break;
            case ShipLiveryType::WING_BAND:
            case ShipLiveryType::DOUBLE_CENTER_STRIPE: weight = scalePercent(weight, 45u); break;
            default: weight = scalePercent(weight, 75u); break;
            }
        }
        return weight;
    }

    ShipLiveryType LiveryGenerator::selectType(const ShipGenerationContext& context, std::mt19937_64& randomGenerator, const std::array<bool, LiveryTypeCount>& usedTypes, bool secondary) const
    {
        uint64_t totalWeight = 0u;
        for (std::size_t index = 0u; index < LiveryTypeCount; ++index)
        {
            if (usedTypes[index]) { continue; }
            totalWeight += getTypeWeight(context, static_cast<ShipLiveryType>(index), secondary);
        }
        if (totalWeight == 0u) { return ShipLiveryType::SHIP_LIVERY_TYPE_END; }

        uint64_t roll = std::uniform_int_distribution<uint64_t>(0u, totalWeight - 1u)(randomGenerator);
        for (std::size_t index = 0u; index < LiveryTypeCount; ++index)
        {
            if (usedTypes[index]) { continue; }
            const uint32_t weight = getTypeWeight(context, static_cast<ShipLiveryType>(index), secondary);
            if (roll < weight) { return static_cast<ShipLiveryType>(index); }
            roll -= weight;
        }
        return ShipLiveryType::SHIP_LIVERY_TYPE_END;
    }

    PixelMask LiveryGenerator::buildMarkingMask(const ShipGenerationContext& context, ShipLiveryType type, std::mt19937_64& randomGenerator, bool& asymmetric) const
    {
        switch (type)
        {
        case ShipLiveryType::CENTER_STRIPE: return buildCenterStripe(context, randomGenerator, false);
        case ShipLiveryType::DOUBLE_CENTER_STRIPE: return buildCenterStripe(context, randomGenerator, true);
        case ShipLiveryType::WING_BAND: return buildWingBand(context, randomGenerator);
        case ShipLiveryType::SHOULDER_BLOCK: return buildShoulderBlock(context, randomGenerator);
        case ShipLiveryType::NOSE_BAND: return buildNoseBand(context, randomGenerator);
        case ShipLiveryType::CHEVRON: return buildChevron(context, randomGenerator);
        case ShipLiveryType::ID_PANEL: return buildIdPanel(context, randomGenerator, asymmetric);
        case ShipLiveryType::GEOMETRIC_INSIGNIA: return buildGeometricInsignia(context, randomGenerator, asymmetric);
        default: return PixelMask(context.Ship.HullMask.getWidth(), context.Ship.HullMask.getHeight(), false);
        }
    }

    PixelMask LiveryGenerator::buildCenterStripe(const ShipGenerationContext& context, std::mt19937_64& randomGenerator, bool doubled) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        PixelMask mask(width, height, false);
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return mask; }
        const uint32_t occupiedHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t startY = bounds.MinY + occupiedHeight * randomUInt(randomGenerator, 8u, 18u) / 100u;
        const uint32_t endY = bounds.MinY + occupiedHeight * randomUInt(randomGenerator, 78u, 92u) / 100u;
        const uint32_t stripeWidth = context.ScaleTraits.Tier >= GenerationScaleTier::LARGE ? 2u : 1u;
        const int32_t center2 = static_cast<int32_t>(width) - 1;
        const uint32_t gap = doubled ? (context.ScaleTraits.Tier >= GenerationScaleTier::MEDIUM ? 2u : 1u) : 0u;

        for (uint32_t y = startY; y <= std::min(endY, bounds.MaxY); ++y)
        {
            for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
            {
                if (!context.Ship.HullMask.get(x, y)) { continue; }
                const uint32_t distance2 = static_cast<uint32_t>(std::abs(static_cast<int32_t>(x) * 2 - center2));
                bool selected = false;
                if (!doubled)
                {
                    selected = distance2 <= stripeWidth;
                }
                else
                {
                    const uint32_t target2 = gap * 2u + stripeWidth + 1u;
                    selected = distance2 >= target2 - stripeWidth && distance2 <= target2 + stripeWidth;
                }
                if (selected) { mask.set(x, y); }
            }
        }
        return mask;
    }

    PixelMask LiveryGenerator::buildWingBand(const ShipGenerationContext& context, std::mt19937_64& randomGenerator) const
    {
        PixelMask mask(context.Ship.HullMask.getWidth(), context.Ship.HullMask.getHeight(), false);
        if (!context.WingRegions.hasWings()) { return mask; }
        const uint32_t start = context.WingRegions.StartY;
        const uint32_t end = std::max(start, context.WingRegions.EndY);
        const uint32_t centerY = randomUInt(randomGenerator, start, end);
        const uint32_t halfHeight = context.ScaleTraits.Tier >= GenerationScaleTier::LARGE ? 1u : 0u;
        for (uint32_t y = centerY > halfHeight ? centerY - halfHeight : 0u; y <= std::min(mask.getHeight() - 1u, centerY + halfHeight); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (context.WingRegions.WingMask.get(x, y)) { mask.set(x, y); }
            }
        }
        return mask;
    }

    PixelMask LiveryGenerator::buildShoulderBlock(const ShipGenerationContext& context, std::mt19937_64& randomGenerator) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        PixelMask mask(width, height, false);
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return mask; }
        const uint32_t occupiedHeight = bounds.MaxY - bounds.MinY + 1u;
        uint32_t startY = bounds.MinY + occupiedHeight * randomUInt(randomGenerator, 32u, 44u) / 100u;
        uint32_t endY = std::min(bounds.MaxY, startY + std::max(1u, GenerationMath::scalePixelsFrom64(4u, height)));
        if (context.WingRegions.hasWings())
        {
            startY = std::min(startY, context.WingRegions.StartY);
            endY = std::max(endY, std::min(context.WingRegions.EndY, context.WingRegions.PeakY + 1u));
        }
        const int32_t center2 = static_cast<int32_t>(width) - 1;
        for (uint32_t y = startY; y <= endY; ++y)
        {
            for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
            {
                if (!context.Ship.HullMask.get(x, y)) { continue; }
                const uint32_t distance2 = static_cast<uint32_t>(std::abs(static_cast<int32_t>(x) * 2 - center2));
                const uint32_t rowWidth = PixelMaskUtils::getOccupiedRowWidth(context.Ship.HullMask, y);
                // A shoulder marking is an outer-surface graphic, not a replacement
                // for most of a broad row. Keeping the inner 60% clear preserves the
                // material/hull read while leaving a substantial paired shoulder shape.
                if (rowWidth > 0u && distance2 * 100u >= rowWidth * 62u) { mask.set(x, y); }
            }
        }
        return mask;
    }

    PixelMask LiveryGenerator::buildNoseBand(const ShipGenerationContext& context, std::mt19937_64& randomGenerator) const
    {
        PixelMask mask(context.Ship.HullMask.getWidth(), context.Ship.HullMask.getHeight(), false);
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return mask; }
        const uint32_t occupiedHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t centerY = bounds.MinY + occupiedHeight * randomUInt(randomGenerator, 18u, 34u) / 100u;
        const uint32_t bandHeight = context.ScaleTraits.Tier >= GenerationScaleTier::LARGE ? 2u : 1u;
        for (uint32_t y = centerY; y <= std::min(bounds.MaxY, centerY + bandHeight - 1u); ++y)
            for (uint32_t x = bounds.MinX; x <= bounds.MaxX; ++x)
                if (context.Ship.HullMask.get(x, y) && !context.WingRegions.WingMask.get(x, y)) { mask.set(x, y); }
        return mask;
    }

    PixelMask LiveryGenerator::buildChevron(const ShipGenerationContext& context, std::mt19937_64& randomGenerator) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        PixelMask mask(width, context.Ship.HullMask.getHeight(), false);
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return mask; }
        const uint32_t occupiedHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t startY = bounds.MinY + occupiedHeight * randomUInt(randomGenerator, 28u, 48u) / 100u;
        const uint32_t armLength = std::max(2u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.Tier >= GenerationScaleTier::LARGE ? 7u : 5u, std::min(mask.getWidth(), mask.getHeight())));
        const int32_t center2 = static_cast<int32_t>(width) - 1;
        for (uint32_t step = 0u; step < armLength && startY + step < mask.getHeight(); ++step)
        {
            const int32_t offset2 = static_cast<int32_t>(step * 2u + 1u);
            const int32_t left2 = center2 - offset2;
            const int32_t right2 = center2 + offset2;
            const int32_t candidates[2] = { left2 / 2, (right2 + 1) / 2 };
            for (const int32_t x : candidates)
            {
                if (x < 0 || x >= static_cast<int32_t>(width)) { continue; }
                const uint32_t ux = static_cast<uint32_t>(x);
                if (context.Ship.HullMask.get(ux, startY + step)) { mask.set(ux, startY + step); }
            }
        }
        return mask;
    }

    PixelMask LiveryGenerator::buildIdPanel(const ShipGenerationContext& context, std::mt19937_64& randomGenerator, bool& asymmetric) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        PixelMask mask(width, height, false);
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return mask; }
        asymmetric = allowAsymmetricMarking(context, randomGenerator);
        const bool left = randomUInt(randomGenerator, 0u, 1u) == 0u;
        const uint32_t panelWidth = std::max(2u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.Tier >= GenerationScaleTier::LARGE ? 7u : 4u, width));
        const uint32_t panelHeight = std::max(1u, GenerationMath::scalePixelsFrom64(context.ScaleTraits.Tier >= GenerationScaleTier::MEDIUM ? 3u : 2u, height));
        const uint32_t occupiedHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t y0 = bounds.MinY + occupiedHeight * randomUInt(randomGenerator, 42u, 68u) / 100u;
        const uint32_t center = width / 2u;
        const uint32_t sideOffset = std::max(1u, (bounds.MaxX - bounds.MinX + 1u) / 5u);
        const int32_t centerX = static_cast<int32_t>(center) + (left ? -static_cast<int32_t>(sideOffset) : static_cast<int32_t>(sideOffset));

        const auto addPanel = [&](int32_t cx)
        {
            const int32_t x0 = cx - static_cast<int32_t>(panelWidth / 2u);
            for (uint32_t py = 0u; py < panelHeight && y0 + py < height; ++py)
                for (uint32_t px = 0u; px < panelWidth; ++px)
                {
                    const int32_t x = x0 + static_cast<int32_t>(px);
                    if (x >= 0 && x < static_cast<int32_t>(width) && context.Ship.HullMask.get(static_cast<uint32_t>(x), y0 + py)) { mask.set(static_cast<uint32_t>(x), y0 + py); }
                }
        };
        addPanel(centerX);
        if (!asymmetric) { addPanel(static_cast<int32_t>(width - 1u) - centerX); }
        return mask;
    }

    PixelMask LiveryGenerator::buildGeometricInsignia(const ShipGenerationContext& context, std::mt19937_64& randomGenerator, bool& asymmetric) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        PixelMask mask(width, height, false);
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        if (!bounds.Valid) { return mask; }
        asymmetric = allowAsymmetricMarking(context, randomGenerator) && context.FactionProfile.Livery.AllowAsymmetricGeometricInsignia;
        const uint32_t radius = context.ScaleTraits.Tier >= GenerationScaleTier::LARGE ? 3u : (context.ScaleTraits.Tier >= GenerationScaleTier::MEDIUM ? 2u : 1u);
        const uint32_t occupiedHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t cy = bounds.MinY + occupiedHeight * randomUInt(randomGenerator, 38u, 62u) / 100u;
        const int32_t center = static_cast<int32_t>((width - 1u) / 2u);
        int32_t cx = center;
        if (asymmetric)
        {
            const int32_t offset = static_cast<int32_t>(std::max(2u, (bounds.MaxX - bounds.MinX + 1u) / 5u));
            cx += randomUInt(randomGenerator, 0u, 1u) == 0u ? -offset : offset;
        }

        for (int32_t oy = -static_cast<int32_t>(radius); oy <= static_cast<int32_t>(radius); ++oy)
        {
            const int32_t span = static_cast<int32_t>(radius) - std::abs(oy);
            for (int32_t ox = -span; ox <= span; ++ox)
            {
                if (std::abs(ox) != span && std::abs(oy) != static_cast<int32_t>(radius)) { continue; }
                const int32_t x = cx + ox;
                const int32_t y = static_cast<int32_t>(cy) + oy;
                if (x >= 0 && y >= 0 && x < static_cast<int32_t>(width) && y < static_cast<int32_t>(height) && context.Ship.HullMask.get(static_cast<uint32_t>(x), static_cast<uint32_t>(y)))
                {
                    mask.set(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
                }
            }
        }
        return mask;
    }

    void LiveryGenerator::sanitizeMask(const ShipGenerationContext& context, PixelMask& mask, const LiveryData& data) const
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
                    || context.CoreTreatment.RecessedMask.get(x, y)
                    || context.CoreTreatment.LuminousMask.get(x, y)
                    || (context.VisualHierarchy.targets(ShipVisualAnchorType::CENTRAL_CORE) && context.CoreTreatment.CoreRegionMask.get(x, y))
                    || context.MajorFeatures.EmissiveMask.get(x, y)
                    || (context.VisualHierarchy.targets(ShipVisualAnchorType::MAJOR_FEATURE) && context.MajorFeatures.OccupiedMask.get(x, y))
                    || context.StructuralNegativeSpace.ReservedMask.get(x, y)
                    || data.PrimaryMarkingMask.get(x, y)
                    || data.SecondaryMarkingMask.get(x, y);
                if (unavailable) { mask.set(x, y, false); }
            }
        }
    }

    uint32_t LiveryGenerator::getCoverageLimitPercent(const ShipGenerationContext& context, bool connected) const
    {
        uint32_t limit = connected ? context.Profile.MaximumLiveryConnectedCoveragePercent : context.Profile.MaximumLiveryCoveragePercent;
        switch (context.ScaleTraits.Tier)
        {
        case GenerationScaleTier::TINY: limit += connected ? 6u : 12u; break;
        case GenerationScaleTier::SMALL: limit += connected ? 3u : 6u; break;
        case GenerationScaleTier::MEDIUM: limit += connected ? 1u : 2u; break;
        default: break;
        }

        return std::max(1u, std::min(100u, limit));
    }

    uint32_t LiveryGenerator::getMaximumSupportingPercent(const ShipGenerationContext& context) const
    {
        switch (context.ScaleTraits.Tier)
        {
        case GenerationScaleTier::TINY: return 85u;
        case GenerationScaleTier::SMALL: return 75u;
        case GenerationScaleTier::MEDIUM: return 65u;
        case GenerationScaleTier::LARGE: return 60u;
        default: return 60u;
        }
    }

    bool LiveryGenerator::validateCoverage(const ShipGenerationContext& context, const PixelMask& mask, bool& materialPreservationFailure) const
    {
        materialPreservationFailure = false;
        const uint32_t hullPixels = PixelMaskUtils::getMaskPixelCount(context.Ship.HullMask);
        if (hullPixels == 0u) { return false; }

        PixelMask combined = context.Livery.PrimaryMarkingMask;
        PixelMaskUtils::mergeMask(combined, context.Livery.SecondaryMarkingMask);
        PixelMaskUtils::mergeMask(combined, mask);

        const uint32_t totalPixels = PixelMaskUtils::getMaskPixelCount(combined);
        const uint32_t connectedPixels = PixelMaskUtils::getLargestConnectedMaskPixelCount(combined);
        const uint32_t totalLimit = getCoverageLimitPercent(context, false);
        const uint32_t connectedLimit = getCoverageLimitPercent(context, true);
        if (static_cast<uint64_t>(totalPixels) * 100u > static_cast<uint64_t>(hullPixels) * totalLimit ||
            static_cast<uint64_t>(connectedPixels) * 100u > static_cast<uint64_t>(hullPixels) * connectedLimit)
        {
            return false;
        }

        // Material regions are intentionally smaller than the whole hull, so allow a
        // larger local percentage than the global livery cap while still requiring a
        // majority of the Task-59 material read to survive. Broad surface markings use
        // the same explicit rule rather than a style/faction special case.
        const uint32_t materialLimit = std::min(65u, totalLimit * 3u);
        const uint32_t mechanicalLimit = std::min(75u, materialLimit + 15u);

        const uint32_t secondaryMaterialPixels = PixelMaskUtils::getMaskPixelCount(context.MaterialComposition.SecondaryHullMask);
        if (secondaryMaterialPixels > 0u)
        {
            const uint32_t overlap = PixelMaskUtils::getMaskOverlapPixelCount(combined, context.MaterialComposition.SecondaryHullMask);
            if (static_cast<uint64_t>(overlap) * 100u > static_cast<uint64_t>(secondaryMaterialPixels) * materialLimit)
            {
                materialPreservationFailure = true;
                return false;
            }
        }

        const uint32_t mechanicalMaterialPixels = PixelMaskUtils::getMaskPixelCount(context.MaterialComposition.MechanicalMask);
        if (mechanicalMaterialPixels > 0u)
        {
            const uint32_t overlap = PixelMaskUtils::getMaskOverlapPixelCount(combined, context.MaterialComposition.MechanicalMask);
            if (static_cast<uint64_t>(overlap) * 100u > static_cast<uint64_t>(mechanicalMaterialPixels) * mechanicalLimit)
            {
                materialPreservationFailure = true;
                return false;
            }
        }
        return true;
    }

    bool LiveryGenerator::addMarking(ShipGenerationContext& context, ShipLiveryType type, PixelMask mask, bool secondary, bool asymmetric) const
    {
        if (PixelMaskUtils::getMaskPixelCount(mask) == 0u) { return false; }
        LiveryPlacement placement;
        placement.Type = type;
        placement.Secondary = secondary;
        placement.Asymmetric = asymmetric;
        placement.Mask = mask;
        PixelMaskUtils::mergeMask(secondary ? context.Livery.SecondaryMarkingMask : context.Livery.PrimaryMarkingMask, mask);
        context.Livery.Placements.push_back(std::move(placement));
        ++context.Livery.TypeCounts[static_cast<std::size_t>(type)];
        return true;
    }

    bool LiveryGenerator::allowAsymmetricMarking(const ShipGenerationContext& context, std::mt19937_64& randomGenerator) const
    {
        uint32_t chance = context.Profile.LiveryAsymmetricChance;
        if (context.MacroAsymmetry.Fulfilled) { chance = std::min(100u, chance + 45u); }
        chance = static_cast<uint32_t>(std::clamp<int32_t>(static_cast<int32_t>(chance) + context.FactionProfile.Livery.AsymmetricChanceOffset, 0, 100));
        chance /= context.FactionProfile.Livery.AsymmetricChanceDivisor;
        return randomUInt(randomGenerator, 0u, 99u) < chance;
    }
}
