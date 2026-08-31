#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace SpectralShipGen
{
    enum class ShipGenerationPerformanceStage : uint32_t
    {
        SETUP_PLANNING = 0u,
        HULL_GENERATION,
        HULL_VALIDATION,
        MACRO_ASYMMETRY_PLANNING,
        COCKPIT,
        ENGINES,
        CENTRAL_CORE,
        HULL_LAYERS,
        MAJOR_FEATURES,
        WEAPONS,
        ATTACHMENTS,
        MATERIAL_COMPOSITION,
        LIVERY,
        DETAILS,
        PAINTING_COMPOSITION,
        SHIP_GENERATION_PERFORMANCE_STAGE_END
    };

    constexpr std::size_t ShipGenerationPerformanceStageCount = static_cast<std::size_t>(ShipGenerationPerformanceStage::SHIP_GENERATION_PERFORMANCE_STAGE_END);

    struct ShipGenerationPerformanceInfo
    {
        void reset()
        {
            TotalDurationNanoseconds = 0u;
            StageDurationNanoseconds.fill(0u);
        }

        uint64_t TotalDurationNanoseconds = 0u;
        std::array<uint64_t, ShipGenerationPerformanceStageCount> StageDurationNanoseconds = {};
    };

    const char* getShipGenerationPerformanceStageName(ShipGenerationPerformanceStage stage);
}
