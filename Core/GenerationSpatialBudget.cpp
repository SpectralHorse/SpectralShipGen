#include <SpectralShipGen/GenerationSpatialBudget.h>

#include <algorithm>
#include <cstdint>

#include <SpectralShipGen/ShipGenerationProfile.h>

namespace SpectralShipGen
{
    namespace
    {
        constexpr std::size_t regionIndex(GenerationSpatialRegion region)
        {
            return static_cast<std::size_t>(region);
        }

        uint32_t integerSqrt(uint32_t value)
        {
            uint32_t result = 0u;
            while ((result + 1u) <= value / (result + 1u)) { ++result; }
            return result;
        }

        int32_t getCapacityBias(GenerationSpatialRegion region)
        {
            switch (region)
            {
            case GenerationSpatialRegion::NOSE: return -1;
            case GenerationSpatialRegion::FRONT_FUSELAGE: return 2;
            case GenerationSpatialRegion::MID_FUSELAGE: return 5;
            case GenerationSpatialRegion::REAR_FUSELAGE: return 2;
            case GenerationSpatialRegion::LEFT_WING_ROOT:
            case GenerationSpatialRegion::RIGHT_WING_ROOT: return 1;
            case GenerationSpatialRegion::LEFT_OUTER_WING:
            case GenerationSpatialRegion::RIGHT_OUTER_WING: return 0;
            default: return 0;
            }
        }

        int32_t getProfileCapacityBias(const ShipSpatialCapacityBias& bias, GenerationSpatialRegion region)
        {
            switch (region)
            {
            case GenerationSpatialRegion::NOSE: return bias.Nose;
            case GenerationSpatialRegion::FRONT_FUSELAGE: return bias.FrontFuselage;
            case GenerationSpatialRegion::MID_FUSELAGE: return bias.MidFuselage;
            case GenerationSpatialRegion::REAR_FUSELAGE: return bias.RearFuselage;
            case GenerationSpatialRegion::LEFT_WING_ROOT:
            case GenerationSpatialRegion::RIGHT_WING_ROOT: return bias.WingRoot;
            case GenerationSpatialRegion::LEFT_OUTER_WING:
            case GenerationSpatialRegion::RIGHT_OUTER_WING: return bias.OuterWing;
            default: return 0;
            }
        }
    }

    void GenerationSpatialBudget::reset()
    {
        m_Regions = {};
        m_RegionMap.clear();
        m_Width = 0u;
        m_Height = 0u;
        m_RejectionDecisionCount = 0u;
    }

    void GenerationSpatialBudget::initialize(const PixelMask& hullMask, const PixelMask& wingMask, const PixelMask& wingRootMask, const PixelMask& outerWingMask, const GenerationScaleTraits& scaleTraits)
    {
        initialize(hullMask, wingMask, wingRootMask, outerWingMask, scaleTraits, ShipGenerationProfile());
    }

