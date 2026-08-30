#include "GenerationComplexityBudget.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace PixelShipGenerator
{
    namespace
    {
        constexpr std::size_t categoryIndex(GenerationComplexityCategory category)
        {
            return static_cast<std::size_t>(category);
        }

        uint32_t getFactionBudgetPercent(ShipFactionType faction)
        {
            switch (faction)
            {
            case ShipFactionType::FRONTIER: return 106u;
            case ShipFactionType::MILITARY: return 98u;
            case ShipFactionType::ASCENDANT: return 92u;
            case ShipFactionType::XENO: return 102u;
            case ShipFactionType::CORPORATE: return 98u;
            case ShipFactionType::RELIC: return 104u;
            default: return 100u;
            }
        }

        std::array<int32_t, GenerationComplexityBudget::CategoryCount> getCategoryWeights(const ShipGenerationProfile& profile, ShipFactionType faction, bool reserveCockpitStructure)
        {
            if (!reserveCockpitStructure)
            {
                std::array<int32_t, GenerationComplexityBudget::CategoryCount> legacyWeights = profile.LegacyComplexityCategoryWeights.toArray();

                switch (faction)
                {
                case ShipFactionType::FRONTIER:
                    legacyWeights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 2;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::ATTACHMENT)] += 3;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::DETAIL)] += 2;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::MAJOR_FEATURE)] -= 2;
                    break;
                case ShipFactionType::MILITARY:
                    legacyWeights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 2;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::LARGE_WEAPON)] += 2;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::MAJOR_FEATURE)] += 1;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::DETAIL)] -= 2;
                    break;
                case ShipFactionType::ASCENDANT:
                    legacyWeights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 1;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::MAJOR_FEATURE)] += 3;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::SILHOUETTE)] -= 2;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::ATTACHMENT)] -= 2;
                    break;
                case ShipFactionType::XENO:
                    legacyWeights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 1;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::MAJOR_FEATURE)] += 1;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::ATTACHMENT)] += 1;
                    break;
                case ShipFactionType::CORPORATE:
                    legacyWeights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 2;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::LARGE_WEAPON)] += 1;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::DETAIL)] += 1;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::ATTACHMENT)] -= 1;
                    break;
                case ShipFactionType::RELIC:
                    legacyWeights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 4;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::MAJOR_FEATURE)] += 3;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::DETAIL)] -= 3;
                    legacyWeights[categoryIndex(GenerationComplexityCategory::ATTACHMENT)] -= 2;
                    break;
                default: break;
                }
                for (std::size_t index = 0u; index < legacyWeights.size(); ++index)
                {
                    if (index == categoryIndex(GenerationComplexityCategory::COCKPIT_STRUCTURE)) { continue; }
                    legacyWeights[index] = std::max(1, legacyWeights[index]);
                }
                return legacyWeights;
            }

            std::array<int32_t, GenerationComplexityBudget::CategoryCount> weights = profile.ComplexityCategoryWeights.toArray();

            switch (faction)
            {
            case ShipFactionType::FRONTIER:
                weights[categoryIndex(GenerationComplexityCategory::COCKPIT_STRUCTURE)] += 1;
                weights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 2;
                weights[categoryIndex(GenerationComplexityCategory::ATTACHMENT)] += 3;
                weights[categoryIndex(GenerationComplexityCategory::DETAIL)] += 2;
                weights[categoryIndex(GenerationComplexityCategory::MAJOR_FEATURE)] -= 2;
                break;
            case ShipFactionType::MILITARY:
                weights[categoryIndex(GenerationComplexityCategory::COCKPIT_STRUCTURE)] += 2;
                weights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 2;
                weights[categoryIndex(GenerationComplexityCategory::LARGE_WEAPON)] += 2;
                weights[categoryIndex(GenerationComplexityCategory::MAJOR_FEATURE)] += 1;
                weights[categoryIndex(GenerationComplexityCategory::DETAIL)] -= 2;
                break;
            case ShipFactionType::ASCENDANT:
                weights[categoryIndex(GenerationComplexityCategory::COCKPIT_STRUCTURE)] += 2;
                weights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 1;
                weights[categoryIndex(GenerationComplexityCategory::MAJOR_FEATURE)] += 3;
                weights[categoryIndex(GenerationComplexityCategory::SILHOUETTE)] -= 2;
                weights[categoryIndex(GenerationComplexityCategory::ATTACHMENT)] -= 2;
                break;
            case ShipFactionType::XENO:
                weights[categoryIndex(GenerationComplexityCategory::COCKPIT_STRUCTURE)] += 1;
                weights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 1;
                weights[categoryIndex(GenerationComplexityCategory::MAJOR_FEATURE)] += 1;
                weights[categoryIndex(GenerationComplexityCategory::ATTACHMENT)] += 1;
                break;
            case ShipFactionType::CORPORATE:
                weights[categoryIndex(GenerationComplexityCategory::COCKPIT_STRUCTURE)] += 2;
                weights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 2;
                weights[categoryIndex(GenerationComplexityCategory::LARGE_WEAPON)] += 1;
                weights[categoryIndex(GenerationComplexityCategory::DETAIL)] += 1;
                weights[categoryIndex(GenerationComplexityCategory::ATTACHMENT)] -= 1;
                break;
            case ShipFactionType::RELIC:
                weights[categoryIndex(GenerationComplexityCategory::COCKPIT_STRUCTURE)] += 3;
                weights[categoryIndex(GenerationComplexityCategory::HULL_LAYER)] += 5;
                weights[categoryIndex(GenerationComplexityCategory::MAJOR_FEATURE)] += 4;
                weights[categoryIndex(GenerationComplexityCategory::DETAIL)] -= 4;
                weights[categoryIndex(GenerationComplexityCategory::ATTACHMENT)] -= 2;
                break;
            default:
                break;
            }

            for (int32_t& weight : weights)
            {
                weight = std::max(1, weight);
            }

            return weights;
        }
    }

    GenerationComplexityBudget GenerationComplexityBudget::create(const GenerationScaleTraits& scaleTraits, const ShipGenerationProfile& profile, ShipFactionType faction, bool reserveCockpitStructure)
    {
        GenerationComplexityBudget budget;
        const uint32_t scaleCapacity = (scaleTraits.MajorFeatureCapacity * 2u + scaleTraits.DetailComplexity + scaleTraits.AttachmentComplexity + scaleTraits.SmallFeatureCapacity + 2u) / 5u;
        uint64_t totalBudget = 38u + (static_cast<uint64_t>(scaleCapacity) * 82u + 50u) / 100u;
        totalBudget = (totalBudget * profile.ComplexityBudgetPercent + 50u) / 100u;
        totalBudget = (totalBudget * getFactionBudgetPercent(faction) + 50u) / 100u;
        budget.m_InitialBudget = static_cast<uint32_t>(std::clamp<uint64_t>(totalBudget, 28u, 132u));

        const auto weights = getCategoryWeights(profile, faction, reserveCockpitStructure);
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

    GenerationComplexityBudget GenerationComplexityBudget::create(const GenerationScaleTraits& scaleTraits, ShipStyle style, ShipFactionType faction, bool reserveCockpitStructure)
    {
        return create(scaleTraits, getShipGenerationProfile(style), faction, reserveCockpitStructure);
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
