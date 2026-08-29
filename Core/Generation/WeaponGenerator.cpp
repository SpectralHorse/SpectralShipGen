#include "WeaponGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <utility>
#include <vector>

#include "GenerationMath.h"
#include "MacroAsymmetryPlanner.h"
#include "PixelMaskUtils.h"

namespace PixelShipGenerator
{
    namespace
    {
        uint32_t getWeaponComplexityCost(ShipWeaponType type)
        {
            switch (type)
            {
            case ShipWeaponType::SINGLE_CANNON: return 14u;
            case ShipWeaponType::TWIN_CANNON: return 18u;
            case ShipWeaponType::COMPACT_TURRET: return 15u;
            case ShipWeaponType::RAIL_WEAPON: return 20u;
            case ShipWeaponType::WEAPON_POD: return 18u;
            default: return 0u;
            }
        }


        GenerationSpatialRegion getWeaponSpatialRegion(ShipWeaponHardpointRegion region, uint32_t x, uint32_t width)
        {
            const bool left = x < width / 2u;
            switch (region)
            {
            case ShipWeaponHardpointRegion::CENTRAL_NOSE: return GenerationSpatialRegion::NOSE;
            case ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE:
            case ShipWeaponHardpointRegion::FORWARD_SHOULDER: return GenerationSpatialRegion::FRONT_FUSELAGE;
            case ShipWeaponHardpointRegion::WING_ROOT: return left ? GenerationSpatialRegion::LEFT_WING_ROOT : GenerationSpatialRegion::RIGHT_WING_ROOT;
            case ShipWeaponHardpointRegion::OUTER_WING: return left ? GenerationSpatialRegion::LEFT_OUTER_WING : GenerationSpatialRegion::RIGHT_OUTER_WING;
            case ShipWeaponHardpointRegion::CENTRAL_BODY: return GenerationSpatialRegion::MID_FUSELAGE;
            default: return GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
            }
        }
        uint32_t getPairedWeaponComplexityCost(ShipWeaponType type)
        {
            const uint32_t singleCost = getWeaponComplexityCost(type);
            return singleCost + (singleCost + 1u) / 2u;
        }

        bool usesWeightedHardpointSelection(const ShipGenerationProfile& profile)
        {
            const ShipWeaponHardpointWeights& weights = profile.LargeWeaponHardpointWeights;
            return weights.CentralNose != 100u || weights.ForwardFuselageSide != 100u || weights.WingRoot != 100u || weights.OuterWing != 100u || weights.ForwardShoulder != 100u || weights.CentralBody != 100u;
        }
    }
    void WeaponGenerator::generate(ShipGenerationContext& context) const
    {
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        context.Weapons.reset(width, height);
        context.Ship.IdleAnimationMetadata.WeaponOccupiedMask.clear(false);
        context.Ship.IdleAnimationMetadata.WeaponMovableMask.clear(false);
        context.Ship.IdleAnimationMetadata.WeaponMuzzleMask.clear(false);
        context.Ship.IdleAnimationMetadata.WeaponEmissiveMask.clear(false);
        context.Ship.IdleAnimationMetadata.WeaponComponents.clear();

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->WeaponHardpointCount = 0u;
            context.DebugInfo->WeaponPlacementAttemptCount = 0u;
            context.DebugInfo->WeaponPlacementRejectionCount = 0u;
            context.DebugInfo->WeaponCount = 0u;
            context.DebugInfo->WeaponPixelCount = 0u;
            context.DebugInfo->WeaponMovablePixelCount = 0u;
            context.DebugInfo->WeaponOccupiedMask = PixelMask(width, height, false);
            context.DebugInfo->WeaponTypeCounts.fill(0u);
            context.DebugInfo->WeaponUnits.clear();
        }

        if (width == 0u || height == 0u)
        {
            return;
        }

