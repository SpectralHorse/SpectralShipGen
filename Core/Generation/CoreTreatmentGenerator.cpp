#include "CoreTreatmentGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "GenerationMath.h"
#include "PixelMaskUtils.h"

namespace SpectralShipGen
{
    namespace
    {
        constexpr uint32_t MaximumPlacementAttempts = 8u;

        uint32_t getCoreRegionWidthPercent(const ShipGenerationContext& context)
        {
            const uint32_t divisor = std::max(1u, context.Profile.CoreRegionWidthHorizontalCapacityDivisor);
            uint32_t value = context.Profile.CoreRegionWidthBasePercent + context.ScaleTraits.HorizontalCapacity / divisor;
            if (context.Profile.CoreRegionWidthMaximumPercent != 0u)
            {
                value = std::min(value, context.Profile.CoreRegionWidthMaximumPercent);
            }
            return value;
        }


    }

    const char* getShipCoreTreatmentTypeName(ShipCoreTreatmentType type)
    {
        switch (type)
        {
        case ShipCoreTreatmentType::CENTRAL_SPINE: return "CENTRAL_SPINE";
        case ShipCoreTreatmentType::COCKPIT_SURROUND: return "COCKPIT_SURROUND";
        case ShipCoreTreatmentType::RAISED_CORE_PLATE: return "RAISED_CORE_PLATE";
        case ShipCoreTreatmentType::LATERAL_RECESSES: return "LATERAL_RECESSES";
        case ShipCoreTreatmentType::LONGITUDINAL_ARMOR_BAND: return "LONGITUDINAL_ARMOR_BAND";
        case ShipCoreTreatmentType::CORE_CHANNEL: return "CORE_CHANNEL";
        default: return "UNKNOWN";
        }
    }

