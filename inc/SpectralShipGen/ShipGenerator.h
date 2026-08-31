#pragma once

#include <cstdint>

#include <SpectralShipGen/GeneratedShip.h>
#include <SpectralShipGen/GenerationTuningProfile.h>
#include <SpectralShipGen/ShipGenerationDebugInfo.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerationPerformance.h>
#include <SpectralShipGen/ShipGenerationRecipe.h>
#include <SpectralShipGen/ShipResolvedGenerationConfiguration.h>

namespace SpectralShipGen
{
    struct ShipFactionProfile;

    class ShipGenerator
    {
    public:
        ShipGenerator();

        // Canonical public resolved-configuration path. Built-in convenience,
        // explicit profile and portable recipe entry points all resolve into
        // this same semantic representation before generation.
        GeneratedShip generate(const ShipResolvedGenerationConfiguration& configuration, ShipGenerationDebugInfo* debugInfo = nullptr, ShipGenerationPerformanceInfo* performanceInfo = nullptr);
        GeneratedShip generateCalibrated(const ShipResolvedGenerationConfiguration& configuration, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo = nullptr);

        // Backward-compatible built-in preset API. Style is resolved to a
        // ShipGenerationProfile and then routed through the same canonical
        // resolved-profile implementation as explicit/custom generation.
        GeneratedShip generate(const ShipGenerationSettings& settings);
        GeneratedShip generate(const ShipGenerationSettings& settings, ShipGenerationDebugInfo* debugInfo);
        GeneratedShip generate(const ShipGenerationSettings& settings, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo);
        GeneratedShip generateCalibrated(const ShipGenerationSettings& settings, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo = nullptr);

        // Portable recipe entry path. Built-in recipe sources retain truthful
        // preset provenance; embedded custom sources use *_END provenance and
        // the same canonical resolved-profile implementation.
        GeneratedShip generate(const ShipGenerationRecipe& recipe, ShipGenerationDebugInfo* debugInfo = nullptr, ShipGenerationPerformanceInfo* performanceInfo = nullptr);

        // First-class explicit-profile API. ShipGenerationConfiguration contains
        // no built-in style identity; custom output therefore carries
        // ShipStyle::SHIP_STYLE_END provenance unless generated via the preset
        // convenience API above.
        GeneratedShip generate(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, ShipGenerationDebugInfo* debugInfo = nullptr, ShipGenerationPerformanceInfo* performanceInfo = nullptr);
        GeneratedShip generateCalibrated(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo = nullptr);

        // Fully explicit structural + faction profile API. The configuration has
        // no ShipStyle/ShipFactionType selector, so neither provenance identity is
        // fabricated or given hidden precedence. PaletteConfiguration independently
        // selects faction-profile generated, explicit generated, or fixed colors.
        // Structural/faction/generated-palette profiles are validated as applicable.
        GeneratedShip generate(const ExplicitShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, const ShipFactionProfile& factionProfile, ShipGenerationDebugInfo* debugInfo = nullptr, ShipGenerationPerformanceInfo* performanceInfo = nullptr);
        GeneratedShip generateCalibrated(const ExplicitShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, const ShipFactionProfile& factionProfile, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo = nullptr);

    private:
        friend struct ShipGeneratorStaticFactionRegressionAccess;

        GeneratedShip generateInternal(const ShipResolvedGenerationConfiguration& configuration,
            const GenerationCalibrationSettings* calibrationSettings,
            ShipGenerationDebugInfo* debugInfo,
            ShipGenerationPerformanceInfo* performanceInfo);

        // Internal compatibility bridge retained for focused routing regressions;
        // it immediately constructs the canonical resolved configuration.
        GeneratedShip generateInternal(const ExplicitShipGenerationConfiguration& configuration,
            const ShipGenerationProfile& profile,
            const ShipFactionProfile& factionProfile,
            ShipStyle builtInStyleProvenance,
            ShipFactionType builtInFactionProvenance,
            const GenerationCalibrationSettings* calibrationSettings,
            ShipGenerationDebugInfo* debugInfo,
            ShipGenerationPerformanceInfo* performanceInfo);

        static constexpr uint32_t MaximumHullGenerationAttempts = 8u;
    };
}
