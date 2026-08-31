#include <SpectralShipGen/GenerationComplexityBudget.h>

#include <algorithm>
#include <array>
#include <cstdint>

namespace SpectralShipGen
{
    namespace
    {
        constexpr std::size_t categoryIndex(GenerationComplexityCategory category)
        {
            return static_cast<std::size_t>(category);
        }

        int32_t getCategoryOffset(const ShipFactionComplexityCategoryOffsets& offsets, GenerationComplexityCategory category)
        {
            switch (category)
            {
            case GenerationComplexityCategory::SILHOUETTE: return offsets.Silhouette;
            case GenerationComplexityCategory::COCKPIT_STRUCTURE: return offsets.CockpitStructure;
            case GenerationComplexityCategory::HULL_LAYER: return offsets.HullLayer;
            case GenerationComplexityCategory::MAJOR_FEATURE: return offsets.MajorFeature;
            case GenerationComplexityCategory::LARGE_WEAPON: return offsets.LargeWeapon;
            case GenerationComplexityCategory::ATTACHMENT: return offsets.Attachment;
            case GenerationComplexityCategory::DETAIL: return offsets.Detail;
            default: return 0;
            }
        }

        std::array<int32_t, GenerationComplexityBudget::CategoryCount> getCategoryWeights(const ShipGenerationProfile& profile, const ShipFactionComplexityProfile& factionProfile)
        {
            std::array<int32_t, GenerationComplexityBudget::CategoryCount> weights = profile.ComplexityCategoryWeights.toArray();
            const ShipFactionComplexityCategoryOffsets& offsets = factionProfile.CategoryOffsets;

            for (std::size_t index = 0u; index < weights.size(); ++index)
            {
                const GenerationComplexityCategory category = static_cast<GenerationComplexityCategory>(index);
                weights[index] += getCategoryOffset(offsets, category);
                weights[index] = std::max(1, weights[index]);
            }
            return weights;
        }
    }

    GenerationComplexityBudget GenerationComplexityBudget::create(const GenerationScaleTraits& scaleTraits, const ShipGenerationProfile& profile, const ShipFactionProfile& factionProfile, bool reserveCockpitStructure)
    {
        GenerationComplexityBudget budget;
        const uint32_t scaleCapacity = (scaleTraits.MajorFeatureCapacity * 2u + scaleTraits.DetailComplexity + scaleTraits.AttachmentComplexity + scaleTraits.SmallFeatureCapacity + 2u) / 5u;
        uint64_t totalBudget = 38u + (static_cast<uint64_t>(scaleCapacity) * 82u + 50u) / 100u;
        totalBudget = (totalBudget * profile.ComplexityBudgetPercent + 50u) / 100u;
        totalBudget = (totalBudget * factionProfile.Complexity.TotalBudgetPercent + 50u) / 100u;
        budget.m_InitialBudget = static_cast<uint32_t>(std::clamp<uint64_t>(totalBudget, 28u, 132u));

        const auto weights = getCategoryWeights(profile, factionProfile.Complexity);
        uint32_t totalWeight = 0u;
        for (int32_t weight : weights) { totalWeight += static_cast<uint32_t>(weight); }

        uint32_t allocated = 0u;
        for (std::size_t index = 0u; index < weights.size(); ++index)
        {
            budget.m_Allocations[index] = static_cast<uint32_t>((static_cast<uint64_t>(budget.m_InitialBudget) * static_cast<uint32_t>(weights[index])) / totalWeight);
            allocated += budget.m_Allocations[index];
        }

        for (std::size_t index = 0u; allocated < budget.m_InitialBudget; index = (index + 1u) % weights.size())
        {
            ++budget.m_Allocations[index];
            ++allocated;
        }

        return budget;
    }

    GenerationComplexityBudget GenerationComplexityBudget::create(const GenerationScaleTraits& scaleTraits, const ShipGenerationProfile& profile, ShipFactionType faction, bool reserveCockpitStructure)
    {
        return create(scaleTraits, profile, getShipFactionProfile(faction), reserveCockpitStructure);
    }

    GenerationComplexityBudget GenerationComplexityBudget::create(const GenerationScaleTraits& scaleTraits, ShipStyle style, ShipFactionType faction, bool reserveCockpitStructure)
    {
        return create(scaleTraits, getShipGenerationProfile(style), getShipFactionProfile(faction), reserveCockpitStructure);
    }

    bool GenerationComplexityBudget::canAfford(GenerationComplexityCategory category, uint32_t cost) const
    {
        if (cost == 0u) { return true; }
        if (category == GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END) { return false; }
        const uint32_t unused = getUnusedBudget();
        const uint32_t spendableUnused = unused > m_HierarchyHeldBudget ? unused - m_HierarchyHeldBudget : 0u;
        if (cost > spendableUnused) { return false; }
        return cost <= getCategoryAvailable(category);
    }

