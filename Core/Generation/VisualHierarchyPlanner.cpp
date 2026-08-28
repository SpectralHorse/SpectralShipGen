#include "VisualHierarchyPlanner.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>

#include "PixelMaskUtils.h"
#include "ShipGenerationSeeds.h"

namespace PixelShipGenerator
{
    namespace
    {
        constexpr uint64_t PrimaryPlanSalt = 0x6D0F27BD11A95E73ull;
        constexpr uint64_t SecondaryPlanSalt = 0xA17C9D4E53B26F81ull;
        constexpr std::size_t AnchorCount = static_cast<std::size_t>(ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END);

        uint32_t tierReservation(GenerationScaleTier tier)
        {
            switch (tier)
            {
            case GenerationScaleTier::TINY: return 2u;
            case GenerationScaleTier::SMALL: return 4u;
            case GenerationScaleTier::MEDIUM: return 6u;
            case GenerationScaleTier::LARGE: return 8u;
            default: return 4u;
            }
        }

        uint32_t scaleWeightPercent(const GenerationScaleTraits& traits, ShipVisualAnchorType anchor)
        {
            if (traits.Tier == GenerationScaleTier::TINY)
            {
                switch (anchor)
                {
                case ShipVisualAnchorType::SILHOUETTE:
                case ShipVisualAnchorType::COCKPIT: return 135u;
                case ShipVisualAnchorType::ENGINES: return 105u;
                case ShipVisualAnchorType::WINGS: return traits.HorizontalCapacity > 0u ? 70u : 0u;
                default: return 0u;
                }
            }

            if (traits.Tier == GenerationScaleTier::SMALL)
            {
                switch (anchor)
                {
                case ShipVisualAnchorType::MACRO_ASYMMETRY: return 35u;
                case ShipVisualAnchorType::NEGATIVE_SPACE: return 65u;
                case ShipVisualAnchorType::HULL_LAYERS:
                case ShipVisualAnchorType::MAJOR_FEATURE: return 75u;
                default: return 100u;
                }
            }

            if (traits.Tier == GenerationScaleTier::LARGE)
            {
                switch (anchor)
                {
                case ShipVisualAnchorType::ENGINES:
                case ShipVisualAnchorType::HULL_LAYERS:
                case ShipVisualAnchorType::MAJOR_FEATURE:
                case ShipVisualAnchorType::CENTRAL_CORE: return 120u;
                default: return 100u;
                }
            }

            return 100u;
        }

        uint32_t randomUInt(std::mt19937_64& generator, uint32_t minimum, uint32_t maximum)
        {
            if (minimum == maximum) { return minimum; }
            std::uniform_int_distribution<uint32_t> distribution(minimum, maximum);
            return distribution(generator);
        }
    }

    void VisualHierarchyPlanner::createPlan(ShipGenerationContext& context) const
    {
        VisualHierarchyPlan plan;
        if (context.Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS)
        {
            plan.InfluenceEnabled = false;
            context.VisualHierarchy = plan;
            return;
        }

        if (context.Settings.Style == ShipStyle::SPEARHEAD)
        {
            plan.PrimaryAnchor = ShipVisualAnchorType::SILHOUETTE;
            plan.ReservedCategory = GenerationComplexityCategory::SILHOUETTE;
            plan.TargetRegion = GenerationSpatialRegion::MID_FUSELAGE;
            plan.ReservedComplexity = 0u;
            plan.InfluenceEnabled = false;
            context.VisualHierarchy = plan;
            return;
        }

        plan.PrimaryAnchor = selectPrimary(context, false);
        plan.SecondaryAnchor = selectSecondary(context, plan.PrimaryAnchor);
        plan.ReservedCategory = getReservedCategory(plan.PrimaryAnchor);
        plan.TargetRegion = getTargetRegion(context, plan.PrimaryAnchor);
        plan.ReservedComplexity = tierReservation(context.ScaleTraits.Tier);
        plan.PrimaryInfluencePercent = context.ScaleTraits.Tier == GenerationScaleTier::TINY ? 140u : 160u;
        plan.SecondaryInfluencePercent = 122u;
        context.VisualHierarchy = plan;
    }

