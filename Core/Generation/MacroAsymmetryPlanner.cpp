#include "MacroAsymmetryPlanner.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "PixelMaskUtils.h"
#include "ShipGenerationSeeds.h"

namespace PixelShipGenerator
{
    namespace
    {
        constexpr uint64_t MacroAsymmetrySalt = 0x7F4A7C159E3779B9ull;

        uint32_t nextRoll(uint64_t& state, uint32_t maximumExclusive)
        {
            state = mixGenerationSeed64(state + 0x9E3779B97F4A7C15ull);
            return maximumExclusive == 0u ? 0u : static_cast<uint32_t>(state % maximumExclusive);
        }

        uint32_t sideLoad(const ShipGenerationContext& context, MacroAsymmetrySide side)
        {
            const auto& states = context.SpatialBudget.getRegionStates();
            const GenerationSpatialRegion root = side == MacroAsymmetrySide::LEFT ? GenerationSpatialRegion::LEFT_WING_ROOT : GenerationSpatialRegion::RIGHT_WING_ROOT;
            const GenerationSpatialRegion outer = side == MacroAsymmetrySide::LEFT ? GenerationSpatialRegion::LEFT_OUTER_WING : GenerationSpatialRegion::RIGHT_OUTER_WING;
            return states[static_cast<std::size_t>(root)].Load + states[static_cast<std::size_t>(outer)].Load;
        }

        uint32_t approximateLateralPercent(const PixelMask& mask)
        {
            const uint32_t pixelCount = PixelMaskUtils::getMaskPixelCount(mask);
            if (pixelCount == 0u || mask.getWidth() <= 1u) { return 0u; }
            const uint32_t center = mask.getWidth() / 2u;
            uint64_t weightedDistance = 0u;
            for (uint32_t y = 0u; y < mask.getHeight(); ++y)
            {
                for (uint32_t x = 0u; x < mask.getWidth(); ++x)
                {
                    if (!mask.get(x, y)) { continue; }
                    const uint32_t distance = x > center ? x - center : center - x;
                    weightedDistance += static_cast<uint64_t>(distance) * 100u / std::max(1u, center);
                }
            }
            return static_cast<uint32_t>(weightedDistance / pixelCount);
        }
    }