    bool GenerationComplexityBudget::tryConsume(GenerationComplexityCategory category, uint32_t cost)
    {
        if (!canAfford(category, cost)) { return false; }
        if (cost == 0u) { return true; }

        const std::size_t index = categoryIndex(category);
        const uint32_t categoryRemaining = m_ConsumedByCategory[index] < m_Allocations[index] ? m_Allocations[index] - m_ConsumedByCategory[index] : 0u;
        const uint32_t borrowed = cost > categoryRemaining ? cost - categoryRemaining : 0u;

        m_ConsumedByCategory[index] += cost;
        m_ConsumedBudget += cost;
        m_SharedReleasedBudget -= borrowed;
        return true;
    }

    void GenerationComplexityBudget::finalizeCategory(GenerationComplexityCategory category)
    {
        if (category == GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END) { return; }
        const std::size_t index = categoryIndex(category);
        if (m_Finalized[index]) { return; }

        if (m_ConsumedByCategory[index] < m_Allocations[index])
        {
            m_SharedReleasedBudget += m_Allocations[index] - m_ConsumedByCategory[index];
        }

        m_Finalized[index] = true;
    }

    uint32_t GenerationComplexityBudget::applyHierarchyReservation(GenerationComplexityCategory category, uint32_t amount)
    {
        if (amount == 0u || m_HierarchyReservationAmount != 0u) { return m_HierarchyReservationAmount; }

        constexpr std::array<GenerationComplexityCategory, CategoryCount> DonorOrder =
        {
            GenerationComplexityCategory::DETAIL,
            GenerationComplexityCategory::ATTACHMENT,
            GenerationComplexityCategory::MAJOR_FEATURE,
            GenerationComplexityCategory::LARGE_WEAPON,
            GenerationComplexityCategory::HULL_LAYER,
            GenerationComplexityCategory::COCKPIT_STRUCTURE,
            GenerationComplexityCategory::SILHOUETTE
        };

        const uint32_t requested = std::min(amount, m_InitialBudget / 5u);
        if (category == GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END)
        {
            m_HierarchyHeldBudget = requested;
            m_HierarchyReservationAmount = requested;
            return requested;
        }

        uint32_t remaining = requested;
        uint32_t transferred = 0u;
        for (GenerationComplexityCategory donor : DonorOrder)
        {
            if (remaining == 0u) { break; }
            if (donor == category) { continue; }
            const std::size_t donorIndex = categoryIndex(donor);
            const uint32_t minimumAllocation = m_ConsumedByCategory[donorIndex] + 1u;
            if (m_Allocations[donorIndex] <= minimumAllocation) { continue; }
            const uint32_t available = m_Allocations[donorIndex] - minimumAllocation;
            const uint32_t moved = std::min(remaining, available);
            m_Allocations[donorIndex] -= moved;
            remaining -= moved;
            transferred += moved;
        }

        m_Allocations[categoryIndex(category)] += transferred;
        m_HierarchyReservationAmount = transferred;
        return transferred;
    }

    uint32_t GenerationComplexityBudget::getCategoryAllocation(GenerationComplexityCategory category) const
    {
        if (category == GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END) { return 0u; }
        return m_Allocations[categoryIndex(category)];
    }

    uint32_t GenerationComplexityBudget::getCategoryConsumed(GenerationComplexityCategory category) const
    {
        if (category == GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END) { return 0u; }
        return m_ConsumedByCategory[categoryIndex(category)];
    }

    uint32_t GenerationComplexityBudget::getCategoryAvailable(GenerationComplexityCategory category) const
    {
        if (category == GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END) { return 0u; }
        const std::size_t index = categoryIndex(category);
        const uint32_t categoryRemaining = m_ConsumedByCategory[index] < m_Allocations[index] ? m_Allocations[index] - m_ConsumedByCategory[index] : 0u;
        return categoryRemaining + m_SharedReleasedBudget;
    }

    const char* getGenerationComplexityCategoryName(GenerationComplexityCategory category)
    {
        switch (category)
        {
        case GenerationComplexityCategory::SILHOUETTE: return "SILHOUETTE";
        case GenerationComplexityCategory::COCKPIT_STRUCTURE: return "COCKPIT_STRUCTURE";
        case GenerationComplexityCategory::HULL_LAYER: return "HULL_LAYER";
        case GenerationComplexityCategory::MAJOR_FEATURE: return "MAJOR_FEATURE";
        case GenerationComplexityCategory::LARGE_WEAPON: return "LARGE_WEAPON";
        case GenerationComplexityCategory::ATTACHMENT: return "ATTACHMENT";
        case GenerationComplexityCategory::DETAIL: return "DETAIL";
        default: return "UNKNOWN";
        }
    }
}