        if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::LARGE_WEAPON, 14u))
        {
            return;
        }

        const FactionWeaponProfile factionProfile = getFactionWeaponProfile(context.Settings.Faction);
        const uint32_t generationChance = getGenerationChance(context, factionProfile);
        const bool macroRequested = context.MacroAsymmetry.targets(MacroAsymmetryCategory::LARGE_WEAPON);

        if (context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedLargeWeaponPresence.has_value())
        {
            const bool normallyGenerated = generationChance > 0u && context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < generationChance;
            (void)normallyGenerated;
            if (!*context.CalibrationSettings->Overrides.ForcedLargeWeaponPresence) { return; }
        }
        else if (!macroRequested && (generationChance == 0u || context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) >= generationChance))
        {
            return;
        }

        const std::vector<WeaponHardpoint> hardpoints = discoverHardpoints(context);

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->WeaponHardpointCount = static_cast<uint32_t>(hardpoints.size());
        }

        if (hardpoints.empty())
        {
            if (macroRequested) { MacroAsymmetryPlanner::reject(context); }
            return;
        }

        const uint32_t maximumGroups = getMaximumWeaponGroups(context);

        if (maximumGroups == 0u)
        {
            if (macroRequested) { MacroAsymmetryPlanner::reject(context); }
            return;
        }

        const uint32_t targetGroups = context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 1u, maximumGroups);
        uint32_t nextSymmetryGroup = 1u;
        bool forcedTypePending = context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedLargeWeaponType.has_value();

        for (uint32_t groupIndex = 0u; groupIndex < targetGroups; ++groupIndex)
        {
            bool placed = false;

            for (uint32_t attempt = 0u; attempt < MaximumWeaponPlacementAttempts; ++attempt)
            {
                if (context.DebugInfo != nullptr)
                {
                    ++context.DebugInfo->WeaponPlacementAttemptCount;
                }

                if (attempt == MaximumWeaponPlacementAttempts / 2u && context.MacroAsymmetry.targets(MacroAsymmetryCategory::LARGE_WEAPON))
                {
                    MacroAsymmetryPlanner::reject(context);
                }

                const bool plannedAsymmetry = context.MacroAsymmetry.targets(MacroAsymmetryCategory::LARGE_WEAPON);
                const WeaponHardpoint* hardpointPtr = nullptr;

                if (!usesWeightedHardpointSelection(context.Profile))
                {
                    const uint32_t startIndex = context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, static_cast<uint32_t>(hardpoints.size() - 1u));
                    for (uint32_t offset = 0u; offset < hardpoints.size(); ++offset)
                    {
                        const WeaponHardpoint& possible = hardpoints[(startIndex + offset) % hardpoints.size()];
                        if (!plannedAsymmetry || hardpointMatchesMacroAsymmetryPlan(context, possible))
                        {
                            hardpointPtr = &possible;
                            break;
                        }
                    }
                }
                else
                {
                    uint64_t totalHardpointWeight = 0u;
                    for (const WeaponHardpoint& possible : hardpoints)
                    {
                        if (plannedAsymmetry && !hardpointMatchesMacroAsymmetryPlan(context, possible)) { continue; }
                        uint32_t hardpointWeight = context.Profile.LargeWeaponHardpointWeights.getWeight(possible.Region);
                        if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.targets(ShipVisualAnchorType::WEAPONS))
                        {
                            const GenerationSpatialRegion region = getWeaponSpatialRegion(possible.Region, possible.X, width);
                            const GenerationSpatialRegion target = context.VisualHierarchy.TargetRegion;
                            if (region == target || region == context.SpatialBudget.getMirroredRegion(target))
                            {
                                hardpointWeight = hardpointWeight * context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::WEAPONS) / 100u;
                            }
                        }
                        totalHardpointWeight += hardpointWeight;
                    }

                    if (totalHardpointWeight > 0u)
                    {
                        uint64_t hardpointRoll = context.getGenerationRandomUInt64(GenerationDomain::WEAPONS, 0u, totalHardpointWeight - 1u);
                        for (const WeaponHardpoint& possible : hardpoints)
                        {
                            if (plannedAsymmetry && !hardpointMatchesMacroAsymmetryPlan(context, possible)) { continue; }
                            uint32_t weight = context.Profile.LargeWeaponHardpointWeights.getWeight(possible.Region);
                            if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.targets(ShipVisualAnchorType::WEAPONS))
                            {
                                const GenerationSpatialRegion region = getWeaponSpatialRegion(possible.Region, possible.X, width);
                                const GenerationSpatialRegion target = context.VisualHierarchy.TargetRegion;
                                if (region == target || region == context.SpatialBudget.getMirroredRegion(target))
                                {
                                    weight = weight * context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::WEAPONS) / 100u;
                                }
                            }
                            if (hardpointRoll < weight)
                            {
                                hardpointPtr = &possible;
                                break;
                            }
                            hardpointRoll -= weight;
                        }
                    }
                }

                if (hardpointPtr == nullptr) { continue; }
                const WeaponHardpoint& hardpoint = *hardpointPtr;
                const ShipWeaponType type = selectWeaponType(context, factionProfile, hardpoint.Region, forcedTypePending);

                if (type == ShipWeaponType::SHIP_WEAPON_TYPE_END)
                {
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->WeaponPlacementRejectionCount;
                    }

                    continue;
                }

                CandidateWeapon candidate(width, height);

                if (!generateCandidate(context, hardpoint, type, factionProfile, candidate) || !validateCandidate(context, candidate))
                {
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->WeaponPlacementRejectionCount;
                    }

                    continue;
                }

                const bool wantsPair = !plannedAsymmetry && hardpoint.PairCapable && context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < getSymmetryChance(context, factionProfile, hardpoint);

                if (wantsPair)
                {
                    CandidateWeapon mirrored(width, height);
                    mirrorCandidate(candidate, mirrored, width);

                    if (validateSymmetricPair(context, candidate, mirrored))
                    {
                        auto spatialRegions = context.SpatialBudget.makeRegionSet(getWeaponSpatialRegion(hardpoint.Region, hardpoint.X, width));
                        context.SpatialBudget.addRegion(spatialRegions, context.SpatialBudget.getMirroredRegion(getWeaponSpatialRegion(hardpoint.Region, hardpoint.X, width)));
                        const uint32_t localCost = getWeaponComplexityCost(type);
                        const uint32_t spatialAcceptance = context.SpatialBudget.getPlacementAcceptancePercent(spatialRegions, localCost, true);
                        if (spatialAcceptance == 0u || context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) >= spatialAcceptance)
                        {
                            context.SpatialBudget.recordRejection(spatialRegions);
                            if (context.DebugInfo != nullptr) { ++context.DebugInfo->WeaponPlacementRejectionCount; }
                            continue;
                        }

                        if (context.ComplexityBudget.tryConsume(GenerationComplexityCategory::LARGE_WEAPON, getPairedWeaponComplexityCost(type)))
                        {
                            context.SpatialBudget.consume(spatialRegions, localCost, true);
                            commitCandidate(context, candidate, nextSymmetryGroup);
                            commitCandidate(context, mirrored, nextSymmetryGroup);
                            ++nextSymmetryGroup;
                            forcedTypePending = false;
                            placed = true;
                            break;
                        }
                    }
                }

                const uint32_t localCost = getWeaponComplexityCost(type);
                if (plannedAsymmetry && !MacroAsymmetryPlanner::canAcceptCandidate(context, candidate.OccupiedMask, localCost))
                {
                    if (context.DebugInfo != nullptr) { ++context.DebugInfo->WeaponPlacementRejectionCount; }
                    continue;
                }
                const auto spatialRegions = context.SpatialBudget.makeRegionSet(getWeaponSpatialRegion(hardpoint.Region, hardpoint.X, width));
                const uint32_t spatialAcceptance = context.SpatialBudget.getPlacementAcceptancePercent(spatialRegions, localCost, true);
                if (spatialAcceptance == 0u || context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) >= spatialAcceptance)
                {
                    context.SpatialBudget.recordRejection(spatialRegions);
                    if (context.DebugInfo != nullptr) { ++context.DebugInfo->WeaponPlacementRejectionCount; }
                    continue;
                }

                if (!context.ComplexityBudget.tryConsume(GenerationComplexityCategory::LARGE_WEAPON, localCost))
                {
                    break;
                }

                if (plannedAsymmetry) { MacroAsymmetryPlanner::fulfill(context, candidate.OccupiedMask, localCost); }
                context.SpatialBudget.consume(spatialRegions, localCost, true);
                commitCandidate(context, candidate, 0u);
                forcedTypePending = false;
                placed = true;
                break;
            }

            if (!placed)
            {
                break;
            }
        }

        if (context.MacroAsymmetry.targets(MacroAsymmetryCategory::LARGE_WEAPON)) { MacroAsymmetryPlanner::reject(context); }

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->WeaponCount = static_cast<uint32_t>(context.Weapons.Placements.size());
            context.DebugInfo->WeaponPixelCount = PixelMaskUtils::getMaskPixelCount(context.Weapons.OccupiedMask);
            context.DebugInfo->WeaponMovablePixelCount = PixelMaskUtils::getMaskPixelCount(context.Weapons.MovableMask);
            context.DebugInfo->WeaponOccupiedMask = context.Weapons.OccupiedMask;
        }
    }

    std::vector<WeaponGenerator::WeaponHardpoint> WeaponGenerator::discoverHardpoints(const ShipGenerationContext& context) const
    {
        std::vector<WeaponHardpoint> hardpoints;
        const PixelMaskUtils::MaskBounds hullBounds = PixelMaskUtils::calculateMaskBounds(context.Ship.HullMask);

        if (!hullBounds.Valid)
        {
            return hardpoints;
        }

        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t hullHeight = hullBounds.MaxY - hullBounds.MinY + 1u;
        const uint32_t leftCenter = (width - 1u) / 2u;
        const uint32_t rightCenter = width / 2u;
        const uint32_t rowStep = std::max(1u, GenerationMath::scalePixelsFrom64(3u, context.Ship.HullMask.getHeight()));
        uint32_t maximumRowWidth = 0u;

        for (uint32_t y = hullBounds.MinY; y <= hullBounds.MaxY; ++y)
        {
            maximumRowWidth = std::max(maximumRowWidth, PixelMaskUtils::getOccupiedRowWidth(context.Ship.HullMask, y));
        }

        for (uint32_t y = hullBounds.MinY; y <= hullBounds.MaxY; y += rowStep)
        {
            const uint32_t verticalPercent = hullHeight > 1u ? ((y - hullBounds.MinY) * 100u) / (hullHeight - 1u) : 0u;
            const uint32_t fuselageHalfWidth = y < context.WingRegions.FuselageHalfWidths.size() ? context.WingRegions.FuselageHalfWidths[y] : 0u;

            if (verticalPercent >= 12u && verticalPercent <= 32u)
            {
                WeaponHardpoint central = { rightCenter, y, ShipWeaponHardpointRegion::CENTRAL_NOSE, ShipAttachmentDirection::UP, false };

                if (context.Ship.HullMask.get(central.X, central.Y) && isHardpointSupported(context, central))
                {
                    hardpoints.push_back(central);
                }
            }

            if (verticalPercent >= 28u && verticalPercent <= 52u)
            {
                WeaponHardpoint central = { rightCenter, y, ShipWeaponHardpointRegion::CENTRAL_BODY, ShipAttachmentDirection::UP, false };

                if (context.Ship.HullMask.get(central.X, central.Y) && isHardpointSupported(context, central))
                {
                    hardpoints.push_back(central);
                }
            }

            if (fuselageHalfWidth >= 2u && verticalPercent >= 20u && verticalPercent <= 48u)
            {
                const uint32_t leftX = leftCenter - (fuselageHalfWidth - 1u);
                const uint32_t rightX = rightCenter + (fuselageHalfWidth - 1u);
                addSymmetricSideHardpoints(context, hardpoints, y, leftX, rightX, ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE);
            }

            if (context.WingRegions.hasWings())
            {
                uint32_t leftRootX = width;
                uint32_t leftOuterX = width;
                bool rootFound = false;
                bool outerFound = false;

                for (uint32_t x = 0u; x <= leftCenter; ++x)
                {
                    if (context.WingRegions.WingRootMask.get(x, y))
                    {
                        leftRootX = std::min(leftRootX, x);
                        rootFound = true;
                    }

                    if (context.WingRegions.OuterWingMask.get(x, y))
                    {
                        leftOuterX = std::min(leftOuterX, x);
                        outerFound = true;
                    }
                }

                if (rootFound)
                {
                    addSymmetricSideHardpoints(context, hardpoints, y, leftRootX, width - 1u - leftRootX, ShipWeaponHardpointRegion::WING_ROOT);
                }

                if (outerFound && verticalPercent <= 70u)
                {
                    addSymmetricSideHardpoints(context, hardpoints, y, leftOuterX, width - 1u - leftOuterX, ShipWeaponHardpointRegion::OUTER_WING);
                }
            }

            const uint32_t rowWidth = PixelMaskUtils::getOccupiedRowWidth(context.Ship.HullMask, y);

            if (verticalPercent >= 30u && verticalPercent <= 56u && maximumRowWidth > 0u && rowWidth * 100u >= maximumRowWidth * 72u)
            {
                uint32_t leftEdge = width;
                uint32_t rightEdge = 0u;
                bool rowFound = false;

                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (!context.Ship.HullMask.get(x, y))
                    {
                        continue;
                    }

                    leftEdge = std::min(leftEdge, x);
                    rightEdge = std::max(rightEdge, x);
                    rowFound = true;
                }

                if (rowFound && leftEdge < rightEdge)
                {
                    addSymmetricSideHardpoints(context, hardpoints, y, leftEdge, rightEdge, ShipWeaponHardpointRegion::FORWARD_SHOULDER);
                }
            }
        }

        return hardpoints;
    }

    void WeaponGenerator::addSymmetricSideHardpoints(const ShipGenerationContext& context, std::vector<WeaponHardpoint>& hardpoints, uint32_t y, uint32_t leftX, uint32_t rightX, ShipWeaponHardpointRegion region) const
    {
        if (leftX >= context.Ship.HullMask.getWidth() || rightX >= context.Ship.HullMask.getWidth() || leftX >= rightX)
        {
            return;
        }

        WeaponHardpoint left = { leftX, y, region, ShipAttachmentDirection::UP, true };
        WeaponHardpoint right = { rightX, y, region, ShipAttachmentDirection::UP, true };

        if (isHardpointSupported(context, left))
        {
            hardpoints.push_back(left);
        }

        if (isHardpointSupported(context, right))
        {
            hardpoints.push_back(right);
        }
    }

    bool WeaponGenerator::isHardpointSupported(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const
    {
        if (!context.Ship.HullMask.isInBounds(hardpoint.X, hardpoint.Y) || !context.Ship.HullMask.get(hardpoint.X, hardpoint.Y))
        {
            return false;
        }

        if (hardpoint.Y < 2u || context.Ship.CockpitMask.get(hardpoint.X, hardpoint.Y) || context.Ship.EngineMask.get(hardpoint.X, hardpoint.Y) || context.MajorFeatures.OccupiedMask.get(hardpoint.X, hardpoint.Y))
        {
            return false;
        }

        uint32_t supportCount = 0u;

        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY)
        {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX)
            {
                const int32_t x = static_cast<int32_t>(hardpoint.X) + offsetX;
                const int32_t y = static_cast<int32_t>(hardpoint.Y) + offsetY;

                if (PixelMaskUtils::isMaskPixel(context.Ship.HullMask, x, y) && !PixelMaskUtils::isMaskPixel(context.Ship.CockpitMask, x, y) && !PixelMaskUtils::isMaskPixel(context.MajorFeatures.OccupiedMask, x, y))
                {
                    ++supportCount;
                }
            }
        }

        const uint32_t minimumSupport = context.ScaleTraits.SmallFeatureCapacity == 0u ? 3u : 4u;
        return supportCount >= minimumSupport;
    }

    bool WeaponGenerator::hardpointMatchesMacroAsymmetryPlan(const ShipGenerationContext& context, const WeaponHardpoint& hardpoint) const
    {
        if (!context.MacroAsymmetry.targets(MacroAsymmetryCategory::LARGE_WEAPON)) { return true; }
        if (!context.MacroAsymmetry.isDominantX(hardpoint.X, context.Ship.HullMask.getWidth())) { return false; }
        const GenerationSpatialRegion region = getWeaponSpatialRegion(hardpoint.Region, hardpoint.X, context.Ship.HullMask.getWidth());
        const GenerationSpatialRegion target = context.MacroAsymmetry.TargetRegion;
        if (target == GenerationSpatialRegion::LEFT_OUTER_WING || target == GenerationSpatialRegion::RIGHT_OUTER_WING || target == GenerationSpatialRegion::LEFT_WING_ROOT || target == GenerationSpatialRegion::RIGHT_WING_ROOT)
        {
            return region == target;
        }
        return region == target || (target == GenerationSpatialRegion::MID_FUSELAGE && region == GenerationSpatialRegion::FRONT_FUSELAGE);
    }

    ShipWeaponType WeaponGenerator::selectWeaponType(ShipGenerationContext& context, const FactionWeaponProfile& factionProfile, ShipWeaponHardpointRegion region, bool allowForcedType) const
    {
        std::array<uint64_t, static_cast<std::size_t>(ShipWeaponType::SHIP_WEAPON_TYPE_END)> weights = {};
        uint64_t totalWeight = 0u;

        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipWeaponType::SHIP_WEAPON_TYPE_END); ++index)
        {
            const ShipWeaponType type = static_cast<ShipWeaponType>(index);

            if (!isWeaponTypeAllowedAtHardpoint(type, region))
            {
                continue;
            }

            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::LARGE_WEAPON, getWeaponComplexityCost(type)))
            {
                continue;
            }

            weights[index] = static_cast<uint64_t>(getStyleWeaponWeight(context.Profile, type)) * factionProfile.WeightMultipliers[index];
            totalWeight += weights[index];
        }

        if (totalWeight == 0u)
        {
            return ShipWeaponType::SHIP_WEAPON_TYPE_END;
        }

        uint64_t roll = context.getGenerationRandomUInt64(GenerationDomain::WEAPONS, 0u, totalWeight - 1u);

        if (allowForcedType && context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedLargeWeaponType.has_value())
        {
            const ShipWeaponType forced = *context.CalibrationSettings->Overrides.ForcedLargeWeaponType;
            const std::size_t forcedIndex = static_cast<std::size_t>(forced);
            if (forced != ShipWeaponType::SHIP_WEAPON_TYPE_END && forcedIndex < weights.size() && weights[forcedIndex] > 0u && isWeaponTypeAllowedAtHardpoint(forced, region))
            {
                return forced;
            }
        }

        for (uint32_t index = 0u; index < weights.size(); ++index)
        {
            if (roll < weights[index])
            {
                return static_cast<ShipWeaponType>(index);
            }

            roll -= weights[index];
        }

        return ShipWeaponType::SHIP_WEAPON_TYPE_END;
    }

    uint32_t WeaponGenerator::getStyleWeaponWeight(const ShipGenerationProfile& profile, ShipWeaponType type) const
    {
        return profile.LargeWeaponWeights.getWeight(type);
    }

    bool WeaponGenerator::isWeaponTypeAllowedAtHardpoint(ShipWeaponType type, ShipWeaponHardpointRegion region) const
    {
        switch (region)
        {
        case ShipWeaponHardpointRegion::CENTRAL_NOSE: return type == ShipWeaponType::SINGLE_CANNON || type == ShipWeaponType::TWIN_CANNON || type == ShipWeaponType::COMPACT_TURRET || type == ShipWeaponType::RAIL_WEAPON;
        case ShipWeaponHardpointRegion::CENTRAL_BODY: return type == ShipWeaponType::COMPACT_TURRET || type == ShipWeaponType::RAIL_WEAPON || type == ShipWeaponType::WEAPON_POD;
        case ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE: return type == ShipWeaponType::SINGLE_CANNON || type == ShipWeaponType::TWIN_CANNON || type == ShipWeaponType::RAIL_WEAPON;
        case ShipWeaponHardpointRegion::WING_ROOT: return type != ShipWeaponType::COMPACT_TURRET;
        case ShipWeaponHardpointRegion::OUTER_WING: return type == ShipWeaponType::SINGLE_CANNON || type == ShipWeaponType::TWIN_CANNON || type == ShipWeaponType::WEAPON_POD;
        case ShipWeaponHardpointRegion::FORWARD_SHOULDER: return type == ShipWeaponType::TWIN_CANNON || type == ShipWeaponType::COMPACT_TURRET || type == ShipWeaponType::WEAPON_POD || type == ShipWeaponType::SINGLE_CANNON;
        default: return false;
        }
    }

    bool WeaponGenerator::generateCandidate(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, ShipWeaponType type, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
    {
        candidate.Placement.Type = type;
        candidate.Placement.Region = hardpoint.Region;
        candidate.Placement.Direction = hardpoint.Direction;
        candidate.Placement.AnchorX = hardpoint.X;
        candidate.Placement.AnchorY = hardpoint.Y;

        bool generated = false;

        switch (type)
        {
        case ShipWeaponType::SINGLE_CANNON: generated = generateSingleCannon(context, hardpoint, factionProfile, candidate); break;
        case ShipWeaponType::TWIN_CANNON: generated = generateTwinCannon(context, hardpoint, factionProfile, candidate); break;
        case ShipWeaponType::COMPACT_TURRET: generated = generateCompactTurret(context, hardpoint, factionProfile, candidate); break;
        case ShipWeaponType::RAIL_WEAPON: generated = generateRailWeapon(context, hardpoint, factionProfile, candidate); break;
        case ShipWeaponType::WEAPON_POD: generated = generateWeaponPod(context, hardpoint, factionProfile, candidate); break;
        default: return false;
        }

        if (!generated)
        {
            return false;
        }

        const PixelMaskUtils::MaskBounds bodyBounds = PixelMaskUtils::calculateMaskBounds(candidate.BodyMask);
        const PixelMaskUtils::MaskBounds barrelBounds = PixelMaskUtils::calculateMaskBounds(candidate.BarrelMask);
        const PixelMaskUtils::MaskBounds muzzleBounds = PixelMaskUtils::calculateMaskBounds(candidate.MuzzleMask);

        if (!bodyBounds.Valid || !barrelBounds.Valid || !muzzleBounds.Valid)
        {
            return false;
        }

        candidate.Placement.BodyMinX = bodyBounds.MinX;
        candidate.Placement.BodyMaxX = bodyBounds.MaxX;
        candidate.Placement.BodyMinY = bodyBounds.MinY;
        candidate.Placement.BodyMaxY = bodyBounds.MaxY;
        candidate.Placement.BarrelMinX = barrelBounds.MinX;
        candidate.Placement.BarrelMaxX = barrelBounds.MaxX;
        candidate.Placement.BarrelMinY = barrelBounds.MinY;
        candidate.Placement.BarrelMaxY = barrelBounds.MaxY;
        const uint32_t muzzleCenterX = (muzzleBounds.MinX + muzzleBounds.MaxX) / 2u;
        candidate.Placement.MuzzleX = muzzleBounds.MinX;
        candidate.Placement.MuzzleY = muzzleBounds.MinY;
        uint32_t bestMuzzleDistance = candidate.OccupiedMask.getWidth();

        for (uint32_t x = muzzleBounds.MinX; x <= muzzleBounds.MaxX; ++x)
        {
            if (!candidate.MuzzleMask.get(x, muzzleBounds.MinY))
            {
                continue;
            }

            const uint32_t distance = x > muzzleCenterX ? x - muzzleCenterX : muzzleCenterX - x;

            if (distance < bestMuzzleDistance)
            {
                bestMuzzleDistance = distance;
                candidate.Placement.MuzzleX = x;
            }
        }
        candidate.Placement.MovableBarrel = PixelMaskUtils::getMaskPixelCount(candidate.MovableMask) > 0u;
        candidate.Placement.Emissive = PixelMaskUtils::getMaskPixelCount(candidate.EmissiveMask) > 0u;
        return true;
    }

    bool WeaponGenerator::generateSingleCannon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
    {
        const uint32_t scalePercent = context.Profile.LargeWeaponScalePercent;
        const uint32_t width = candidate.OccupiedMask.getWidth();
        const uint32_t height = candidate.OccupiedMask.getHeight();
        const uint32_t bodyWidth = std::max(1u, scaleWeaponPixelsFrom64(4u, width, scalePercent));
        const uint32_t bodyLength = std::max(1u, scaleWeaponPixelsFrom64(3u, height, scalePercent));
        const uint32_t barrelLength = std::max(1u, scaleWeaponPixelsFrom64(5u, height, scalePercent));
        const uint32_t barrelWidth = std::min(bodyWidth, std::max(1u, scaleWeaponPixelsFrom64(1u, width, scalePercent)));
        const uint32_t totalLength = bodyLength + barrelLength;

        if (hardpoint.Y + 1u < totalLength || !buildRoot(context, hardpoint, bodyWidth > 2u ? 1u : 0u, std::max(1u, scaleWeaponPixelsFrom64(2u, height, scalePercent)), candidate))
        {
            return false;
        }

        const int32_t bodyX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(bodyWidth / 2u);
        const int32_t bodyY = static_cast<int32_t>(hardpoint.Y) - static_cast<int32_t>(bodyLength) + 1;
        const int32_t barrelX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(barrelWidth / 2u);
        const int32_t barrelY = bodyY - static_cast<int32_t>(barrelLength);

        if (!addCandidateRectangle(candidate, candidate.BodyMask, bodyX, bodyY, bodyWidth, bodyLength) || !addCandidateRectangle(candidate, candidate.BarrelMask, barrelX, barrelY, barrelWidth, barrelLength))
        {
            return false;
        }

        const uint32_t muzzleWidth = std::min(bodyWidth, std::max(barrelWidth, scaleWeaponPixelsFrom64(2u, width, scalePercent)));
        const int32_t muzzleX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(muzzleWidth / 2u);

        if (!addCandidateRectangle(candidate, candidate.MuzzleMask, muzzleX, barrelY, muzzleWidth, 1u))
        {
            return false;
        }

        for (uint32_t y = static_cast<uint32_t>(barrelY); y < static_cast<uint32_t>(bodyY); ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (candidate.BarrelMask.get(x, y)) { candidate.MovableMask.set(x, y, true); }
            }
        }

        const bool emissive = context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < factionProfile.EmissiveChance;
        if (emissive) { candidate.EmissiveMask.set(candidate.Placement.AnchorX, static_cast<uint32_t>(barrelY), true); }
        return true;
    }

    bool WeaponGenerator::generateTwinCannon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
    {
        const uint32_t scalePercent = context.Profile.LargeWeaponScalePercent;
        const uint32_t width = candidate.OccupiedMask.getWidth();
        const uint32_t height = candidate.OccupiedMask.getHeight();
        const uint32_t bodyWidth = std::max(3u, scaleWeaponPixelsFrom64(6u, width, scalePercent));
        const uint32_t bodyLength = std::max(1u, scaleWeaponPixelsFrom64(3u, height, scalePercent));
        const uint32_t barrelLength = std::max(2u, scaleWeaponPixelsFrom64(5u, height, scalePercent));
        const uint32_t barrelWidth = std::max(1u, scaleWeaponPixelsFrom64(1u, width, scalePercent));
        const uint32_t separation = std::max(1u, bodyWidth / 4u);

        if (hardpoint.Y + 1u < bodyLength + barrelLength || !buildRoot(context, hardpoint, std::max(1u, bodyWidth / 3u), std::max(1u, scaleWeaponPixelsFrom64(2u, height, scalePercent)), candidate))
        {
            return false;
        }

        const int32_t bodyX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(bodyWidth / 2u);
        const int32_t bodyY = static_cast<int32_t>(hardpoint.Y) - static_cast<int32_t>(bodyLength) + 1;
        const int32_t barrelY = bodyY - static_cast<int32_t>(barrelLength);
        const int32_t leftBarrelX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(separation) - static_cast<int32_t>(barrelWidth);
        const int32_t rightBarrelX = static_cast<int32_t>(hardpoint.X) + static_cast<int32_t>(separation);

        if (!addCandidateRectangle(candidate, candidate.BodyMask, bodyX, bodyY, bodyWidth, bodyLength) || !addCandidateRectangle(candidate, candidate.BarrelMask, leftBarrelX, barrelY, barrelWidth, barrelLength) || !addCandidateRectangle(candidate, candidate.BarrelMask, rightBarrelX, barrelY, barrelWidth, barrelLength))
        {
            return false;
        }

        if (!addCandidateRectangle(candidate, candidate.MuzzleMask, leftBarrelX, barrelY, barrelWidth, 1u) || !addCandidateRectangle(candidate, candidate.MuzzleMask, rightBarrelX, barrelY, barrelWidth, 1u))
        {
            return false;
        }

        PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.BarrelMask);
        PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.MuzzleMask);

        if (context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < factionProfile.EmissiveChance)
        {
            PixelMaskUtils::mergeMask(candidate.EmissiveMask, candidate.MuzzleMask);
        }

        return true;
    }

    bool WeaponGenerator::generateCompactTurret(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
    {
        const uint32_t scalePercent = context.Profile.LargeWeaponScalePercent;
        const uint32_t width = candidate.OccupiedMask.getWidth();
        const uint32_t height = candidate.OccupiedMask.getHeight();
        const uint32_t bodyWidth = std::max(3u, scaleWeaponPixelsFrom64(6u, width, scalePercent));
        const uint32_t bodyLength = std::max(2u, scaleWeaponPixelsFrom64(4u, height, scalePercent));
        const uint32_t barrelLength = std::max(1u, scaleWeaponPixelsFrom64(3u, height, scalePercent));

        if (hardpoint.Y + 1u < bodyLength + barrelLength || !buildRoot(context, hardpoint, std::max(1u, bodyWidth / 3u), std::max(1u, scaleWeaponPixelsFrom64(2u, height, scalePercent)), candidate))
        {
            return false;
        }

        const int32_t bodyY = static_cast<int32_t>(hardpoint.Y) - static_cast<int32_t>(bodyLength) + 1;

        for (uint32_t row = 0u; row < bodyLength; ++row)
        {
            uint32_t rowWidth = bodyWidth;
            if (row == 0u || row + 1u == bodyLength) { rowWidth = std::max(1u, bodyWidth - 2u); }
            const int32_t rowX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(rowWidth / 2u);
            if (!addCandidateRectangle(candidate, candidate.BodyMask, rowX, bodyY + static_cast<int32_t>(row), rowWidth, 1u)) { return false; }
        }

        const int32_t barrelY = bodyY - static_cast<int32_t>(barrelLength);
        if (!addCandidateRectangle(candidate, candidate.BarrelMask, static_cast<int32_t>(hardpoint.X), barrelY, 1u, barrelLength) || !addCandidatePixel(candidate, candidate.MuzzleMask, static_cast<int32_t>(hardpoint.X), barrelY))
        {
            return false;
        }

        PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.BarrelMask);
        PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.MuzzleMask);

        if (context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < factionProfile.EmissiveChance)
        {
            candidate.EmissiveMask.set(hardpoint.X, static_cast<uint32_t>(barrelY), true);
        }

        return true;
    }

    bool WeaponGenerator::generateRailWeapon(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
    {
        const uint32_t scalePercent = context.Profile.LargeWeaponScalePercent;
        const uint32_t width = candidate.OccupiedMask.getWidth();
        const uint32_t height = candidate.OccupiedMask.getHeight();
        const uint32_t bodyWidth = std::max(2u, scaleWeaponPixelsFrom64(3u, width, scalePercent));
        const uint32_t bodyLength = std::max(2u, scaleWeaponPixelsFrom64(4u, height, scalePercent));
        const uint32_t barrelLength = std::max(3u, scaleWeaponPixelsFrom64(8u, height, scalePercent));

        if (hardpoint.Y + 1u < bodyLength + barrelLength || !buildRoot(context, hardpoint, bodyWidth > 2u ? 1u : 0u, std::max(1u, scaleWeaponPixelsFrom64(2u, height, scalePercent)), candidate))
        {
            return false;
        }

        const int32_t bodyX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(bodyWidth / 2u);
        const int32_t bodyY = static_cast<int32_t>(hardpoint.Y) - static_cast<int32_t>(bodyLength) + 1;
        const int32_t barrelY = bodyY - static_cast<int32_t>(barrelLength);

        if (!addCandidateRectangle(candidate, candidate.BodyMask, bodyX, bodyY, bodyWidth, bodyLength) || !addCandidateRectangle(candidate, candidate.BarrelMask, static_cast<int32_t>(hardpoint.X), barrelY, 1u, barrelLength) || !addCandidatePixel(candidate, candidate.MuzzleMask, static_cast<int32_t>(hardpoint.X), barrelY))
        {
            return false;
        }

        PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.BarrelMask);
        PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.MuzzleMask);

        if (context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < std::max(35u, factionProfile.EmissiveChance))
        {
            for (int32_t y = bodyY; y >= std::max<int32_t>(barrelY, bodyY - static_cast<int32_t>(std::max(1u, bodyLength / 2u))); --y)
            {
                if (y >= 0) { candidate.EmissiveMask.set(hardpoint.X, static_cast<uint32_t>(y), true); }
            }
        }

        return true;
    }

    bool WeaponGenerator::generateWeaponPod(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, const FactionWeaponProfile& factionProfile, CandidateWeapon& candidate) const
    {
        const uint32_t scalePercent = context.Profile.LargeWeaponScalePercent;
        const uint32_t width = candidate.OccupiedMask.getWidth();
        const uint32_t height = candidate.OccupiedMask.getHeight();
        const uint32_t bodyWidth = std::max(4u, scaleWeaponPixelsFrom64(8u, width, scalePercent));
        const uint32_t bodyLength = std::max(2u, scaleWeaponPixelsFrom64(5u, height, scalePercent));
        const uint32_t barrelLength = std::max(1u, scaleWeaponPixelsFrom64(3u, height, scalePercent));
        const uint32_t barrelOffset = std::max(1u, bodyWidth / 4u);

        if (hardpoint.Y + 1u < bodyLength + barrelLength || !buildRoot(context, hardpoint, std::max(1u, bodyWidth / 3u), std::max(1u, scaleWeaponPixelsFrom64(2u, height, scalePercent)), candidate))
        {
            return false;
        }

        const int32_t bodyY = static_cast<int32_t>(hardpoint.Y) - static_cast<int32_t>(bodyLength) + 1;
        const int32_t barrelY = bodyY - static_cast<int32_t>(barrelLength);
        const int32_t leftBarrelX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(barrelOffset);
        const int32_t rightBarrelX = static_cast<int32_t>(hardpoint.X) + static_cast<int32_t>(barrelOffset);

        for (uint32_t row = 0u; row < bodyLength; ++row)
        {
            const uint32_t rowWidth = row == 0u ? std::max(2u, bodyWidth - 2u) : bodyWidth;
            const int32_t rowX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(rowWidth / 2u);
            if (!addCandidateRectangle(candidate, candidate.BodyMask, rowX, bodyY + static_cast<int32_t>(row), rowWidth, 1u)) { return false; }
        }

        if (!addCandidateRectangle(candidate, candidate.BarrelMask, leftBarrelX, barrelY, 1u, barrelLength) || !addCandidateRectangle(candidate, candidate.BarrelMask, rightBarrelX, barrelY, 1u, barrelLength) || !addCandidatePixel(candidate, candidate.MuzzleMask, leftBarrelX, barrelY) || !addCandidatePixel(candidate, candidate.MuzzleMask, rightBarrelX, barrelY))
        {
            return false;
        }

        PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.BarrelMask);
        PixelMaskUtils::mergeMask(candidate.MovableMask, candidate.MuzzleMask);

        if (context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < factionProfile.EmissiveChance)
        {
            PixelMaskUtils::mergeMask(candidate.EmissiveMask, candidate.MuzzleMask);
        }

        return true;
    }

    bool WeaponGenerator::buildRoot(ShipGenerationContext& context, const WeaponHardpoint& hardpoint, uint32_t halfWidth, uint32_t depth, CandidateWeapon& candidate) const
    {
        const int32_t startX = static_cast<int32_t>(hardpoint.X) - static_cast<int32_t>(halfWidth);
        uint32_t supportCount = 0u;
        uint32_t possibleCount = 0u;

        for (uint32_t offsetY = 0u; offsetY < depth; ++offsetY)
        {
            const uint32_t y = hardpoint.Y + offsetY;

            if (y >= context.Ship.HullMask.getHeight())
            {
                break;
            }

            for (uint32_t offsetX = 0u; offsetX < halfWidth * 2u + 1u; ++offsetX)
            {
                const int32_t x = startX + static_cast<int32_t>(offsetX);
                ++possibleCount;

                if (x < 0 || x >= static_cast<int32_t>(context.Ship.HullMask.getWidth()))
                {
                    continue;
                }

                const uint32_t pixelX = static_cast<uint32_t>(x);

                if (!context.Ship.HullMask.get(pixelX, y) || context.Ship.CockpitMask.get(pixelX, y) || context.Ship.EngineMask.get(pixelX, y) || context.MajorFeatures.OccupiedMask.get(pixelX, y))
                {
                    continue;
                }

                candidate.RootMask.set(pixelX, y, true);
                candidate.OccupiedMask.set(pixelX, y, true);
                ++supportCount;
            }
        }

        const uint32_t minimumSupport = std::max(1u, (possibleCount + 1u) / 2u);
        return supportCount >= minimumSupport;
    }

    bool WeaponGenerator::addCandidateRectangle(CandidateWeapon& candidate, PixelMask& semanticMask, int32_t startX, int32_t startY, uint32_t width, uint32_t height) const
    {
        if (width == 0u || height == 0u || startX < 0 || startY < 0)
        {
            return false;
        }

        if (startX + static_cast<int32_t>(width) > static_cast<int32_t>(candidate.OccupiedMask.getWidth()) || startY + static_cast<int32_t>(height) > static_cast<int32_t>(candidate.OccupiedMask.getHeight()))
        {
            return false;
        }

        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                semanticMask.set(static_cast<uint32_t>(startX) + x, static_cast<uint32_t>(startY) + y, true);
                candidate.OccupiedMask.set(static_cast<uint32_t>(startX) + x, static_cast<uint32_t>(startY) + y, true);
            }
        }

        return true;
    }

    bool WeaponGenerator::addCandidatePixel(CandidateWeapon& candidate, PixelMask& semanticMask, int32_t x, int32_t y) const
    {
        if (x < 0 || y < 0 || x >= static_cast<int32_t>(candidate.OccupiedMask.getWidth()) || y >= static_cast<int32_t>(candidate.OccupiedMask.getHeight()))
        {
            return false;
        }

        semanticMask.set(static_cast<uint32_t>(x), static_cast<uint32_t>(y), true);
        candidate.OccupiedMask.set(static_cast<uint32_t>(x), static_cast<uint32_t>(y), true);
        return true;
    }

    bool WeaponGenerator::validateCandidate(const ShipGenerationContext& context, const CandidateWeapon& candidate) const
    {
        if (PixelMaskUtils::getMaskPixelCount(candidate.RootMask) == 0u || PixelMaskUtils::getMaskPixelCount(candidate.BodyMask) == 0u || PixelMaskUtils::getMaskPixelCount(candidate.BarrelMask) == 0u || PixelMaskUtils::getMaskPixelCount(candidate.MuzzleMask) == 0u)
        {
            return false;
        }

        for (uint32_t y = 0u; y < candidate.OccupiedMask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < candidate.OccupiedMask.getWidth(); ++x)
            {
                if (candidate.RootMask.get(x, y) && !context.Ship.HullMask.get(x, y))
                {
                    return false;
                }

                if (!candidate.OccupiedMask.get(x, y))
                {
                    continue;
                }

                if (context.StructuralNegativeSpace.ReservedMask.get(x, y) || context.Ship.CockpitMask.get(x, y) || context.Ship.EngineMask.get(x, y) || context.Ship.EngineExhaustMask.get(x, y) || context.MajorFeatures.OccupiedMask.get(x, y) || context.Weapons.OccupiedMask.get(x, y))
                {
                    return false;
                }
            }
        }

        if (!validateConnected(candidate) || !validateFiringClearance(context, candidate))
        {
            return false;
        }

        return candidate.Placement.MuzzleY < candidate.Placement.AnchorY;
    }

    bool WeaponGenerator::validateConnected(const CandidateWeapon& candidate) const
    {
        const uint32_t totalPixels = PixelMaskUtils::getMaskPixelCount(candidate.OccupiedMask);

        if (totalPixels == 0u)
        {
            return false;
        }

        std::queue<std::pair<uint32_t, uint32_t>> queue;
        PixelMask visited(candidate.OccupiedMask.getWidth(), candidate.OccupiedMask.getHeight(), false);
        bool found = false;

        for (uint32_t y = 0u; y < candidate.OccupiedMask.getHeight() && !found; ++y)
        {
            for (uint32_t x = 0u; x < candidate.OccupiedMask.getWidth(); ++x)
            {
                if (candidate.OccupiedMask.get(x, y))
                {
                    queue.push({ x, y });
                    visited.set(x, y, true);
                    found = true;
                    break;
                }
            }
        }

        uint32_t visitedCount = 0u;
        constexpr std::array<std::pair<int32_t, int32_t>, 4u> Directions = { std::pair<int32_t, int32_t>{ 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };

        while (!queue.empty())
        {
            const auto [x, y] = queue.front();
            queue.pop();
            ++visitedCount;

            for (const auto& [dx, dy] : Directions)
            {
                const int32_t nextX = static_cast<int32_t>(x) + dx;
                const int32_t nextY = static_cast<int32_t>(y) + dy;

                if (nextX < 0 || nextY < 0 || nextX >= static_cast<int32_t>(candidate.OccupiedMask.getWidth()) || nextY >= static_cast<int32_t>(candidate.OccupiedMask.getHeight()))
                {
                    continue;
                }

                const uint32_t px = static_cast<uint32_t>(nextX);
                const uint32_t py = static_cast<uint32_t>(nextY);

                if (candidate.OccupiedMask.get(px, py) && !visited.get(px, py))
                {
                    visited.set(px, py, true);
                    queue.push({ px, py });
                }
            }
        }

        return visitedCount == totalPixels;
    }

    bool WeaponGenerator::validateFiringClearance(const ShipGenerationContext& context, const CandidateWeapon& candidate) const
    {
        const PixelMaskUtils::MaskBounds muzzleBounds = PixelMaskUtils::calculateMaskBounds(candidate.MuzzleMask);

        if (!muzzleBounds.Valid || muzzleBounds.MinY == 0u)
        {
            return false;
        }

        bool muzzleOutsideHull = false;

        for (uint32_t x = muzzleBounds.MinX; x <= muzzleBounds.MaxX; ++x)
        {
            if (!candidate.MuzzleMask.get(x, muzzleBounds.MinY))
            {
                continue;
            }

            if (!context.Ship.HullMask.get(x, muzzleBounds.MinY))
            {
                muzzleOutsideHull = true;
            }

            const uint32_t forwardY = muzzleBounds.MinY - 1u;

            if (context.Ship.HullMask.get(x, forwardY) || context.Ship.CockpitMask.get(x, forwardY) || context.Ship.EngineMask.get(x, forwardY) || context.MajorFeatures.OccupiedMask.get(x, forwardY) || context.Weapons.OccupiedMask.get(x, forwardY))
            {
                return false;
            }
        }

        return muzzleOutsideHull;
    }

    void WeaponGenerator::mirrorCandidate(const CandidateWeapon& source, CandidateWeapon& destination, uint32_t imageWidth) const
    {
        auto mirrorMask = [imageWidth](const PixelMask& input, PixelMask& output)
            {
                for (uint32_t y = 0u; y < input.getHeight(); ++y)
                {
                    for (uint32_t x = 0u; x < input.getWidth(); ++x)
                    {
                        if (input.get(x, y)) { output.set(imageWidth - 1u - x, y, true); }
                    }
                }
            };

        mirrorMask(source.OccupiedMask, destination.OccupiedMask);
        mirrorMask(source.RootMask, destination.RootMask);
        mirrorMask(source.BodyMask, destination.BodyMask);
        mirrorMask(source.BarrelMask, destination.BarrelMask);
        mirrorMask(source.MuzzleMask, destination.MuzzleMask);
        mirrorMask(source.MovableMask, destination.MovableMask);
        mirrorMask(source.EmissiveMask, destination.EmissiveMask);
        destination.Placement = source.Placement;
        destination.Placement.AnchorX = imageWidth - 1u - source.Placement.AnchorX;
        destination.Placement.BodyMinX = imageWidth - 1u - source.Placement.BodyMaxX;
        destination.Placement.BodyMaxX = imageWidth - 1u - source.Placement.BodyMinX;
        destination.Placement.BarrelMinX = imageWidth - 1u - source.Placement.BarrelMaxX;
        destination.Placement.BarrelMaxX = imageWidth - 1u - source.Placement.BarrelMinX;
        destination.Placement.MuzzleX = imageWidth - 1u - source.Placement.MuzzleX;
    }

    bool WeaponGenerator::validateSymmetricPair(const ShipGenerationContext& context, const CandidateWeapon& first, const CandidateWeapon& second) const
    {
        if (PixelMaskUtils::masksOverlap(first.OccupiedMask, second.OccupiedMask))
        {
            return false;
        }

        return validateCandidate(context, second);
    }

    void WeaponGenerator::commitCandidate(ShipGenerationContext& context, CandidateWeapon& candidate, uint32_t symmetryGroup) const
    {
        candidate.Placement.SymmetryGroup = symmetryGroup;
        PixelMaskUtils::mergeMask(context.Weapons.OccupiedMask, candidate.OccupiedMask);
        PixelMaskUtils::mergeMask(context.Weapons.RootMask, candidate.RootMask);
        PixelMaskUtils::mergeMask(context.Weapons.BodyMask, candidate.BodyMask);
        PixelMaskUtils::mergeMask(context.Weapons.BarrelMask, candidate.BarrelMask);
        PixelMaskUtils::mergeMask(context.Weapons.MuzzleMask, candidate.MuzzleMask);
        PixelMaskUtils::mergeMask(context.Weapons.MovableMask, candidate.MovableMask);
        PixelMaskUtils::mergeMask(context.Weapons.EmissiveMask, candidate.EmissiveMask);
        context.Weapons.Placements.push_back(candidate.Placement);

        PixelMaskUtils::mergeMask(context.Ship.IdleAnimationMetadata.WeaponOccupiedMask, candidate.OccupiedMask);
        PixelMaskUtils::mergeMask(context.Ship.IdleAnimationMetadata.WeaponMovableMask, candidate.MovableMask);
        PixelMaskUtils::mergeMask(context.Ship.IdleAnimationMetadata.WeaponMuzzleMask, candidate.MuzzleMask);
        PixelMaskUtils::mergeMask(context.Ship.IdleAnimationMetadata.WeaponEmissiveMask, candidate.EmissiveMask);

        ShipWeaponAnimationComponent animationComponent;
        animationComponent.Type = candidate.Placement.Type;
        animationComponent.Region = candidate.Placement.Region;
        animationComponent.Direction = candidate.Placement.Direction;
        animationComponent.AnchorX = candidate.Placement.AnchorX;
        animationComponent.AnchorY = candidate.Placement.AnchorY;
        animationComponent.MinimumX = std::min(candidate.Placement.BodyMinX, candidate.Placement.BarrelMinX);
        animationComponent.MaximumX = std::max(candidate.Placement.BodyMaxX, candidate.Placement.BarrelMaxX);
        animationComponent.MinimumY = std::min(candidate.Placement.BodyMinY, candidate.Placement.BarrelMinY);
        animationComponent.MaximumY = std::max(candidate.Placement.BodyMaxY, candidate.Placement.BarrelMaxY);
        animationComponent.SymmetryGroup = symmetryGroup;
        animationComponent.MovableBarrel = candidate.Placement.MovableBarrel;
        animationComponent.Emissive = candidate.Placement.Emissive;
        context.Ship.IdleAnimationMetadata.WeaponComponents.push_back(animationComponent);

        if (context.DebugInfo != nullptr)
        {
            ++context.DebugInfo->WeaponTypeCounts[static_cast<std::size_t>(candidate.Placement.Type)];
            WeaponUnitDebugInfo debug;
            debug.Type = candidate.Placement.Type;
            debug.Region = candidate.Placement.Region;
            debug.AnchorX = candidate.Placement.AnchorX;
            debug.AnchorY = candidate.Placement.AnchorY;
            debug.BodyMinX = candidate.Placement.BodyMinX;
            debug.BodyMaxX = candidate.Placement.BodyMaxX;
            debug.BodyMinY = candidate.Placement.BodyMinY;
            debug.BodyMaxY = candidate.Placement.BodyMaxY;
            debug.BarrelMinX = candidate.Placement.BarrelMinX;
            debug.BarrelMaxX = candidate.Placement.BarrelMaxX;
            debug.BarrelMinY = candidate.Placement.BarrelMinY;
            debug.BarrelMaxY = candidate.Placement.BarrelMaxY;
            debug.MuzzleX = candidate.Placement.MuzzleX;
            debug.MuzzleY = candidate.Placement.MuzzleY;
            debug.SymmetryGroup = symmetryGroup;
            debug.MovableBarrel = candidate.Placement.MovableBarrel;
            debug.Emissive = candidate.Placement.Emissive;
            context.DebugInfo->WeaponUnits.push_back(debug);
        }
    }

    WeaponGenerator::FactionWeaponProfile WeaponGenerator::getFactionWeaponProfile(ShipFactionType faction) const
    {
        FactionWeaponProfile profile;
        profile.WeightMultipliers.fill(100u);

        switch (faction)
        {
        case ShipFactionType::FRONTIER:
            profile.ChancePercent = 100u;
            profile.SymmetryChanceOffset = -15;
            profile.WeightMultipliers = { 125u, 85u, 90u, 70u, 135u };
            profile.EmissiveChance = 10u;
            break;
        case ShipFactionType::MILITARY:
            profile.ChancePercent = 120u;
            profile.SymmetryChanceOffset = 15;
            profile.WeightMultipliers = { 120u, 145u, 105u, 110u, 115u };
            profile.EmissiveChance = 20u;
            break;
        case ShipFactionType::ASCENDANT:
            profile.ChancePercent = 85u;
            profile.SymmetryChanceOffset = 10;
            profile.WeightMultipliers = { 60u, 80u, 125u, 165u, 55u };
            profile.EmissiveChance = 80u;
            break;
        case ShipFactionType::XENO:
            profile.ChancePercent = 100u;
            profile.SymmetryChanceOffset = -5;
            profile.WeightMultipliers = { 75u, 95u, 135u, 145u, 115u };
            profile.EmissiveChance = 65u;
            break;
        case ShipFactionType::CORPORATE:
            profile.ChancePercent = 108u;
            profile.SymmetryChanceOffset = 24;
            profile.WeightMultipliers = { 82u, 142u, 100u, 88u, 155u };
            profile.EmissiveChance = 28u;
            break;
        case ShipFactionType::RELIC:
            profile.ChancePercent = 94u;
            profile.SymmetryChanceOffset = 4;
            profile.WeightMultipliers = { 58u, 78u, 145u, 175u, 62u };
            profile.EmissiveChance = 76u;
            break;
        default:
            break;
        }

        return profile;
    }

    uint32_t WeaponGenerator::getGenerationChance(const ShipGenerationContext& context, const FactionWeaponProfile& factionProfile) const
    {
        const uint32_t capacity = context.ScaleTraits.MajorFeatureCapacity;
        uint32_t scalePercent = 38u;

        if (capacity < 20u)
        {
            scalePercent += static_cast<uint32_t>((static_cast<uint64_t>(32u) * capacity + 10u) / 20u);
        }
        else if (capacity < 40u)
        {
            scalePercent = 70u + static_cast<uint32_t>((static_cast<uint64_t>(30u) * (capacity - 20u) + 10u) / 20u);
        }
        else if (capacity < 80u)
        {
            scalePercent = 100u + static_cast<uint32_t>((static_cast<uint64_t>(10u) * (capacity - 40u) + 20u) / 40u);
        }
        else
        {
            scalePercent = 110u;
        }

        uint64_t chance = static_cast<uint64_t>(context.Profile.LargeWeaponChance) * factionProfile.ChancePercent;
        chance = (chance + 50u) / 100u;
        chance = (chance * scalePercent + 50u) / 100u;
        if (context.VisualHierarchy.InfluenceEnabled)
        {
            if (context.VisualHierarchy.targets(ShipVisualAnchorType::WEAPONS))
            {
                chance = (chance * context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::WEAPONS) + 50u) / 100u;
                if (context.VisualHierarchy.isPrimary(ShipVisualAnchorType::WEAPONS)) { chance = std::max<uint64_t>(chance, 88u); }
            }
            else
            {
                chance = chance * context.VisualHierarchy.getCompetingFeaturePercent(ShipVisualAnchorType::WEAPONS) / 100u;
            }
        }
        return static_cast<uint32_t>(std::min<uint64_t>(100u, chance));
    }

    uint32_t WeaponGenerator::getMaximumWeaponGroups(const ShipGenerationContext& context) const
    {
        uint32_t scaleMaximumGroups = 1u + context.ScaleTraits.MajorFeatureCapacity / 50u;
        uint32_t profileMaximum = context.Profile.MaximumLargeWeaponGroups;
        if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.isPrimary(ShipVisualAnchorType::WEAPONS) && context.ScaleTraits.Tier >= GenerationScaleTier::MEDIUM)
        {
            scaleMaximumGroups = std::min(3u, scaleMaximumGroups + 1u);
            profileMaximum = std::min(3u, profileMaximum + 1u);
        }
        return std::min(profileMaximum, scaleMaximumGroups);
    }

    uint32_t WeaponGenerator::getSymmetryChance(const ShipGenerationContext& context, const FactionWeaponProfile& factionProfile, const WeaponHardpoint& hardpoint) const
    {
        int32_t chance = static_cast<int32_t>(context.Profile.LargeWeaponSymmetryChance) + factionProfile.SymmetryChanceOffset;

        if (hardpoint.Region == ShipWeaponHardpointRegion::OUTER_WING || hardpoint.Region == ShipWeaponHardpointRegion::WING_ROOT)
        {
            chance += 8;
        }

        return static_cast<uint32_t>(std::clamp(chance, 0, 100));
    }

    uint32_t WeaponGenerator::scaleWeaponPixelsFrom64(uint32_t value, uint32_t dimension, uint32_t scalePercent) const
    {
        const uint32_t scaled = GenerationMath::scalePixelsFrom64(value, dimension);
        return std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(scaled) * scalePercent + 50u) / 100u));
    }
}