    void CoreTreatmentGenerator::generate(ShipGenerationContext& context) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        context.CoreTreatment.reset(width, height);
        deriveCoreRegion(context);
        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->CoreRegionPixelCount = PixelMaskUtils::getMaskPixelCount(context.CoreTreatment.CoreRegionMask);
            context.DebugInfo->CoreRegionMask = context.CoreTreatment.CoreRegionMask;
            context.DebugInfo->CoreRaisedMask = context.CoreTreatment.RaisedMask;
            context.DebugInfo->CoreRecessedMask = context.CoreTreatment.RecessedMask;
            context.DebugInfo->CoreSecondaryMaterialMask = context.CoreTreatment.SecondaryMaterialMask;
            context.DebugInfo->CoreLuminousMask = context.CoreTreatment.LuminousMask;
        }

        if (context.Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS || width == 0u || height == 0u)
        {
            return;
        }

        const uint32_t corePixels = PixelMaskUtils::getMaskPixelCount(context.CoreTreatment.CoreRegionMask);
        if (corePixels < 6u || !context.ComplexityBudget.canAfford(GenerationComplexityCategory::HULL_LAYER, 4u))
        {
            return;
        }

        const uint32_t scaleChance = 35u + context.ScaleTraits.ShadingComplexity * 65u / 100u;
        uint32_t chance = context.Profile.CoreTreatmentChance * scaleChance / 100u;
        chance = std::min(94u, chance * context.FactionProfile.CoreTreatment.ChancePercent / 100u);
        if (context.VisualHierarchy.InfluenceEnabled)
        {
            if (context.VisualHierarchy.targets(ShipVisualAnchorType::CENTRAL_CORE))
            {
                chance = chance * context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::CENTRAL_CORE) / 100u;
                if (context.VisualHierarchy.isPrimary(ShipVisualAnchorType::CENTRAL_CORE)) { chance = std::max(chance, 90u); }
            }
            else
            {
                chance = chance * context.VisualHierarchy.getCompetingFeaturePercent(ShipVisualAnchorType::CENTRAL_CORE) / 100u;
            }
            chance = std::min(98u, chance);
        }
        if (context.Cockpit.SizeClass == CockpitSizeClass::MASSIVE && !context.VisualHierarchy.isPrimary(ShipVisualAnchorType::CENTRAL_CORE)) { chance = chance * 82u / 100u; }
        if (context.getGenerationRandomUInt(GenerationDomain::HULL_LAYERS, 0u, 99u) >= chance) { return; }

        uint32_t targetCount = 1u;
        if (context.ScaleTraits.ShadingComplexity >= 55u && context.ScaleTraits.MinimumDimension >= 44u && context.getGenerationRandomUInt(GenerationDomain::HULL_LAYERS, 0u, 99u) < 42u) { targetCount = 2u; }
        if (context.ScaleTraits.ShadingComplexity >= 88u && context.ScaleTraits.MinimumDimension >= 96u && context.getGenerationRandomUInt(GenerationDomain::HULL_LAYERS, 0u, 99u) < 24u) { targetCount = 3u; }
        targetCount = std::min(targetCount, context.Profile.MaximumCoreTreatments);
        if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.isPrimary(ShipVisualAnchorType::CENTRAL_CORE) && context.ScaleTraits.Tier >= GenerationScaleTier::MEDIUM)
        {
            targetCount = std::min(3u, std::max(2u, targetCount));
        }

        std::array<bool, static_cast<std::size_t>(ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END)> used = {};
        for (uint32_t treatmentIndex = 0u; treatmentIndex < targetCount; ++treatmentIndex)
        {
            bool placed = false;
            for (uint32_t attempt = 0u; attempt < MaximumPlacementAttempts; ++attempt)
            {
                const ShipCoreTreatmentType type = selectTreatmentType(context, used);
                if (type == ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END) { break; }

                Candidate candidate(width, height);
                candidate.Type = type;
                if (!generateCandidate(context, type, candidate) || !validateCandidate(context, candidate))
                {
                    ++context.CoreTreatment.PlacementRejectionCount;
                    continue;
                }

                const uint32_t cost = getTreatmentCost(type);
                const uint32_t categoryAllocation = context.ComplexityBudget.getCategoryAllocation(GenerationComplexityCategory::HULL_LAYER);
                const uint32_t coreAllocationCap = std::max(6u, categoryAllocation * 65u / 100u);
                if (context.CoreTreatment.ComplexityCost + cost > coreAllocationCap)
                {
                    ++context.CoreTreatment.PlacementRejectionCount;
                    continue;
                }
                const bool dominant = isDominantTreatment(type);
                const PixelMask* loadMask = &candidate.Raised;
                if (PixelMaskUtils::getMaskPixelCount(*loadMask) == 0u) { loadMask = &candidate.Secondary; }
                if (PixelMaskUtils::getMaskPixelCount(*loadMask) == 0u) { loadMask = &candidate.Recessed; }
                const auto regions = context.SpatialBudget.collectRegions(*loadMask);
                const uint32_t acceptance = context.SpatialBudget.getPlacementAcceptancePercent(regions, cost, dominant);
                if (acceptance == 0u || context.getGenerationRandomUInt(GenerationDomain::HULL_LAYERS, 0u, 99u) >= acceptance)
                {
                    context.SpatialBudget.recordRejection(regions);
                    ++context.CoreTreatment.PlacementRejectionCount;
                    continue;
                }
                if (!context.ComplexityBudget.tryConsume(GenerationComplexityCategory::HULL_LAYER, cost)) { break; }

                context.SpatialBudget.consume(regions, cost, dominant);
                commitCandidate(context, candidate, cost, dominant);
                used[static_cast<std::size_t>(type)] = true;
                placed = true;
                break;
            }
            if (!placed) { break; }
        }

        rebuildBoundaryMask(context.CoreTreatment);
        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->CoreTreatmentCount = context.CoreTreatment.TreatmentCount;
            context.DebugInfo->CoreTreatmentComplexityCost = context.CoreTreatment.ComplexityCost;
            context.DebugInfo->CoreTreatmentPlacementRejectionCount = context.CoreTreatment.PlacementRejectionCount;
            context.DebugInfo->CoreRegionPixelCount = PixelMaskUtils::getMaskPixelCount(context.CoreTreatment.CoreRegionMask);
            context.DebugInfo->CoreRaisedPixelCount = PixelMaskUtils::getMaskPixelCount(context.CoreTreatment.RaisedMask);
            context.DebugInfo->CoreRecessedPixelCount = PixelMaskUtils::getMaskPixelCount(context.CoreTreatment.RecessedMask);
            context.DebugInfo->CoreSecondaryMaterialPixelCount = PixelMaskUtils::getMaskPixelCount(context.CoreTreatment.SecondaryMaterialMask);
            context.DebugInfo->CoreLuminousPixelCount = PixelMaskUtils::getMaskPixelCount(context.CoreTreatment.LuminousMask);
            context.DebugInfo->CoreTreatmentTypeCounts = context.CoreTreatment.TypeCounts;
            context.DebugInfo->CoreRegionMask = context.CoreTreatment.CoreRegionMask;
            context.DebugInfo->CoreRaisedMask = context.CoreTreatment.RaisedMask;
            context.DebugInfo->CoreRecessedMask = context.CoreTreatment.RecessedMask;
            context.DebugInfo->CoreSecondaryMaterialMask = context.CoreTreatment.SecondaryMaterialMask;
            context.DebugInfo->CoreLuminousMask = context.CoreTreatment.LuminousMask;
        }
    }

    void CoreTreatmentGenerator::deriveCoreRegion(ShipGenerationContext& context) const
    {
        CoreTreatmentData& data = context.CoreTreatment;
        const PixelMaskUtils::MaskBounds hullBounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);
        const PixelMaskUtils::MaskBounds cockpitBounds = PixelMaskUtils::calculateMaskBounds(context.Ship.CockpitMask);
        if (!hullBounds.Valid) { return; }

        const uint32_t hullHeight = hullBounds.MaxY - hullBounds.MinY + 1u;
        const uint32_t lead = std::max(1u, GenerationMath::scaleVerticalPixelsFrom64(2u, context.Settings.Dimensions));
        const uint32_t startY = cockpitBounds.Valid && cockpitBounds.MinY > lead ? std::max(hullBounds.MinY, cockpitBounds.MinY - lead) : hullBounds.MinY + hullHeight / 5u;
        const uint32_t cockpitRear = cockpitBounds.Valid ? cockpitBounds.MaxY : startY;
        const uint32_t desiredEnd = std::max(cockpitRear + std::max(2u, hullHeight / 3u), hullBounds.MinY + hullHeight * 3u / 5u);
        const uint32_t endY = std::min(hullBounds.MaxY, desiredEnd);
        const uint32_t imageWidth = context.Ship.HullMask.getWidth();
        const uint32_t leftCenter = (imageWidth - 1u) / 2u;
        const uint32_t rightCenter = imageWidth / 2u;
        const uint32_t widthPercent = getCoreRegionWidthPercent(context);

        for (uint32_t y = startY; y <= endY; ++y)
        {
            if (y >= context.WingRegions.FuselageHalfWidths.size()) { continue; }
            const uint32_t halfWidth = context.WingRegions.FuselageHalfWidths[y];
            if (halfWidth == 0u) { continue; }
            const uint32_t allowedHalfWidth = std::max(1u, halfWidth * widthPercent / 100u);
            for (uint32_t x = 0u; x < imageWidth; ++x)
            {
                if (!context.Ship.HullMask.get(x, y) || context.WingRegions.WingMask.get(x, y)) { continue; }
                const uint32_t distance = x <= leftCenter ? leftCenter - x : x - rightCenter;
                if (distance < allowedHalfWidth) { data.CoreRegionMask.set(x, y, true); }
            }
        }
    }

    ShipCoreTreatmentType CoreTreatmentGenerator::selectTreatmentType(ShipGenerationContext& context, const std::array<bool, static_cast<std::size_t>(ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END)>& used) const
    {
        uint64_t totalWeight = 0u;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END); ++index)
        {
            if (!used[index]) { totalWeight += getTreatmentWeight(context.Profile, context.FactionProfile.CoreTreatment, static_cast<ShipCoreTreatmentType>(index)); }
        }
        if (totalWeight == 0u) { return ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END; }
        uint64_t roll = context.getGenerationRandomUInt64(GenerationDomain::HULL_LAYERS, 0u, totalWeight - 1u);
        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END); ++index)
        {
            if (used[index]) { continue; }
            const uint32_t weight = getTreatmentWeight(context.Profile, context.FactionProfile.CoreTreatment, static_cast<ShipCoreTreatmentType>(index));
            if (roll < weight) { return static_cast<ShipCoreTreatmentType>(index); }
            roll -= weight;
        }
        return ShipCoreTreatmentType::SHIP_CORE_TREATMENT_TYPE_END;
    }

    bool CoreTreatmentGenerator::generateCandidate(ShipGenerationContext& context, ShipCoreTreatmentType type, Candidate& candidate) const
    {
        switch (type)
        {
        case ShipCoreTreatmentType::CENTRAL_SPINE: return generateCentralSpine(context, candidate);
        case ShipCoreTreatmentType::COCKPIT_SURROUND: return generateCockpitSurround(context, candidate);
        case ShipCoreTreatmentType::RAISED_CORE_PLATE: return generateRaisedCorePlate(context, candidate);
        case ShipCoreTreatmentType::LATERAL_RECESSES: return generateLateralRecesses(context, candidate);
        case ShipCoreTreatmentType::LONGITUDINAL_ARMOR_BAND: return generateLongitudinalArmorBand(context, candidate);
        case ShipCoreTreatmentType::CORE_CHANNEL: return generateCoreChannel(context, candidate);
        default: return false;
        }
    }

    bool CoreTreatmentGenerator::generateCentralSpine(ShipGenerationContext& context, Candidate& candidate) const
    {
        const PixelMaskUtils::MaskBounds bounds = PixelMaskUtils::calculateMaskBounds(context.CoreTreatment.CoreRegionMask);
        if (!bounds.Valid) { return false; }
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t band = context.ScaleTraits.HorizontalCapacity >= 70u ? 2u : 1u;
        for (uint32_t y = bounds.MinY; y <= bounds.MaxY; ++y)
        {
            for (uint32_t offset = 0u; offset < band; ++offset)
            {
                addSymmetricPixel(context, candidate.Raised, static_cast<int32_t>(leftCenter - std::min(leftCenter, offset)), static_cast<int32_t>(y));
            }
        }
        return true;
    }

    bool CoreTreatmentGenerator::generateCockpitSurround(ShipGenerationContext& context, Candidate& candidate) const
    {
        const auto cockpit = PixelMaskUtils::calculateMaskBounds(context.Ship.CockpitMask);
        if (!cockpit.Valid) { return false; }
        const uint32_t padX = std::max(1u, GenerationMath::scaleHorizontalPixelsFrom64(2u, context.Settings.Dimensions));
        const uint32_t padY = std::max(1u, GenerationMath::scaleVerticalPixelsFrom64(2u, context.Settings.Dimensions));
        const int32_t minX = static_cast<int32_t>(cockpit.MinX) - static_cast<int32_t>(padX);
        const int32_t maxX = static_cast<int32_t>(cockpit.MaxX) + static_cast<int32_t>(padX);
        const int32_t minY = static_cast<int32_t>(cockpit.MinY) - static_cast<int32_t>(padY);
        const int32_t maxY = static_cast<int32_t>(cockpit.MaxY) + static_cast<int32_t>(padY * 2u);
        for (int32_t y = minY; y <= maxY; ++y)
        {
            for (int32_t x = minX; x <= maxX; ++x)
            {
                const bool rim = x == minX || x == maxX || y == minY || y == maxY;
                if (rim) { addPixel(context, candidate.Secondary, x, y); }
            }
        }
        return true;
    }

    bool CoreTreatmentGenerator::generateRaisedCorePlate(ShipGenerationContext& context, Candidate& candidate) const
    {
        const auto bounds = PixelMaskUtils::calculateMaskBounds(context.CoreTreatment.CoreRegionMask);
        if (!bounds.Valid) { return false; }
        const uint32_t regionHeight = bounds.MaxY - bounds.MinY + 1u;
        const uint32_t startY = bounds.MinY + regionHeight / 5u;
        const uint32_t endY = bounds.MinY + regionHeight * 4u / 5u;
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t baseHalf = std::max(1u, GenerationMath::scaleHorizontalPixelsFrom64(4u + context.ScaleTraits.HorizontalCapacity / 30u, context.Settings.Dimensions));
        const uint32_t half = GenerationMath::scaleByPercent(baseHalf, context.Profile.RaisedCorePlateWidthPercent);
        for (uint32_t y = startY; y <= endY; ++y)
        {
            const uint32_t taper = std::min((y - startY) / std::max(1u, GenerationMath::scaleVerticalPixelsFrom64(5u, context.Settings.Dimensions)), half - 1u);
            for (uint32_t dx = 0u; dx < half - taper; ++dx)
            {
                addSymmetricPixel(context, candidate.Raised, static_cast<int32_t>(leftCenter - std::min(leftCenter, dx)), static_cast<int32_t>(y));
            }
        }
        return true;
    }

    bool CoreTreatmentGenerator::generateLateralRecesses(ShipGenerationContext& context, Candidate& candidate) const
    {
        const auto bounds = PixelMaskUtils::calculateMaskBounds(context.CoreTreatment.CoreRegionMask);
        if (!bounds.Valid) { return false; }
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t startY = bounds.MinY + (bounds.MaxY - bounds.MinY + 1u) / 4u;
        const uint32_t endY = bounds.MaxY;
        for (uint32_t y = startY; y <= endY; y += 2u)
        {
            if (y >= context.WingRegions.FuselageHalfWidths.size()) { continue; }
            const uint32_t half = context.WingRegions.FuselageHalfWidths[y];
            if (half < 3u) { continue; }
            const uint32_t offset = std::max(2u, half * 2u / 3u);
            addSymmetricPixel(context, candidate.Recessed, static_cast<int32_t>(leftCenter >= offset ? leftCenter - offset : 0u), static_cast<int32_t>(y));
            if (context.ScaleTraits.ShadingComplexity >= 65u) { addSymmetricPixel(context, candidate.Recessed, static_cast<int32_t>(leftCenter >= offset ? leftCenter - offset : 0u), static_cast<int32_t>(y + 1u)); }
        }
        return true;
    }

    bool CoreTreatmentGenerator::generateLongitudinalArmorBand(ShipGenerationContext& context, Candidate& candidate) const
    {
        const auto bounds = PixelMaskUtils::calculateMaskBounds(context.CoreTreatment.CoreRegionMask);
        if (!bounds.Valid) { return false; }
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t offset = std::max(1u, GenerationMath::scaleHorizontalPixelsFrom64(3u, context.Settings.Dimensions));
        for (uint32_t y = bounds.MinY; y <= bounds.MaxY; ++y)
        {
            addSymmetricPixel(context, candidate.Secondary, static_cast<int32_t>(leftCenter >= offset ? leftCenter - offset : 0u), static_cast<int32_t>(y));
            if (context.ScaleTraits.ShadingComplexity >= 72u) { addSymmetricPixel(context, candidate.Secondary, static_cast<int32_t>(leftCenter >= offset + 1u ? leftCenter - offset - 1u : 0u), static_cast<int32_t>(y)); }
        }
        return true;
    }

    bool CoreTreatmentGenerator::generateCoreChannel(ShipGenerationContext& context, Candidate& candidate) const
    {
        const auto bounds = PixelMaskUtils::calculateMaskBounds(context.CoreTreatment.CoreRegionMask);
        if (!bounds.Valid) { return false; }
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t startY = bounds.MinY + (bounds.MaxY - bounds.MinY + 1u) / 4u;
        for (uint32_t y = startY; y <= bounds.MaxY; ++y)
        {
            addSymmetricPixel(context, candidate.Recessed, static_cast<int32_t>(leftCenter), static_cast<int32_t>(y));
            const ShipFactionCoreChannelLuminousPattern luminousPattern = context.FactionProfile.CoreTreatment.CoreChannelLuminousPattern;
            const bool luminous = luminousPattern == ShipFactionCoreChannelLuminousPattern::EVERY_THIRD_ROW
                ? y % 3u == 0u
                : (luminousPattern == ShipFactionCoreChannelLuminousPattern::EXCEPT_EVERY_THIRD_ROW && y % 3u != 0u);
            if (luminous)
            {
                addSymmetricPixel(context, candidate.Luminous, static_cast<int32_t>(leftCenter), static_cast<int32_t>(y));
            }
        }
        return true;
    }

    bool CoreTreatmentGenerator::validateCandidate(const ShipGenerationContext& context, const Candidate& candidate) const
    {
        const uint32_t candidatePixelCount = PixelMaskUtils::getMaskPixelCount(candidate.Raised) + PixelMaskUtils::getMaskPixelCount(candidate.Recessed) + PixelMaskUtils::getMaskPixelCount(candidate.Secondary) + PixelMaskUtils::getMaskPixelCount(candidate.Luminous);
        if (candidatePixelCount < 2u) { return false; }
        const PixelMask* masks[] = { &candidate.Raised, &candidate.Recessed, &candidate.Secondary, &candidate.Luminous };
        for (const PixelMask* mask : masks)
        {
            for (uint32_t y = 0u; y < mask->getHeight(); ++y)
            {
                for (uint32_t x = 0u; x < mask->getWidth(); ++x)
                {
                    if (!mask->get(x, y)) { continue; }
                    if (!context.CoreTreatment.CoreRegionMask.get(x, y) || !context.Ship.HullMask.get(x, y) || context.Ship.CockpitMask.get(x, y) || context.Ship.EngineMask.get(x, y)) { return false; }
                }
            }
        }
        return true;
    }

    void CoreTreatmentGenerator::commitCandidate(ShipGenerationContext& context, const Candidate& candidate, uint32_t cost, bool) const
    {
        PixelMaskUtils::mergeMask(context.CoreTreatment.RaisedMask, candidate.Raised);
        PixelMaskUtils::mergeMask(context.CoreTreatment.RecessedMask, candidate.Recessed);
        PixelMaskUtils::mergeMask(context.CoreTreatment.SecondaryMaterialMask, candidate.Secondary);
        PixelMaskUtils::mergeMask(context.CoreTreatment.LuminousMask, candidate.Luminous);
        ++context.CoreTreatment.TreatmentCount;
        context.CoreTreatment.ComplexityCost += cost;
        ++context.CoreTreatment.TypeCounts[static_cast<std::size_t>(candidate.Type)];
    }

    bool CoreTreatmentGenerator::addPixel(const ShipGenerationContext& context, PixelMask& mask, int32_t x, int32_t y) const
    {
        if (x < 0 || y < 0 || x >= static_cast<int32_t>(mask.getWidth()) || y >= static_cast<int32_t>(mask.getHeight())) { return false; }
        const uint32_t px = static_cast<uint32_t>(x);
        const uint32_t py = static_cast<uint32_t>(y);
        if (!context.CoreTreatment.CoreRegionMask.get(px, py) || context.Ship.CockpitMask.get(px, py) || context.Ship.EngineMask.get(px, py)) { return false; }
        mask.set(px, py, true);
        return true;
    }

    bool CoreTreatmentGenerator::addSymmetricPixel(const ShipGenerationContext& context, PixelMask& mask, int32_t x, int32_t y) const
    {
        if (!addPixel(context, mask, x, y)) { return false; }
        const int32_t mirrorX = static_cast<int32_t>(mask.getWidth()) - 1 - x;
        return addPixel(context, mask, mirrorX, y);
    }

    uint32_t CoreTreatmentGenerator::getTreatmentCost(ShipCoreTreatmentType type) const
    {
        switch (type)
        {
        case ShipCoreTreatmentType::CENTRAL_SPINE: return 6u;
        case ShipCoreTreatmentType::COCKPIT_SURROUND: return 5u;
        case ShipCoreTreatmentType::RAISED_CORE_PLATE: return 9u;
        case ShipCoreTreatmentType::LATERAL_RECESSES: return 6u;
        case ShipCoreTreatmentType::LONGITUDINAL_ARMOR_BAND: return 7u;
        case ShipCoreTreatmentType::CORE_CHANNEL: return 6u;
        default: return 6u;
        }
    }

    bool CoreTreatmentGenerator::isDominantTreatment(ShipCoreTreatmentType type) const
    {
        return type == ShipCoreTreatmentType::RAISED_CORE_PLATE || type == ShipCoreTreatmentType::CENTRAL_SPINE;
    }

    uint32_t CoreTreatmentGenerator::getTreatmentWeight(const ShipGenerationProfile& profile, const ShipFactionCoreTreatmentProfile& factionProfile, ShipCoreTreatmentType type) const
    {
        const std::array<uint32_t, 6u> weights = {
            profile.CoreTreatmentWeights.CentralSpine,
            profile.CoreTreatmentWeights.CockpitSurround,
            profile.CoreTreatmentWeights.RaisedCorePlate,
            profile.CoreTreatmentWeights.LateralRecesses,
            profile.CoreTreatmentWeights.LongitudinalArmorBand,
            profile.CoreTreatmentWeights.CoreChannel
        };
        const int64_t adjusted = static_cast<int64_t>(weights[static_cast<std::size_t>(type)])
            + static_cast<int64_t>(factionProfile.WeightOffsets.getOffset(type));
        return static_cast<uint32_t>(std::max<int64_t>(1, adjusted));
    }

    void CoreTreatmentGenerator::rebuildBoundaryMask(CoreTreatmentData& data) const
    {
        PixelMask combined = data.RaisedMask;
        PixelMaskUtils::mergeMask(combined, data.RecessedMask);
        PixelMaskUtils::mergeMask(combined, data.SecondaryMaterialMask);
        PixelMaskUtils::mergeMask(combined, data.LuminousMask);
        for (uint32_t y = 0u; y < combined.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < combined.getWidth(); ++x)
            {
                if (combined.get(x, y) && PixelMaskUtils::getDirectionalMaskExposure(combined, x, y).isBoundary()) { data.BoundaryMask.set(x, y, true); }
            }
        }
    }
}
