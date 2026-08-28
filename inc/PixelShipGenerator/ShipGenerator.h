#pragma once

#include <cstdint>

#include "GeneratedShip.h"
#include "GenerationTuningProfile.h"
#include "ShipGenerationDebugInfo.h"
#include "ShipGenerationSettings.h"
#include "ShipGenerationPerformance.h"

namespace PixelShipGenerator
{
    class ShipGenerator
    {
    public:
        ShipGenerator();

        GeneratedShip generate(const ShipGenerationSettings& settings);
        GeneratedShip generate(const ShipGenerationSettings& settings, ShipGenerationDebugInfo* debugInfo);
        GeneratedShip generate(const ShipGenerationSettings& settings, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo);
        GeneratedShip generateCalibrated(const ShipGenerationSettings& settings, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo = nullptr);

    private:
        GeneratedShip generateInternal(const ShipGenerationSettings& settings, const GenerationCalibrationSettings* calibrationSettings, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo);

        static constexpr uint32_t MaximumHullGenerationAttempts = 8u;
    };
}
