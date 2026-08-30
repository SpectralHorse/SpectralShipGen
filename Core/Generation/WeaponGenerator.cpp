#include "WeaponGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "MacroAsymmetryPlanner.h"
#include "PixelMaskUtils.h"
#include "WeaponCandidateBuilder.h"
#include "WeaponCandidateValidator.h"
#include "WeaponGenerationInternal.h"
#include "ShipFactionProfile.h"
#include "WeaponHardpointPlanner.h"

namespace PixelShipGenerator
{
    using WeaponGenerationInternal::CandidateWeapon;
    using WeaponGenerationInternal::FactionWeaponProfile;
    using WeaponGenerationInternal::WeaponCandidateBuilder;
    using WeaponGenerationInternal::WeaponCandidateValidationFailureReason;
    using WeaponGenerationInternal::WeaponCandidateValidator;
    using WeaponGenerationInternal::WeaponHardpoint;
    using WeaponGenerationInternal::WeaponHardpointPlanner;

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

        uint32_t getPairedWeaponComplexityCost(ShipWeaponType type)
        {
            const uint32_t singleCost = getWeaponComplexityCost(type);
            return singleCost + (singleCost + 1u) / 2u;
        }

        constexpr uint32_t MaximumWeaponPlacementAttempts = 18u;


        uint32_t getStyleWeaponWeight(const ShipGenerationProfile& profile, ShipWeaponType type);
        bool isWeaponTypeAllowedAtHardpoint(ShipWeaponType type, ShipWeaponHardpointRegion region);

        ShipWeaponType selectWeaponType(ShipGenerationContext& context, const FactionWeaponProfile& factionProfile, ShipWeaponHardpointRegion region, bool allowForcedType)
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

        uint32_t getStyleWeaponWeight(const ShipGenerationProfile& profile, ShipWeaponType type)
        {
            return profile.LargeWeaponWeights.getWeight(type);
        }

        bool isWeaponTypeAllowedAtHardpoint(ShipWeaponType type, ShipWeaponHardpointRegion region)
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

        void commitCandidate(ShipGenerationContext& context, CandidateWeapon& candidate, uint32_t symmetryGroup)
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

        void recordValidationFailure(ShipGenerationDebugInfo* debugInfo, WeaponCandidateValidationFailureReason reason)
        {
            if (debugInfo == nullptr) { return; }

            switch (reason)
            {
            case WeaponCandidateValidationFailureReason::SEMANTIC_COLLISION:
            case WeaponCandidateValidationFailureReason::INVALID_ROOT_SUPPORT:
            case WeaponCandidateValidationFailureReason::MISSING_SEMANTIC_GEOMETRY:
            case WeaponCandidateValidationFailureReason::INVALID_MUZZLE_DIRECTION:
                ++debugInfo->WeaponSemanticCollisionFailureCount;
                break;
            case WeaponCandidateValidationFailureReason::DISCONNECTED_GEOMETRY:
                ++debugInfo->WeaponConnectivityFailureCount;
                break;
            case WeaponCandidateValidationFailureReason::FIRING_CLEARANCE:
                ++debugInfo->WeaponFiringClearanceFailureCount;
                break;
            default:
                break;
            }
        }

        FactionWeaponProfile getFactionWeaponProfile(ShipFactionType faction)
        {
            const ShipFactionWeaponProfile& source = getShipFactionProfile(faction).Weapons;
            FactionWeaponProfile profile;
            profile.ChancePercent = source.ChancePercent;
            profile.SymmetryChanceOffset = source.SymmetryChanceOffset;
            profile.EmissiveChance = source.EmissiveChance;
            for (std::size_t index = 0u; index < profile.WeightMultipliers.size(); ++index)
            {
                profile.WeightMultipliers[index] = source.WeightMultipliersPercent.getWeightPercent(static_cast<ShipWeaponType>(index));
            }
            return profile;
        }

