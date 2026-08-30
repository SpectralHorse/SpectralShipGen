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

        // Backward-compatible built-in preset API. Style is resolved to a
        // ShipGenerationProfile and then routed through the same canonical
        // resolved-profile implementation as explicit/custom generation.
        GeneratedShip generate(const ShipGenerationSettings& settings);
        GeneratedShip generate(const ShipGenerationSettings& settings, ShipGenerationDebugInfo* debugInfo);
        GeneratedShip generate(const ShipGenerationSettings& settings, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo);
        GeneratedShip generateCalibrated(const ShipGenerationSettings& settings, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo = nullptr);

        // First-class explicit-profile API. ShipGenerationConfiguration contains
        // no built-in style identity; custom output therefore carries
        // ShipStyle::SHIP_STYLE_END provenance unless generated via the preset
        // convenience API above.
        GeneratedShip generate(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, ShipGenerationDebugInfo* debugInfo = nullptr, ShipGenerationPerformanceInfo* performanceInfo = nullptr);
        GeneratedShip generateCalibrated(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo = nullptr);

    private:
        GeneratedShip generateInternal(const ShipGenerationConfiguration& configuration,
            const ShipGenerationProfile& profile,
            ShipStyle builtInStyleProvenance,
            const GenerationCalibrationSettings* calibrationSettings,
            ShipGenerationDebugInfo* debugInfo,
            ShipGenerationPerformanceInfo* performanceInfo);

        static constexpr uint32_t MaximumHullGenerationAttempts = 8u;
    };
}