    void VisualHierarchyPlanner::resolveAfterHull(ShipGenerationContext& context) const
    {
        if (!context.VisualHierarchy.InfluenceEnabled || context.Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS)
        {
            return;
        }

        const ShipVisualAnchorType previous = context.VisualHierarchy.PrimaryAnchor;
        if (!isAnchorFeasible(context, previous, true))
        {
            if (previous == ShipVisualAnchorType::WINGS || previous == ShipVisualAnchorType::NEGATIVE_SPACE)
            {
                context.VisualHierarchy.PrimaryAnchor = ShipVisualAnchorType::SILHOUETTE;
            }
            else
            {
                context.VisualHierarchy.PrimaryAnchor = selectPrimary(context, true, previous);
            }
            context.VisualHierarchy.FallbackOccurred = context.VisualHierarchy.PrimaryAnchor != previous;
        }

        if (!isAnchorFeasible(context, context.VisualHierarchy.SecondaryAnchor, true) || context.VisualHierarchy.SecondaryAnchor == context.VisualHierarchy.PrimaryAnchor)
        {
            context.VisualHierarchy.SecondaryAnchor = ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END;
        }

        context.VisualHierarchy.ReservedCategory = getReservedCategory(context.VisualHierarchy.PrimaryAnchor);
        context.VisualHierarchy.TargetRegion = getTargetRegion(context, context.VisualHierarchy.PrimaryAnchor);
    }

    void VisualHierarchyPlanner::applySpatialPreference(ShipGenerationContext& context) const
    {
        if (!context.VisualHierarchy.InfluenceEnabled || context.VisualHierarchy.TargetRegion == GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END)
        {
            return;
        }
        context.SpatialBudget.applyHierarchyPreference(context.VisualHierarchy.TargetRegion, 128u);
    }

    GenerationComplexityCategory VisualHierarchyPlanner::getReservedCategory(ShipVisualAnchorType anchor)
    {
        switch (anchor)
        {
        case ShipVisualAnchorType::SILHOUETTE:
        case ShipVisualAnchorType::WINGS:
        case ShipVisualAnchorType::NEGATIVE_SPACE: return GenerationComplexityCategory::SILHOUETTE;
        case ShipVisualAnchorType::COCKPIT: return GenerationComplexityCategory::COCKPIT_STRUCTURE;
        case ShipVisualAnchorType::WEAPONS: return GenerationComplexityCategory::LARGE_WEAPON;
        case ShipVisualAnchorType::MAJOR_FEATURE: return GenerationComplexityCategory::MAJOR_FEATURE;
        case ShipVisualAnchorType::HULL_LAYERS:
        case ShipVisualAnchorType::CENTRAL_CORE: return GenerationComplexityCategory::HULL_LAYER;
        default: return GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END;
        }
    }

    GenerationSpatialRegion VisualHierarchyPlanner::getTargetRegion(const ShipGenerationContext& context, ShipVisualAnchorType anchor)
    {
        switch (anchor)
        {
        case ShipVisualAnchorType::COCKPIT: return GenerationSpatialRegion::FRONT_FUSELAGE;
        case ShipVisualAnchorType::ENGINES: return GenerationSpatialRegion::REAR_FUSELAGE;
        case ShipVisualAnchorType::CENTRAL_CORE:
        case ShipVisualAnchorType::MAJOR_FEATURE:
        case ShipVisualAnchorType::SILHOUETTE: return GenerationSpatialRegion::MID_FUSELAGE;
        case ShipVisualAnchorType::WINGS:
        case ShipVisualAnchorType::NEGATIVE_SPACE: return GenerationSpatialRegion::LEFT_WING_ROOT;
        case ShipVisualAnchorType::HULL_LAYERS:
            return context.Settings.Style == ShipStyle::DELTA || context.Settings.Style == ShipStyle::HEAVY
                ? GenerationSpatialRegion::LEFT_WING_ROOT : GenerationSpatialRegion::MID_FUSELAGE;
        case ShipVisualAnchorType::WEAPONS:
            return context.Settings.Style == ShipStyle::FIGHTER || context.Settings.Style == ShipStyle::DELTA
                ? GenerationSpatialRegion::LEFT_WING_ROOT : GenerationSpatialRegion::FRONT_FUSELAGE;
        default: return GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
        }
    }