    void MacroAsymmetryPlanner::createPlan(ShipGenerationContext& context) const
    {
        context.MacroAsymmetry = MacroAsymmetryPlan();
        if (context.Profile.MacroAsymmetryChance == 0u || context.ScaleTraits.MajorFeatureCapacity < 12u) { return; }

        uint32_t chance = static_cast<uint32_t>((static_cast<uint64_t>(context.Profile.MacroAsymmetryChance) * context.FactionProfile.MacroAsymmetry.ChancePercent + 50u) / 100u);
        chance = static_cast<uint32_t>((static_cast<uint64_t>(chance) * (30u + context.ScaleTraits.MajorFeatureCapacity * 70u / 100u) + 50u) / 100u);
        if (context.VisualHierarchy.InfluenceEnabled)
        {
            if (context.VisualHierarchy.targets(ShipVisualAnchorType::MACRO_ASYMMETRY))
            {
                chance = chance * context.VisualHierarchy.getAnchorWeightPercent(ShipVisualAnchorType::MACRO_ASYMMETRY) / 100u;
                if (context.VisualHierarchy.isPrimary(ShipVisualAnchorType::MACRO_ASYMMETRY)) { chance = std::max(chance, 68u); }
            }
            else
            {
                chance = chance * context.VisualHierarchy.getCompetingFeaturePercent(ShipVisualAnchorType::MACRO_ASYMMETRY) / 100u;
            }
        }
        chance = std::min(context.VisualHierarchy.InfluenceEnabled ? 72u : 45u, chance);

        uint64_t legacyState = mixGenerationSeed64(context.Seeds.Structure ^ context.Seeds.Attachments ^ MacroAsymmetrySalt);
        auto roll = [&](uint32_t maximumExclusive) mutable -> uint32_t
            {
                if (maximumExclusive == 0u) { return 0u; }
                if (context.Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS)
                {
                    return nextRoll(legacyState, maximumExclusive);
                }
                return context.getGenerationRandomUInt(GenerationDomain::MACRO_ASYMMETRY, 0u, maximumExclusive - 1u);
            };
        if (roll(100u) >= chance) { return; }

        const auto weights = context.Profile.MacroAsymmetryCategoryWeights.toArray();
        uint32_t total = 0u;
        std::array<uint32_t, 3u> adjusted = weights;
        if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::HULL_LAYER, 10u)) { adjusted[0u] = 0u; }
        if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::LARGE_WEAPON, 14u)) { adjusted[1u] = 0u; }
        if (!context.Settings.AttachmentsEnabled || !context.ComplexityBudget.canAfford(GenerationComplexityCategory::ATTACHMENT, 6u)) { adjusted[2u] = 0u; }
        for (const uint32_t value : adjusted) { total += value; }
        if (total == 0u) { return; }

        uint32_t categoryRoll = roll(total);
        uint32_t categoryIndex = 0u;
        for (; categoryIndex < adjusted.size(); ++categoryIndex)
        {
            if (categoryRoll < adjusted[categoryIndex]) { break; }
            categoryRoll -= adjusted[categoryIndex];
        }
        if (categoryIndex >= adjusted.size()) { return; }

        context.MacroAsymmetry.Enabled = true;
        context.MacroAsymmetry.DominantSide = roll(2u) == 0u ? MacroAsymmetrySide::LEFT : MacroAsymmetrySide::RIGHT;
        context.MacroAsymmetry.Category = static_cast<MacroAsymmetryCategory>(categoryIndex);
        context.MacroAsymmetry.BalanceStrategy = roll(100u) < 70u ? MacroAsymmetryBalanceStrategy::OPPOSITE_SUBTLE_DETAIL : MacroAsymmetryBalanceStrategy::DISTRIBUTED_COUNTERWEIGHT;
        context.MacroAsymmetry.DesiredVisualWeight = 24u + context.ScaleTraits.MajorFeatureCapacity / 5u;
        context.MacroAsymmetry.DesiredVisualWeight = context.MacroAsymmetry.DesiredVisualWeight * context.Profile.MacroAsymmetryVisualWeightPercent / 100u;

        const bool preferOuter = context.WingRegions.hasWings() && context.ScaleTraits.HorizontalCapacity >= 30u && roll(100u) < context.Profile.MacroAsymmetryOuterRegionChance;
        if (preferOuter)
        {
            context.MacroAsymmetry.TargetRegion = context.MacroAsymmetry.isLeftSide() ? GenerationSpatialRegion::LEFT_OUTER_WING : GenerationSpatialRegion::RIGHT_OUTER_WING;
        }
        else if (context.WingRegions.hasWings() && roll(100u) < context.Profile.MacroAsymmetryWingRootRegionChance)
        {
            context.MacroAsymmetry.TargetRegion = context.MacroAsymmetry.isLeftSide() ? GenerationSpatialRegion::LEFT_WING_ROOT : GenerationSpatialRegion::RIGHT_WING_ROOT;
        }
        else
        {
            context.MacroAsymmetry.TargetRegion = roll(2u) == 0u ? GenerationSpatialRegion::FRONT_FUSELAGE : GenerationSpatialRegion::MID_FUSELAGE;
        }

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->MacroAsymmetryPlanned = true;
            context.DebugInfo->MacroAsymmetryDominantSide = context.MacroAsymmetry.DominantSide;
            context.DebugInfo->MacroAsymmetryFeatureCategory = context.MacroAsymmetry.Category;
            context.DebugInfo->MacroAsymmetryTargetRegion = context.MacroAsymmetry.TargetRegion;
            context.DebugInfo->MacroAsymmetryBalancingStrategy = context.MacroAsymmetry.BalanceStrategy;
            context.DebugInfo->MacroAsymmetryDesiredVisualWeight = context.MacroAsymmetry.DesiredVisualWeight;
        }
    }

    bool MacroAsymmetryPlanner::candidateMatchesDominantSide(const ShipGenerationContext& context, const PixelMask& mask)
    {
        if (!context.MacroAsymmetry.Enabled) { return true; }
        uint32_t dominant = 0u;
        uint32_t opposite = 0u;
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (!mask.get(x, y)) { continue; }
                if (context.MacroAsymmetry.isDominantX(x, mask.getWidth())) { ++dominant; }
                else { ++opposite; }
            }
        }
        return dominant > 0u && dominant * 4u >= (dominant + opposite) * 3u;
    }

    uint32_t MacroAsymmetryPlanner::calculateVisualWeight(const ShipGenerationContext& context, const PixelMask& mask, uint32_t complexityCost)
    {
        const uint32_t pixels = PixelMaskUtils::getMaskPixelCount(mask);
        uint32_t root = 0u;
        while ((root + 1u) <= pixels / (root + 1u)) { ++root; }
        const uint32_t lateral = approximateLateralPercent(mask);
        return complexityCost + root + lateral / 12u + context.ScaleTraits.MinimumDimension / 96u;
    }

    uint32_t MacroAsymmetryPlanner::calculateBalanceScore(const ShipGenerationContext& context, const PixelMask& mask, uint32_t complexityCost)
    {
        const uint32_t leftLoad = sideLoad(context, MacroAsymmetrySide::LEFT);
        const uint32_t rightLoad = sideLoad(context, MacroAsymmetrySide::RIGHT);
        const uint32_t visualWeight = calculateVisualWeight(context, mask, complexityCost);
        const uint32_t projectedLeft = leftLoad + (context.MacroAsymmetry.DominantSide == MacroAsymmetrySide::LEFT ? visualWeight : 0u);
        const uint32_t projectedRight = rightLoad + (context.MacroAsymmetry.DominantSide == MacroAsymmetrySide::RIGHT ? visualWeight : 0u);
        const uint32_t difference = projectedLeft > projectedRight ? projectedLeft - projectedRight : projectedRight - projectedLeft;
        const uint32_t allowance = std::max(20u, context.MacroAsymmetry.DesiredVisualWeight + context.ScaleTraits.HorizontalCapacity / 10u);
        return difference >= allowance * 2u ? 0u : 100u - (difference * 50u) / allowance;
    }

    bool MacroAsymmetryPlanner::canAcceptCandidate(const ShipGenerationContext& context, const PixelMask& mask, uint32_t complexityCost)
    {
        if (!candidateMatchesDominantSide(context, mask)) { return false; }
        const uint32_t visualWeight = calculateVisualWeight(context, mask, complexityCost);
        if (visualWeight > context.MacroAsymmetry.DesiredVisualWeight + 18u) { return false; }
        return calculateBalanceScore(context, mask, complexityCost) >= 42u;
    }

    void MacroAsymmetryPlanner::fulfill(ShipGenerationContext& context, const PixelMask& mask, uint32_t complexityCost)
    {
        context.MacroAsymmetry.Fulfilled = true;
        context.MacroAsymmetry.ActualVisualWeight = calculateVisualWeight(context, mask, complexityCost);
        context.MacroAsymmetry.BalanceScore = calculateBalanceScore(context, mask, complexityCost);
        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->MacroAsymmetryFulfilled = true;
            context.DebugInfo->MacroAsymmetryActualVisualWeight = context.MacroAsymmetry.ActualVisualWeight;
            context.DebugInfo->MacroAsymmetryBalanceScore = context.MacroAsymmetry.BalanceScore;
            context.DebugInfo->MacroAsymmetryMask = mask;
        }
    }

    void MacroAsymmetryPlanner::reject(ShipGenerationContext& context)
    {
        context.MacroAsymmetry.Rejected = true;
        context.MacroAsymmetry.Enabled = false;
        if (context.DebugInfo != nullptr) { context.DebugInfo->MacroAsymmetryRejected = true; }
    }

    GenerationSpatialRegion MacroAsymmetryPlanner::getOppositeRegion(const ShipGenerationContext& context)
    {
        return context.SpatialBudget.getMirroredRegion(context.MacroAsymmetry.TargetRegion);
    }

    const char* getMacroAsymmetrySideName(MacroAsymmetrySide side)
    {
        switch (side)
        {
        case MacroAsymmetrySide::LEFT: return "LEFT";
        case MacroAsymmetrySide::RIGHT: return "RIGHT";
        default: return "NONE";
        }
    }

    const char* getMacroAsymmetryCategoryName(MacroAsymmetryCategory category)
    {
        switch (category)
        {
        case MacroAsymmetryCategory::HULL_LAYER: return "HULL_LAYER";
        case MacroAsymmetryCategory::LARGE_WEAPON: return "LARGE_WEAPON";
        case MacroAsymmetryCategory::ATTACHMENT: return "ATTACHMENT";
        default: return "NONE";
        }
    }

    const char* getMacroAsymmetryBalanceStrategyName(MacroAsymmetryBalanceStrategy strategy)
    {
        switch (strategy)
        {
        case MacroAsymmetryBalanceStrategy::OPPOSITE_SUBTLE_DETAIL: return "OPPOSITE_SUBTLE_DETAIL";
        case MacroAsymmetryBalanceStrategy::DISTRIBUTED_COUNTERWEIGHT: return "DISTRIBUTED_COUNTERWEIGHT";
        default: return "NONE";
        }
    }
}
