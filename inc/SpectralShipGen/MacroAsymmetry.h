#pragma once

#include <cstdint>

#include <SpectralShipGen/GenerationSpatialBudget.h>

namespace SpectralShipGen
{
    enum class MacroAsymmetrySide : uint32_t
    {
        LEFT = 0u,
        RIGHT,
        MACRO_ASYMMETRY_SIDE_END
    };

    enum class MacroAsymmetryCategory : uint32_t
    {
        HULL_LAYER = 0u,
        LARGE_WEAPON,
        ATTACHMENT,
        MACRO_ASYMMETRY_CATEGORY_END
    };

    enum class MacroAsymmetryBalanceStrategy : uint32_t
    {
        OPPOSITE_SUBTLE_DETAIL = 0u,
        DISTRIBUTED_COUNTERWEIGHT,
        MACRO_ASYMMETRY_BALANCE_STRATEGY_END
    };

    struct MacroAsymmetryPlan
    {
        bool Enabled = false;
        bool Fulfilled = false;
        bool Rejected = false;
        MacroAsymmetrySide DominantSide = MacroAsymmetrySide::LEFT;
        MacroAsymmetryCategory Category = MacroAsymmetryCategory::MACRO_ASYMMETRY_CATEGORY_END;
        GenerationSpatialRegion TargetRegion = GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END;
        MacroAsymmetryBalanceStrategy BalanceStrategy = MacroAsymmetryBalanceStrategy::OPPOSITE_SUBTLE_DETAIL;
        uint32_t DesiredVisualWeight = 0u;
        uint32_t ActualVisualWeight = 0u;
        uint32_t BalanceScore = 100u;

        bool targets(MacroAsymmetryCategory category) const { return Enabled && !Fulfilled && !Rejected && Category == category; }
        bool isLeftSide() const { return DominantSide == MacroAsymmetrySide::LEFT; }
        bool isDominantX(uint32_t x, uint32_t width) const
        {
            return isLeftSide() ? x < width / 2u : x >= width / 2u;
        }
    };

    const char* getMacroAsymmetrySideName(MacroAsymmetrySide side);
    const char* getMacroAsymmetryCategoryName(MacroAsymmetryCategory category);
    const char* getMacroAsymmetryBalanceStrategyName(MacroAsymmetryBalanceStrategy strategy);
}