    uint32_t VisualHierarchyPlanner::applyAnchorPercent(const ShipGenerationContext& context, ShipVisualAnchorType anchor, uint32_t value)
    {
        return static_cast<uint32_t>((static_cast<uint64_t>(value) * context.VisualHierarchy.getAnchorWeightPercent(anchor) + 50u) / 100u);
    }

    uint32_t VisualHierarchyPlanner::applyCompetingPercent(const ShipGenerationContext& context, ShipVisualAnchorType anchor, uint32_t value)
    {
        return static_cast<uint32_t>((static_cast<uint64_t>(value) * context.VisualHierarchy.getCompetingFeaturePercent(anchor) + 50u) / 100u);
    }

    bool VisualHierarchyPlanner::isAnchorFeasible(const ShipGenerationContext& context, ShipVisualAnchorType anchor, bool hullAvailable) const
    {
        if (anchor == ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END) { return false; }
        switch (anchor)
        {
        case ShipVisualAnchorType::SILHOUETTE:
        case ShipVisualAnchorType::COCKPIT:
        case ShipVisualAnchorType::ENGINES: return true;
        case ShipVisualAnchorType::WINGS:
            return hullAvailable ? context.WingRegions.hasWings() : context.ScaleTraits.HorizontalCapacity > 0u;
        case ShipVisualAnchorType::WEAPONS:
            return context.ScaleTraits.MajorFeatureCapacity >= 10u && context.Profile.LargeWeaponChance > 0u;
        case ShipVisualAnchorType::MAJOR_FEATURE:
            return context.ScaleTraits.MajorFeatureCapacity >= 16u && context.Profile.MajorFeatureChance > 0u;
        case ShipVisualAnchorType::HULL_LAYERS:
        case ShipVisualAnchorType::CENTRAL_CORE:
            return context.ScaleTraits.MinimumDimension >= 32u;
        case ShipVisualAnchorType::MACRO_ASYMMETRY:
            return context.ScaleTraits.MajorFeatureCapacity >= 12u && context.Profile.MacroAsymmetryChance > 0u;
        case ShipVisualAnchorType::NEGATIVE_SPACE:
            return hullAvailable ? !context.StructuralNegativeSpace.Placements.empty()
                                 : context.ScaleTraits.MinimumDimension >= 32u && context.Profile.StructuralNegativeSpaceChance > 0u;
        default: return false;
        }
    }

