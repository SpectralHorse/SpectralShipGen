#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <SpectralShipGen/GenerationScaleTraits.h>
#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipFactionType.h>
#include <SpectralShipGen/ShipGenerationProfile.h>

namespace SpectralShipGen
{
    enum class GenerationComplexityCategory : uint32_t
    {
        SILHOUETTE = 0u,
        COCKPIT_STRUCTURE,
        HULL_LAYER,
        MAJOR_FEATURE,
        LARGE_WEAPON,
        ATTACHMENT,
        DETAIL,
        GENERATION_COMPLEXITY_CATEGORY_END
    };

    struct GenerationComplexityBudget
    {
        static constexpr std::size_t CategoryCount = static_cast<std::size_t>(GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END);

        static GenerationComplexityBudget create(const GenerationScaleTraits& scaleTraits, const ShipGenerationProfile& profile, const ShipFactionProfile& factionProfile, bool reserveCockpitStructure = true);
        // Backward-compatible built-in faction convenience.
        static GenerationComplexityBudget create(const GenerationScaleTraits& scaleTraits, const ShipGenerationProfile& profile, ShipFactionType faction, bool reserveCockpitStructure = true);
        // Compatibility/preset convenience. Static generation should pass an already-resolved profile.
        static GenerationComplexityBudget create(const GenerationScaleTraits& scaleTraits, ShipStyle style, ShipFactionType faction, bool reserveCockpitStructure = true);

        bool canAfford(GenerationComplexityCategory category, uint32_t cost) const;
        bool tryConsume(GenerationComplexityCategory category, uint32_t cost);
        void finalizeCategory(GenerationComplexityCategory category);
        uint32_t applyHierarchyReservation(GenerationComplexityCategory category, uint32_t amount);

        uint32_t getInitialBudget() const { return m_InitialBudget; }
        uint32_t getConsumedBudget() const { return m_ConsumedBudget; }
        uint32_t getUnusedBudget() const { return m_InitialBudget - m_ConsumedBudget; }
        uint32_t getHierarchyReservedBudget() const { return m_HierarchyReservationAmount; }
        uint32_t getSharedReleasedBudget() const { return m_SharedReleasedBudget; }
        uint32_t getCategoryAllocation(GenerationComplexityCategory category) const;
        uint32_t getCategoryConsumed(GenerationComplexityCategory category) const;
        uint32_t getCategoryAvailable(GenerationComplexityCategory category) const;
        const std::array<uint32_t, CategoryCount>& getAllocations() const { return m_Allocations; }
        const std::array<uint32_t, CategoryCount>& getConsumedByCategory() const { return m_ConsumedByCategory; }

    private:
        uint32_t m_InitialBudget = 0u;
        uint32_t m_ConsumedBudget = 0u;
        uint32_t m_SharedReleasedBudget = 0u;
        uint32_t m_HierarchyHeldBudget = 0u;
        uint32_t m_HierarchyReservationAmount = 0u;
        std::array<uint32_t, CategoryCount> m_Allocations = {};
        std::array<uint32_t, CategoryCount> m_ConsumedByCategory = {};
        std::array<bool, CategoryCount> m_Finalized = {};
    };

    const char* getGenerationComplexityCategoryName(GenerationComplexityCategory category);
}
