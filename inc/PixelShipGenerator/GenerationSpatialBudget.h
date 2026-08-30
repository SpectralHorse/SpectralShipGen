#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "GenerationScaleTraits.h"
#include "PixelMask.h"

namespace PixelShipGenerator
{
    enum class ShipStyle : uint32_t;
    struct ShipGenerationProfile;

    enum class GenerationSpatialRegion : uint32_t
    {
        NOSE = 0u,
        FRONT_FUSELAGE,
        MID_FUSELAGE,
        REAR_FUSELAGE,
        LEFT_WING_ROOT,
        RIGHT_WING_ROOT,
        LEFT_OUTER_WING,
        RIGHT_OUTER_WING,
        GENERATION_SPATIAL_REGION_END
    };

    struct GenerationSpatialRegionState
    {
        uint32_t AreaPixels = 0u;
        uint32_t Capacity = 0u;
        uint32_t Load = 0u;
        uint32_t DominantFeatureCount = 0u;
        uint32_t RejectionCount = 0u;
    };

    struct GenerationSpatialBudget
    {
        static constexpr std::size_t RegionCount = static_cast<std::size_t>(GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END);
        using RegionSet = std::array<bool, RegionCount>;

        void reset();
        void initialize(const PixelMask& hullMask, const PixelMask& wingMask, const PixelMask& wingRootMask, const PixelMask& outerWingMask, const GenerationScaleTraits& scaleTraits);
        void initialize(const PixelMask& hullMask, const PixelMask& wingMask, const PixelMask& wingRootMask, const PixelMask& outerWingMask, const GenerationScaleTraits& scaleTraits, const ShipGenerationProfile& profile);
        // Compatibility/preset convenience. Static generation should pass an already-resolved profile.
        void initialize(const PixelMask& hullMask, const PixelMask& wingMask, const PixelMask& wingRootMask, const PixelMask& outerWingMask, const GenerationScaleTraits& scaleTraits, ShipStyle style);

        GenerationSpatialRegion getRegionAt(uint32_t x, uint32_t y) const;
        GenerationSpatialRegion getMirroredRegion(GenerationSpatialRegion region) const;
        RegionSet makeRegionSet(GenerationSpatialRegion region) const;
        RegionSet collectRegions(const PixelMask& mask) const;
        RegionSet collectRegions(const std::vector<std::pair<uint32_t, uint32_t>>& pixels) const;
        void addRegion(RegionSet& regions, GenerationSpatialRegion region) const;
        void applyHierarchyPreference(GenerationSpatialRegion region, uint32_t capacityPercent);

        uint32_t getPlacementAcceptancePercent(const RegionSet& regions, uint32_t cost, bool dominant) const;
        uint32_t getDetailPreferencePercent(const RegionSet& regions, uint32_t cost) const;
        void consume(const RegionSet& regions, uint32_t cost, bool dominant);
        void recordRejection(const RegionSet& regions);

        uint32_t getRegionUtilizationPercent(GenerationSpatialRegion region) const;
        uint32_t getAverageUtilizationPercent() const;
        uint32_t getTotalRejectionCount() const;
        const std::array<GenerationSpatialRegionState, RegionCount>& getRegionStates() const { return m_Regions; }
        const std::vector<uint8_t>& getRegionMap() const { return m_RegionMap; }
        uint32_t getWidth() const { return m_Width; }
        uint32_t getHeight() const { return m_Height; }

    private:
        std::array<GenerationSpatialRegionState, RegionCount> m_Regions = {};
        std::vector<uint8_t> m_RegionMap;
        uint32_t m_Width = 0u;
        uint32_t m_Height = 0u;
        uint32_t m_RejectionDecisionCount = 0u;
    };

    const char* getGenerationSpatialRegionName(GenerationSpatialRegion region);
}