        uint32_t getGenerationChance(const ShipGenerationContext& context, const FactionWeaponProfile& factionProfile, bool hasValidOpportunity)
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
                    if (context.VisualHierarchy.isPrimary(ShipVisualAnchorType::WEAPONS) && hasValidOpportunity) { chance = 100u; }
                }
                else
                {
                    chance = chance * context.VisualHierarchy.getCompetingFeaturePercent(ShipVisualAnchorType::WEAPONS) / 100u;
                }
            }

            if (hasValidOpportunity && capacity > 20u && !context.VisualHierarchy.isPrimary(ShipVisualAnchorType::WEAPONS))
            {
                chance += std::min<uint64_t>(16u, context.Profile.LargeWeaponChance / 6u);
            }

            return static_cast<uint32_t>(std::min<uint64_t>(100u, chance));
        }

        uint32_t getMaximumWeaponGroups(const ShipGenerationContext& context)
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

        uint32_t getSymmetryChance(const ShipGenerationContext& context, const FactionWeaponProfile& factionProfile, const WeaponHardpoint& hardpoint)
        {
            int32_t chance = static_cast<int32_t>(context.Profile.LargeWeaponSymmetryChance) + factionProfile.SymmetryChanceOffset;

            if (hardpoint.Region == ShipWeaponHardpointRegion::OUTER_WING || hardpoint.Region == ShipWeaponHardpointRegion::WING_ROOT)
            {
                chance += 8;
            }

            return static_cast<uint32_t>(std::clamp(chance, 0, 100));
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
            context.DebugInfo->WeaponRequestedGroupCount = 0u;
            context.DebugInfo->WeaponRealizedGroupCount = 0u;
            context.DebugInfo->WeaponPlacementAttemptCount = 0u;
            context.DebugInfo->WeaponPlacementRejectionCount = 0u;
            context.DebugInfo->WeaponGenerationChanceSkipCount = 0u;
            context.DebugInfo->WeaponNoHardpointFailureCount = 0u;
            context.DebugInfo->WeaponTypeSelectionFailureCount = 0u;
            context.DebugInfo->WeaponCandidateGeometryFailureCount = 0u;
            context.DebugInfo->WeaponSemanticCollisionFailureCount = 0u;
            context.DebugInfo->WeaponConnectivityFailureCount = 0u;
            context.DebugInfo->WeaponFiringClearanceFailureCount = 0u;
            context.DebugInfo->WeaponSymmetryPairFailureCount = 0u;
            context.DebugInfo->WeaponSpatialBudgetRejectionCount = 0u;
            context.DebugInfo->WeaponComplexityBudgetRejectionCount = 0u;
            context.DebugInfo->WeaponCount = 0u;
            context.DebugInfo->WeaponPixelCount = 0u;
            context.DebugInfo->WeaponMovablePixelCount = 0u;
            context.DebugInfo->WeaponCoveragePermille = 0u;
            context.DebugInfo->WeaponVisualAnchorOpportunity = false;
            context.DebugInfo->WeaponVisualAnchorRealized = false;
            context.DebugInfo->WeaponOccupiedMask = PixelMask(width, height, false);
            context.DebugInfo->WeaponTypeCounts.fill(0u);
            context.DebugInfo->WeaponUnits.clear();
        }

        if (width == 0u || height == 0u)
        {
            return;
        }

        const WeaponHardpointPlanner hardpointPlanner;
        const WeaponCandidateBuilder candidateBuilder;
        const WeaponCandidateValidator candidateValidator;
        const std::vector<WeaponHardpoint> hardpoints = hardpointPlanner.discoverHardpoints(context);

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->WeaponHardpointCount = static_cast<uint32_t>(hardpoints.size());
            context.DebugInfo->WeaponVisualAnchorOpportunity = context.VisualHierarchy.InfluenceEnabled &&
                context.VisualHierarchy.isPrimary(ShipVisualAnchorType::WEAPONS) && !hardpoints.empty();
        }

        if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::LARGE_WEAPON, 14u))
        {
            if (context.DebugInfo != nullptr) { ++context.DebugInfo->WeaponComplexityBudgetRejectionCount; }
            return;
        }

        const FactionWeaponProfile factionProfile = getFactionWeaponProfile(context.Settings.Faction);
        const uint32_t generationChance = getGenerationChance(context, factionProfile, !hardpoints.empty());
        const bool macroRequested = context.MacroAsymmetry.targets(MacroAsymmetryCategory::LARGE_WEAPON);

        if (context.CalibrationSettings != nullptr && context.CalibrationSettings->Overrides.ForcedLargeWeaponPresence.has_value())
        {
            const bool normallyGenerated = generationChance > 0u && context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < generationChance;
            (void)normallyGenerated;
            if (!*context.CalibrationSettings->Overrides.ForcedLargeWeaponPresence)
            {
                if (context.DebugInfo != nullptr) { ++context.DebugInfo->WeaponGenerationChanceSkipCount; }
                return;
            }
        }
        else if (!macroRequested && (generationChance == 0u || context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) >= generationChance))
        {
            if (context.DebugInfo != nullptr) { ++context.DebugInfo->WeaponGenerationChanceSkipCount; }
            return;
        }

        if (hardpoints.empty())
        {
            if (context.DebugInfo != nullptr) { ++context.DebugInfo->WeaponNoHardpointFailureCount; }
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
        if (context.DebugInfo != nullptr) { context.DebugInfo->WeaponRequestedGroupCount = targetGroups; }
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
                const WeaponHardpoint* hardpointPtr = hardpointPlanner.selectHardpoint(context, hardpoints, plannedAsymmetry);

                if (hardpointPtr == nullptr) { continue; }
                const WeaponHardpoint& hardpoint = *hardpointPtr;
                const ShipWeaponType type = selectWeaponType(context, factionProfile, hardpoint.Region, forcedTypePending);

                if (type == ShipWeaponType::SHIP_WEAPON_TYPE_END)
                {
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->WeaponPlacementRejectionCount;
                        ++context.DebugInfo->WeaponTypeSelectionFailureCount;
                    }

                    continue;
                }

                CandidateWeapon candidate(width, height);

                if (!candidateBuilder.generateCandidate(context, hardpoint, type, factionProfile, candidate))
                {
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->WeaponPlacementRejectionCount;
                        ++context.DebugInfo->WeaponCandidateGeometryFailureCount;
                    }
                    continue;
                }

                WeaponCandidateValidationFailureReason validationFailure = WeaponCandidateValidationFailureReason::NONE;
                if (!candidateValidator.validateCandidate(context, candidate, &validationFailure))
                {
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->WeaponPlacementRejectionCount;
                        recordValidationFailure(context.DebugInfo, validationFailure);
                    }
                    continue;
                }

                const bool wantsPair = !plannedAsymmetry && hardpoint.PairCapable && context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < getSymmetryChance(context, factionProfile, hardpoint);

                if (wantsPair)
                {
                    CandidateWeapon mirrored(width, height);
                    candidateBuilder.mirrorCandidate(candidate, mirrored, width);

                    WeaponCandidateValidationFailureReason pairFailure = WeaponCandidateValidationFailureReason::NONE;
                    if (candidateValidator.validateSymmetricPair(context, candidate, mirrored, &pairFailure))
                    {
                        auto spatialRegions = context.SpatialBudget.makeRegionSet(hardpointPlanner.getSpatialRegion(hardpoint.Region, hardpoint.X, width));
                        context.SpatialBudget.addRegion(spatialRegions, context.SpatialBudget.getMirroredRegion(hardpointPlanner.getSpatialRegion(hardpoint.Region, hardpoint.X, width)));
                        const uint32_t localCost = getWeaponComplexityCost(type);
                        const uint32_t spatialAcceptance = context.SpatialBudget.getPlacementAcceptancePercent(spatialRegions, localCost, true);
                        const bool spatialAccepted = spatialAcceptance > 0u && context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) < spatialAcceptance;
                        if (!spatialAccepted)
                        {
                            context.SpatialBudget.recordRejection(spatialRegions);
                            if (context.DebugInfo != nullptr)
                            {
                                ++context.DebugInfo->WeaponSymmetryPairFailureCount;
                                ++context.DebugInfo->WeaponSpatialBudgetRejectionCount;
                            }
                        }
                        else if (context.ComplexityBudget.tryConsume(GenerationComplexityCategory::LARGE_WEAPON, getPairedWeaponComplexityCost(type)))
                        {
                            context.SpatialBudget.consume(spatialRegions, localCost, true);
                            commitCandidate(context, candidate, nextSymmetryGroup);
                            commitCandidate(context, mirrored, nextSymmetryGroup);
                            ++nextSymmetryGroup;
                            forcedTypePending = false;
                            if (context.DebugInfo != nullptr) { ++context.DebugInfo->WeaponRealizedGroupCount; }
                            placed = true;
                            break;
                        }
                        else if (context.DebugInfo != nullptr)
                        {
                            ++context.DebugInfo->WeaponSymmetryPairFailureCount;
                            ++context.DebugInfo->WeaponComplexityBudgetRejectionCount;
                        }
                    }
                    else if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->WeaponSymmetryPairFailureCount;
                    }
                }

                const uint32_t localCost = getWeaponComplexityCost(type);
                if (plannedAsymmetry && !MacroAsymmetryPlanner::canAcceptCandidate(context, candidate.OccupiedMask, localCost))
                {
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->WeaponPlacementRejectionCount;
                        ++context.DebugInfo->WeaponSemanticCollisionFailureCount;
                    }
                    continue;
                }
                const auto spatialRegions = context.SpatialBudget.makeRegionSet(hardpointPlanner.getSpatialRegion(hardpoint.Region, hardpoint.X, width));
                const uint32_t spatialAcceptance = context.SpatialBudget.getPlacementAcceptancePercent(spatialRegions, localCost, true);
                if (spatialAcceptance == 0u || context.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 99u) >= spatialAcceptance)
                {
                    context.SpatialBudget.recordRejection(spatialRegions);
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->WeaponPlacementRejectionCount;
                        ++context.DebugInfo->WeaponSpatialBudgetRejectionCount;
                    }
                    continue;
                }

                if (!context.ComplexityBudget.tryConsume(GenerationComplexityCategory::LARGE_WEAPON, localCost))
                {
                    if (context.DebugInfo != nullptr) { ++context.DebugInfo->WeaponComplexityBudgetRejectionCount; }
                    break;
                }

                if (plannedAsymmetry) { MacroAsymmetryPlanner::fulfill(context, candidate.OccupiedMask, localCost); }
                context.SpatialBudget.consume(spatialRegions, localCost, true);
                commitCandidate(context, candidate, 0u);
                forcedTypePending = false;
                if (context.DebugInfo != nullptr) { ++context.DebugInfo->WeaponRealizedGroupCount; }
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
            const uint32_t hullPixels = PixelMaskUtils::getMaskPixelCount(context.Ship.HullMask);
            context.DebugInfo->WeaponCoveragePermille = hullPixels == 0u ? 0u : static_cast<uint32_t>((static_cast<uint64_t>(context.DebugInfo->WeaponPixelCount) * 1000u + hullPixels / 2u) / hullPixels);
            context.DebugInfo->WeaponVisualAnchorRealized = context.DebugInfo->WeaponVisualAnchorOpportunity && context.DebugInfo->WeaponCount > 0u;
            context.DebugInfo->WeaponOccupiedMask = context.Weapons.OccupiedMask;
        }
    }
}