    void GenerationSpatialBudget::initialize(const PixelMask& hullMask, const PixelMask& wingMask, const PixelMask& wingRootMask, const PixelMask& outerWingMask, const GenerationScaleTraits& scaleTraits, const ShipGenerationProfile& profile)
    {
        reset();
        m_Width = hullMask.getWidth();
        m_Height = hullMask.getHeight();
        m_RegionMap.assign(static_cast<std::size_t>(m_Width) * m_Height, static_cast<uint8_t>(GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END));
        if (m_Width == 0u || m_Height == 0u) { return; }

        uint32_t minY = m_Height;
        uint32_t maxY = 0u;
        bool foundHull = false;
        for (uint32_t y = 0u; y < m_Height; ++y)
        {
            for (uint32_t x = 0u; x < m_Width; ++x)
            {
                if (!hullMask.get(x, y)) { continue; }
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
                foundHull = true;
            }
        }
        if (!foundHull) { return; }

        const uint32_t hullHeight = maxY - minY + 1u;
        const uint32_t centerX = m_Width / 2u;
        for (uint32_t y = 0u; y < m_Height; ++y)
        {
            for (uint32_t x = 0u; x < m_Width; ++x)
            {
                if (!hullMask.get(x, y)) { continue; }

                GenerationSpatialRegion region = GenerationSpatialRegion::MID_FUSELAGE;
                if (wingRootMask.isInBounds(x, y) && wingRootMask.get(x, y))
                {
                    region = x < centerX ? GenerationSpatialRegion::LEFT_WING_ROOT : GenerationSpatialRegion::RIGHT_WING_ROOT;
                }
                else if ((outerWingMask.isInBounds(x, y) && outerWingMask.get(x, y)) || (wingMask.isInBounds(x, y) && wingMask.get(x, y)))
                {
                    region = x < centerX ? GenerationSpatialRegion::LEFT_OUTER_WING : GenerationSpatialRegion::RIGHT_OUTER_WING;
                }
                else
                {
                    const uint32_t verticalPercent = hullHeight <= 1u ? 50u : ((y - minY) * 100u) / (hullHeight - 1u);
                    if (verticalPercent < 20u) { region = GenerationSpatialRegion::NOSE; }
                    else if (verticalPercent < 45u) { region = GenerationSpatialRegion::FRONT_FUSELAGE; }
                    else if (verticalPercent < 73u) { region = GenerationSpatialRegion::MID_FUSELAGE; }
                    else { region = GenerationSpatialRegion::REAR_FUSELAGE; }
                }

                m_RegionMap[static_cast<std::size_t>(y) * m_Width + x] = static_cast<uint8_t>(region);
                ++m_Regions[regionIndex(region)].AreaPixels;
            }
        }

        const uint32_t scaleBoost = scaleTraits.SmallFeatureCapacity / 14u;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END); ++index)
        {
            auto& state = m_Regions[index];
            if (state.AreaPixels == 0u) { continue; }
            const GenerationSpatialRegion region = static_cast<GenerationSpatialRegion>(index);
            const int32_t capacity = static_cast<int32_t>(7u + integerSqrt(state.AreaPixels) + scaleBoost) + getCapacityBias(region) + getProfileCapacityBias(profile.SpatialCapacityBias, region);
            state.Capacity = static_cast<uint32_t>(std::clamp(capacity, 10, 44));
        }
    }

    void GenerationSpatialBudget::initialize(const PixelMask& hullMask, const PixelMask& wingMask, const PixelMask& wingRootMask, const PixelMask& outerWingMask, const GenerationScaleTraits& scaleTraits, ShipStyle style)
    {
        initialize(hullMask, wingMask, wingRootMask, outerWingMask, scaleTraits, getShipGenerationProfile(style));
    }

    GenerationSpatialRegion GenerationSpatialBudget::getRegionAt(uint32_t x, uint32_t y) const
    {
        if (x >= m_Width || y >= m_Height || m_RegionMap.empty()) { return GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END; }
        return static_cast<GenerationSpatialRegion>(m_RegionMap[static_cast<std::size_t>(y) * m_Width + x]);
    }

    GenerationSpatialRegion GenerationSpatialBudget::getMirroredRegion(GenerationSpatialRegion region) const
    {
        switch (region)
        {
        case GenerationSpatialRegion::LEFT_WING_ROOT: return GenerationSpatialRegion::RIGHT_WING_ROOT;
        case GenerationSpatialRegion::RIGHT_WING_ROOT: return GenerationSpatialRegion::LEFT_WING_ROOT;
        case GenerationSpatialRegion::LEFT_OUTER_WING: return GenerationSpatialRegion::RIGHT_OUTER_WING;
        case GenerationSpatialRegion::RIGHT_OUTER_WING: return GenerationSpatialRegion::LEFT_OUTER_WING;
        default: return region;
        }
    }

    GenerationSpatialBudget::RegionSet GenerationSpatialBudget::makeRegionSet(GenerationSpatialRegion region) const
    {
        RegionSet result = {};
        addRegion(result, region);
        return result;
    }

    GenerationSpatialBudget::RegionSet GenerationSpatialBudget::collectRegions(const PixelMask& mask) const
    {
        RegionSet result = {};
        const uint32_t width = std::min(mask.getWidth(), m_Width);
        const uint32_t height = std::min(mask.getHeight(), m_Height);
        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (mask.get(x, y)) { addRegion(result, getRegionAt(x, y)); }
            }
        }
        return result;
    }

    GenerationSpatialBudget::RegionSet GenerationSpatialBudget::collectRegions(const std::vector<std::pair<uint32_t, uint32_t>>& pixels) const
    {
        RegionSet result = {};
        for (const auto& pixel : pixels) { addRegion(result, getRegionAt(pixel.first, pixel.second)); }
        return result;
    }

    void GenerationSpatialBudget::addRegion(RegionSet& regions, GenerationSpatialRegion region) const
    {
        if (region == GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END) { return; }
        regions[regionIndex(region)] = true;
    }

    void GenerationSpatialBudget::applyHierarchyPreference(GenerationSpatialRegion region, uint32_t capacityPercent)
    {
        if (region == GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END || capacityPercent == 100u) { return; }
        RegionSet preferred = makeRegionSet(region);
        addRegion(preferred, getMirroredRegion(region));
        for (uint32_t index = 0u; index < static_cast<uint32_t>(GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END); ++index)
        {
            if (!preferred[index] || m_Regions[index].Capacity == 0u) { continue; }
            const uint64_t scaled = static_cast<uint64_t>(m_Regions[index].Capacity) * capacityPercent;
            m_Regions[index].Capacity = static_cast<uint32_t>(std::min<uint64_t>(60u, (scaled + 50u) / 100u));
        }
    }

    uint32_t GenerationSpatialBudget::getPlacementAcceptancePercent(const RegionSet& regions, uint32_t cost, bool dominant) const
    {
        uint32_t acceptance = 100u;
        bool hasRegion = false;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END); ++index)
        {
            if (!regions[index]) { continue; }
            const auto& state = m_Regions[index];
            if (state.Capacity == 0u) { continue; }
            hasRegion = true;
            const uint32_t utilization = (state.Load * 100u) / state.Capacity;
            const uint32_t projected = ((state.Load + cost) * 100u) / state.Capacity;
            uint32_t regionAcceptance = 100u - std::min(65u, utilization * 55u / 100u);
            if (dominant && state.DominantFeatureCount > 0u) { regionAcceptance = std::min(regionAcceptance, 22u); }
            if (projected > 100u) { regionAcceptance = std::min(regionAcceptance, dominant ? 18u : 50u); }
            if (projected > (dominant ? 128u : 165u)) { regionAcceptance = dominant ? 0u : 28u; }
            acceptance = std::min(acceptance, regionAcceptance);
        }
        return hasRegion ? acceptance : 100u;
    }

    uint32_t GenerationSpatialBudget::getDetailPreferencePercent(const RegionSet& regions, uint32_t cost) const
    {
        const uint32_t base = getPlacementAcceptancePercent(regions, cost, false);
        const uint32_t averageUtilization = getAverageUtilizationPercent();
        uint32_t maximumUtilization = 0u;
        bool hasRegion = false;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END); ++index)
        {
            if (!regions[index] || m_Regions[index].Capacity == 0u) { continue; }
            hasRegion = true;
            maximumUtilization = std::max(maximumUtilization, (m_Regions[index].Load * 100u) / m_Regions[index].Capacity);
        }
        if (!hasRegion || maximumUtilization <= averageUtilization) { return 100u; }
        const uint32_t relativePenalty = std::min(35u, maximumUtilization - averageUtilization);
        return std::max(35u, std::min(base, 100u - relativePenalty));
    }

    void GenerationSpatialBudget::consume(const RegionSet& regions, uint32_t cost, bool dominant)
    {
        if (cost == 0u) { return; }
        for (uint32_t index = 0u; index < static_cast<uint32_t>(GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END); ++index)
        {
            if (!regions[index] || m_Regions[index].Capacity == 0u) { continue; }
            m_Regions[index].Load += cost;
            if (dominant) { ++m_Regions[index].DominantFeatureCount; }
        }
    }

    void GenerationSpatialBudget::recordRejection(const RegionSet& regions)
    {
        ++m_RejectionDecisionCount;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END); ++index)
        {
            if (regions[index]) { ++m_Regions[index].RejectionCount; }
        }
    }

    uint32_t GenerationSpatialBudget::getRegionUtilizationPercent(GenerationSpatialRegion region) const
    {
        if (region == GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END) { return 0u; }
        const auto& state = m_Regions[regionIndex(region)];
        return state.Capacity == 0u ? 0u : (state.Load * 100u) / state.Capacity;
    }

    uint32_t GenerationSpatialBudget::getAverageUtilizationPercent() const
    {
        uint32_t total = 0u;
        uint32_t count = 0u;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END); ++index)
        {
            if (m_Regions[index].Capacity == 0u) { continue; }
            total += (m_Regions[index].Load * 100u) / m_Regions[index].Capacity;
            ++count;
        }
        return count == 0u ? 0u : total / count;
    }

    uint32_t GenerationSpatialBudget::getTotalRejectionCount() const
    {
        return m_RejectionDecisionCount;
    }

    const char* getGenerationSpatialRegionName(GenerationSpatialRegion region)
    {
        switch (region)
        {
        case GenerationSpatialRegion::NOSE: return "NOSE";
        case GenerationSpatialRegion::FRONT_FUSELAGE: return "FRONT_FUSELAGE";
        case GenerationSpatialRegion::MID_FUSELAGE: return "MID_FUSELAGE";
        case GenerationSpatialRegion::REAR_FUSELAGE: return "REAR_FUSELAGE";
        case GenerationSpatialRegion::LEFT_WING_ROOT: return "LEFT_WING_ROOT";
        case GenerationSpatialRegion::RIGHT_WING_ROOT: return "RIGHT_WING_ROOT";
        case GenerationSpatialRegion::LEFT_OUTER_WING: return "LEFT_OUTER_WING";
        case GenerationSpatialRegion::RIGHT_OUTER_WING: return "RIGHT_OUTER_WING";
        default: return "UNKNOWN";
        }
    }
}