    ShipVisualAnchorType VisualHierarchyPlanner::selectPrimary(const ShipGenerationContext& context, bool hullAvailable, ShipVisualAnchorType excluded) const
    {
        std::array<uint64_t, AnchorCount> weights = {};
        uint64_t total = 0u;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END); ++index)
        {
            const ShipVisualAnchorType anchor = static_cast<ShipVisualAnchorType>(index);
            if (anchor == excluded || !isAnchorFeasible(context, anchor, hullAvailable)) { continue; }
            uint64_t weight = getStyleWeight(context, anchor);
            weight = weight * getFactionWeightPercent(context.Settings.Faction, anchor) / 100u;
            weight = weight * scaleWeightPercent(context.ScaleTraits, anchor) / 100u;
            weights[index] = weight;
            total += weight;
        }
        if (total == 0u) { return ShipVisualAnchorType::SILHOUETTE; }

        std::mt19937_64 random(getPlanningSeed(context, PrimaryPlanSalt));
        std::uniform_int_distribution<uint64_t> distribution(0u, total - 1u);
        uint64_t roll = distribution(random);
        for (uint32_t index = 0u; index < weights.size(); ++index)
        {
            if (roll < weights[index]) { return static_cast<ShipVisualAnchorType>(index); }
            roll -= weights[index];
        }
        return ShipVisualAnchorType::SILHOUETTE;
    }

    ShipVisualAnchorType VisualHierarchyPlanner::selectSecondary(const ShipGenerationContext& context, ShipVisualAnchorType primary) const
    {
        if (context.ScaleTraits.Tier == GenerationScaleTier::TINY || context.Profile.VisualSecondaryAnchorChance == 0u)
        {
            return ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END;
        }

        uint32_t chance = context.Profile.VisualSecondaryAnchorChance;
        if (context.ScaleTraits.Tier == GenerationScaleTier::SMALL) { chance = chance * 3u / 4u; }
        else if (context.ScaleTraits.Tier == GenerationScaleTier::LARGE) { chance = std::min(70u, chance + 12u); }
        std::mt19937_64 random(getPlanningSeed(context, SecondaryPlanSalt));
        if (randomUInt(random, 0u, 99u) >= chance) { return ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END; }

        std::array<uint64_t, AnchorCount> weights = {};
        uint64_t total = 0u;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END); ++index)
        {
            const ShipVisualAnchorType anchor = static_cast<ShipVisualAnchorType>(index);
            if (anchor == primary || !isAnchorFeasible(context, anchor, false)) { continue; }
            uint64_t weight = getStyleWeight(context, anchor);
            weight = weight * getFactionWeightPercent(context.Settings.Faction, anchor) / 100u;
            weight = weight * scaleWeightPercent(context.ScaleTraits, anchor) / 100u;
            weights[index] = weight;
            total += weight;
        }
        if (total == 0u) { return ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END; }
        std::uniform_int_distribution<uint64_t> distribution(0u, total - 1u);
        uint64_t roll = distribution(random);
        for (uint32_t index = 0u; index < weights.size(); ++index)
        {
            if (roll < weights[index]) { return static_cast<ShipVisualAnchorType>(index); }
            roll -= weights[index];
        }
        return ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END;
    }

    uint32_t VisualHierarchyPlanner::getStyleWeight(const ShipGenerationContext& context, ShipVisualAnchorType anchor) const
    {
        return context.Profile.VisualAnchorWeights.getWeight(anchor);
    }

    uint32_t VisualHierarchyPlanner::getFactionWeightPercent(ShipFactionType faction, ShipVisualAnchorType anchor) const
    {
        switch (faction)
        {
        case ShipFactionType::FRONTIER:
            if (anchor == ShipVisualAnchorType::NEGATIVE_SPACE || anchor == ShipVisualAnchorType::ENGINES || anchor == ShipVisualAnchorType::MACRO_ASYMMETRY) { return 125u; }
            if (anchor == ShipVisualAnchorType::CENTRAL_CORE) { return 80u; }
            break;
        case ShipFactionType::MILITARY:
            if (anchor == ShipVisualAnchorType::WEAPONS || anchor == ShipVisualAnchorType::HULL_LAYERS || anchor == ShipVisualAnchorType::WINGS) { return 125u; }
            if (anchor == ShipVisualAnchorType::MACRO_ASYMMETRY) { return 70u; }
            break;
        case ShipFactionType::ASCENDANT:
            if (anchor == ShipVisualAnchorType::COCKPIT || anchor == ShipVisualAnchorType::CENTRAL_CORE || anchor == ShipVisualAnchorType::SILHOUETTE) { return 125u; }
            if (anchor == ShipVisualAnchorType::NEGATIVE_SPACE) { return 90u; }
            break;
        case ShipFactionType::XENO:
            if (anchor == ShipVisualAnchorType::SILHOUETTE || anchor == ShipVisualAnchorType::NEGATIVE_SPACE || anchor == ShipVisualAnchorType::MAJOR_FEATURE) { return 120u; }
            break;
        case ShipFactionType::CORPORATE:
            if (anchor == ShipVisualAnchorType::COCKPIT || anchor == ShipVisualAnchorType::HULL_LAYERS || anchor == ShipVisualAnchorType::ENGINES) { return 120u; }
            if (anchor == ShipVisualAnchorType::MACRO_ASYMMETRY) { return 65u; }
            break;
        case ShipFactionType::RELIC:
            if (anchor == ShipVisualAnchorType::CENTRAL_CORE || anchor == ShipVisualAnchorType::HULL_LAYERS || anchor == ShipVisualAnchorType::MAJOR_FEATURE) { return 135u; }
            if (anchor == ShipVisualAnchorType::WEAPONS) { return 90u; }
            break;
        default: break;
        }
        return 100u;
    }

    uint64_t VisualHierarchyPlanner::getPlanningSeed(const ShipGenerationContext& context, uint64_t salt) const
    {
        return mixGenerationSeed64(context.DomainSeeds.get(GenerationDomain::HULL) ^ salt);
    }
}
