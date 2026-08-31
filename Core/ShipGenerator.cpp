#include <SpectralShipGen/ShipGenerator.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "Generation/AttachmentGenerator.h"
#include "Generation/CockpitGenerator.h"
#include "Generation/CoreTreatmentGenerator.h"
#include "Generation/DetailGenerator.h"
#include "Generation/EngineGenerator.h"
#include "Generation/HullGenerator.h"
#include "Generation/HullLayerGenerator.h"
#include "Generation/MacroAsymmetryPlanner.h"
#include "Generation/MajorFeatureGenerator.h"
#include "Generation/MaterialCompositionGenerator.h"
#include "Generation/LiveryGenerator.h"
#include "Generation/ShipGenerationContext.h"
#include "Generation/ShipPainter.h"
#include "Generation/WeaponGenerator.h"
#include "Generation/VisualHierarchyPlanner.h"
#include <SpectralShipGen/GenerationTuningProfile.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationProfileValidation.h>
#include <SpectralShipGen/ShipFactionProfileValidation.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerationPerformance.h>
#include <SpectralShipGen/ShipPaletteGenerator.h>
#include <SpectralShipGen/ShipPaletteGenerationProfileValidation.h>
#include <SpectralShipGen/ShipResolvedGenerationConfiguration.h>

namespace SpectralShipGen
{
    namespace
    {
        constexpr uint64_t HullCalibrationSalt = 0xA6D94B5E74F3C221ull;
        constexpr uint64_t EngineCalibrationSalt = 0x61C8864680B583EBull;
        constexpr uint64_t MajorFeatureCalibrationSalt = 0xD1B54A32D192ED03ull;
        constexpr uint64_t WeaponCalibrationSalt = 0x94D049BB133111EBull;
        constexpr uint64_t AttachmentCalibrationSalt = 0x369DEA0F31A53F85ull;

        bool groupUsesHullStage(GenerationWeightGroup group) { return group == GenerationWeightGroup::HULL_MODIFIER; }
        bool groupUsesEngineStage(GenerationWeightGroup group) { return group == GenerationWeightGroup::ENGINE_LAYOUT || group == GenerationWeightGroup::ENGINE_SIZE || group == GenerationWeightGroup::ENGINE_NACELLE_PRESENCE; }
        bool groupUsesMajorFeatureStage(GenerationWeightGroup group) { return group == GenerationWeightGroup::MAJOR_FEATURE_TYPE || group == GenerationWeightGroup::MAJOR_FEATURE_PRESENCE; }
        bool groupUsesWeaponStage(GenerationWeightGroup group) { return group == GenerationWeightGroup::LARGE_WEAPON_TYPE || group == GenerationWeightGroup::LARGE_WEAPON_PRESENCE; }
        bool groupUsesAttachmentStage(GenerationWeightGroup group) { return group == GenerationWeightGroup::ATTACHMENT_TYPE || group == GenerationWeightGroup::ATTACHMENT_PRESENCE; }

        class ScopedStageTimer
        {
        public:
            ScopedStageTimer(ShipGenerationPerformanceInfo* info, ShipGenerationPerformanceStage stage) : Info(info), Stage(stage)
            {
                if (Info != nullptr) { Start = std::chrono::steady_clock::now(); }
            }
            ~ScopedStageTimer()
            {
                if (Info == nullptr) { return; }
                const auto elapsed = std::chrono::steady_clock::now() - Start;
                Info->StageDurationNanoseconds[static_cast<std::size_t>(Stage)] += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());
            }
        private:
            ShipGenerationPerformanceInfo* Info = nullptr;
            ShipGenerationPerformanceStage Stage = ShipGenerationPerformanceStage::SETUP_PLANNING;
            std::chrono::steady_clock::time_point Start;
        };

        uint64_t calibrationSalt(const GenerationCalibrationSettings& settings, uint64_t stageSalt, uint64_t attempt = 0u)
        {
            return mixGenerationSeed64(stageSalt ^ settings.IsolationSalt ^ (attempt * 0x9E3779B97F4A7C15ull));
        }
    }

    ShipGenerator::ShipGenerator() = default;

    namespace
    {
        ShipPalette resolvePalette(const ShipPaletteConfiguration& paletteConfiguration,
            uint64_t paletteSeed,
            const ShipFactionProfile& factionProfile,
            const ShipGenerationProfile& structuralProfile)
        {
            switch (paletteConfiguration.Mode)
            {
            case ShipPaletteSourceMode::FACTION_PROFILE_GENERATED:
                return ShipPaletteGenerator::generate(paletteSeed, factionProfile, structuralProfile);
            case ShipPaletteSourceMode::EXPLICIT_GENERATED:
                return ShipPaletteGenerator::generate(paletteSeed, paletteConfiguration.Generated, structuralProfile);
            case ShipPaletteSourceMode::FIXED:
                return paletteConfiguration.Fixed;
            default:
                throw std::invalid_argument("ShipPaletteConfiguration.Mode is outside the supported range.");
            }
        }

        void throwIfInvalid(const ShipResolvedGenerationConfiguration& configuration)
        {
            const ValidationResult validation = validateShipGenerationConfiguration(configuration);
            if (validation.isValid()) { return; }

            std::string message = "Invalid ShipResolvedGenerationConfiguration";
            const std::size_t maximumReportedErrors = 4u;
            const std::size_t errorCount = std::min(maximumReportedErrors, validation.Errors.size());
            for (std::size_t index = 0u; index < errorCount; ++index)
            {
                message += index == 0u ? ": " : "; ";
                message += validation.Errors[index].Field + " - " + validation.Errors[index].Message;
            }
            if (validation.Errors.size() > errorCount) { message += "; ..."; }
            throw std::invalid_argument(message);
        }

        ShipGenerationProfile resolveCalibration(const ShipGenerationProfile& profile,
            const std::optional<ShipStyle>& builtInStyleProvenance,
            const GenerationCalibrationSettings* calibrationSettings)
        {
            if (calibrationSettings == nullptr) { return profile; }
            if (calibrationSettings->TuningProfile != nullptr && calibrationSettings->ExplicitTuningProfile != nullptr)
            {
                throw std::invalid_argument("GenerationCalibrationSettings cannot specify both TuningProfile and ExplicitTuningProfile.");
            }
            if (calibrationSettings->ExplicitTuningProfile != nullptr)
            {
                return applyGenerationTuningProfile(profile, *calibrationSettings->ExplicitTuningProfile);
            }
            if (calibrationSettings->TuningProfile != nullptr)
            {
                if (!builtInStyleProvenance.has_value())
                {
                    throw std::invalid_argument("Explicit-profile calibration cannot use a style-indexed TuningProfile; supply ExplicitTuningProfile instead.");
                }
                return applyGenerationTuningProfile(profile, *builtInStyleProvenance, *calibrationSettings->TuningProfile);
            }
            return profile;
        }
    }

    GeneratedShip ShipGenerator::generate(const ShipResolvedGenerationConfiguration& configuration, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo)
    {
        return generateInternal(configuration, nullptr, debugInfo, performanceInfo);
    }

    GeneratedShip ShipGenerator::generateCalibrated(const ShipResolvedGenerationConfiguration& configuration, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo)
    {
        return generateInternal(configuration, &calibrationSettings, debugInfo, nullptr);
    }

    GeneratedShip ShipGenerator::generate(const ShipGenerationSettings& settings)
    {
        return generate(settings, nullptr, nullptr);
    }

    GeneratedShip ShipGenerator::generate(const ShipGenerationSettings& settings, ShipGenerationDebugInfo* debugInfo)
    {
        return generate(settings, debugInfo, nullptr);
    }

    GeneratedShip ShipGenerator::generate(const ShipGenerationSettings& settings, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo)
    {
        return generate(resolveShipGenerationConfiguration(settings), debugInfo, performanceInfo);
    }

    GeneratedShip ShipGenerator::generateCalibrated(const ShipGenerationSettings& settings, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo)
    {
        return generateCalibrated(resolveShipGenerationConfiguration(settings), calibrationSettings, debugInfo);
    }

    GeneratedShip ShipGenerator::generate(const ShipGenerationRecipe& recipe, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo)
    {
        const ValidationResult validation = validateShipGenerationRecipe(recipe);
        if (!validation.isValid())
        {
            throw std::invalid_argument("Invalid ShipGenerationRecipe: " + validation.Errors.front().Field + " - " + validation.Errors.front().Message);
        }
        return generate(resolveShipGenerationConfiguration(recipe), debugInfo, performanceInfo);
    }

    GeneratedShip ShipGenerator::generate(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo)
    {
        return generate(resolveShipGenerationConfiguration(configuration, profile), debugInfo, performanceInfo);
    }

    GeneratedShip ShipGenerator::generateCalibrated(const ShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo)
    {
        return generateCalibrated(resolveShipGenerationConfiguration(configuration, profile), calibrationSettings, debugInfo);
    }

    GeneratedShip ShipGenerator::generate(const ExplicitShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, const ShipFactionProfile& factionProfile, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo)
    {
        return generate(resolveShipGenerationConfiguration(configuration, profile, factionProfile), debugInfo, performanceInfo);
    }

    GeneratedShip ShipGenerator::generateCalibrated(const ExplicitShipGenerationConfiguration& configuration, const ShipGenerationProfile& profile, const ShipFactionProfile& factionProfile, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo)
    {
        return generateCalibrated(resolveShipGenerationConfiguration(configuration, profile, factionProfile), calibrationSettings, debugInfo);
    }

    GeneratedShip ShipGenerator::generateInternal(const ShipResolvedGenerationConfiguration& resolvedConfiguration, const GenerationCalibrationSettings* calibrationSettings, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo)
    {
        throwIfInvalid(resolvedConfiguration);

        const ExplicitShipGenerationConfiguration& configuration = resolvedConfiguration.Generation;
        const ShipFactionProfile& factionProfile = resolvedConfiguration.FactionProfile;
        const ShipGenerationProfile profile = resolveCalibration(resolvedConfiguration.StructuralProfile, resolvedConfiguration.Provenance.StructuralPreset, calibrationSettings);
        ShipResolvedGenerationConfiguration calibratedConfiguration = resolvedConfiguration;
        calibratedConfiguration.StructuralProfile = profile;
        throwIfInvalid(calibratedConfiguration);

        const auto generationStart = performanceInfo == nullptr ? std::chrono::steady_clock::time_point() : std::chrono::steady_clock::now();
        if (performanceInfo != nullptr) { performanceInfo->reset(); }

        const auto setupStart = performanceInfo == nullptr ? std::chrono::steady_clock::time_point() : std::chrono::steady_clock::now();
        ShipGenerationSeeds seeds = deriveShipGenerationSeeds(configuration.Seed);
        seeds = applyShipGenerationSeedOverrides(seeds, configuration.SeedOverrides);

        ShipGenerationContext context(configuration, profile, factionProfile, seeds, debugInfo, calibrationSettings);
        context.Ship.Provenance = resolvedConfiguration.Provenance;
        context.Ship.PaletteSourceMode = configuration.PaletteConfiguration.Mode;
        context.Ship.Palette = resolvePalette(configuration.PaletteConfiguration, context.DomainSeeds.get(GenerationDomain::PALETTE), factionProfile, profile);

        HullGenerator hullGenerator;
        HullLayerGenerator hullLayerGenerator;
        CockpitGenerator cockpitGenerator;
        CoreTreatmentGenerator coreTreatmentGenerator;
        EngineGenerator engineGenerator;
        MajorFeatureGenerator majorFeatureGenerator;
        MaterialCompositionGenerator materialCompositionGenerator;
        LiveryGenerator liveryGenerator;
        MacroAsymmetryPlanner macroAsymmetryPlanner;
        WeaponGenerator weaponGenerator;
        AttachmentGenerator attachmentGenerator;
        DetailGenerator detailGenerator;
        ShipPainter shipPainter;
        VisualHierarchyPlanner visualHierarchyPlanner;

        visualHierarchyPlanner.createPlan(context);
        if (performanceInfo != nullptr)
        {
            performanceInfo->StageDurationNanoseconds[static_cast<std::size_t>(ShipGenerationPerformanceStage::SETUP_PLANNING)] += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - setupStart).count());
        }

        if (debugInfo != nullptr)
        {
            *debugInfo = ShipGenerationDebugInfo();
            debugInfo->ScaleTraits = context.ScaleTraits;
        }

        uint32_t structuralNegativeSpaceAttempts = 0u;
        uint32_t structuralNegativeSpaceSuccesses = 0u;
        for (uint32_t attempt = 0u; attempt < MaximumHullGenerationAttempts; ++attempt)
        {
            context.Ship.clear();
            context.resetComplexityBudget();
            if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.ReservedCategory == GenerationComplexityCategory::SILHOUETTE)
            {
                context.VisualHierarchy.ReservedComplexity = context.ComplexityBudget.applyHierarchyReservation(context.VisualHierarchy.ReservedCategory, context.VisualHierarchy.ReservedComplexity);
            }

            if (debugInfo != nullptr)
            {
                debugInfo->HullGenerationAttemptCount = attempt + 1u;
                debugInfo->AppliedHullModifiers.clear();
                debugInfo->HullStages.clear();
                debugInfo->SilhouetteGuidanceAppliedCount = 0u;
                debugInfo->LastSilhouetteValidationFailure = SilhouetteValidationFailureReason::NONE;
            }

            const bool isolateHull = calibrationSettings != nullptr && calibrationSettings->IsolatedGroup.has_value() && groupUsesHullStage(*calibrationSettings->IsolatedGroup);
            if (isolateHull) { context.beginGenerationDomainCalibrationSubstream(GenerationDomain::HULL, calibrationSalt(*calibrationSettings, HullCalibrationSalt, attempt)); }
            { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::HULL_GENERATION); hullGenerator.generate(context); }
            if (debugInfo != nullptr)
            {
                structuralNegativeSpaceAttempts += debugInfo->StructuralNegativeSpaceAttemptCount;
                structuralNegativeSpaceSuccesses += debugInfo->StructuralNegativeSpaceSuccessCount;
                debugInfo->StructuralNegativeSpaceAttemptCount = structuralNegativeSpaceAttempts;
                debugInfo->StructuralNegativeSpaceSuccessCount = structuralNegativeSpaceSuccesses;
            }
            if (isolateHull) { context.endGenerationDomainCalibrationSubstream(); }

            bool hullValid = false;
            { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::HULL_VALIDATION); hullValid = hullGenerator.validate(context); }
            if (hullValid)
            {
                visualHierarchyPlanner.resolveAfterHull(context);
                if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.ReservedCategory != GenerationComplexityCategory::SILHOUETTE)
                {
                    context.VisualHierarchy.ReservedComplexity = context.ComplexityBudget.applyHierarchyReservation(context.VisualHierarchy.ReservedCategory, context.VisualHierarchy.ReservedComplexity);
                }
                context.resetSpatialBudget();
                visualHierarchyPlanner.applySpatialPreference(context);
                context.updateSpatialBudgetDebugInfo();
                context.updateVisualHierarchyDebugInfo();
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::SILHOUETTE);
                { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::MACRO_ASYMMETRY_PLANNING); macroAsymmetryPlanner.createPlan(context); }
                { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::COCKPIT); cockpitGenerator.generate(context); }
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::COCKPIT_STRUCTURE);

                const bool isolateEngine = calibrationSettings != nullptr && calibrationSettings->IsolatedGroup.has_value() && groupUsesEngineStage(*calibrationSettings->IsolatedGroup);
                if (isolateEngine) { context.beginGenerationDomainCalibrationSubstream(GenerationDomain::ENGINES, calibrationSalt(*calibrationSettings, EngineCalibrationSalt)); }
                { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::ENGINES); engineGenerator.generate(context); }
                if (isolateEngine) { context.endGenerationDomainCalibrationSubstream(); }

                { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::CENTRAL_CORE); coreTreatmentGenerator.generate(context); }
                { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::HULL_LAYERS); hullLayerGenerator.generate(context); }
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::HULL_LAYER);

                const bool isolateMajorFeature = calibrationSettings != nullptr && calibrationSettings->IsolatedGroup.has_value() && groupUsesMajorFeatureStage(*calibrationSettings->IsolatedGroup);
                if (isolateMajorFeature) { context.beginGenerationDomainCalibrationSubstream(GenerationDomain::MAJOR_FEATURES, calibrationSalt(*calibrationSettings, MajorFeatureCalibrationSalt)); }
                { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::MAJOR_FEATURES); majorFeatureGenerator.generate(context); }
                if (isolateMajorFeature) { context.endGenerationDomainCalibrationSubstream(); }
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::MAJOR_FEATURE);

                const bool isolateWeapon = calibrationSettings != nullptr && calibrationSettings->IsolatedGroup.has_value() && groupUsesWeaponStage(*calibrationSettings->IsolatedGroup);
                if (isolateWeapon) { context.beginGenerationDomainCalibrationSubstream(GenerationDomain::WEAPONS, calibrationSalt(*calibrationSettings, WeaponCalibrationSalt)); }
                { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::WEAPONS); weaponGenerator.generate(context); }
                if (isolateWeapon) { context.endGenerationDomainCalibrationSubstream(); }
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::LARGE_WEAPON);

                if (configuration.AttachmentsEnabled)
                {
                    const bool isolateAttachment = calibrationSettings != nullptr && calibrationSettings->IsolatedGroup.has_value() && groupUsesAttachmentStage(*calibrationSettings->IsolatedGroup);
                    if (isolateAttachment) { context.beginGenerationDomainCalibrationSubstream(GenerationDomain::ATTACHMENTS, calibrationSalt(*calibrationSettings, AttachmentCalibrationSalt)); }
                    { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::ATTACHMENTS); attachmentGenerator.generate(context); }
                    if (isolateAttachment) { context.endGenerationDomainCalibrationSubstream(); }
                }
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::ATTACHMENT);

                { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::MATERIAL_COMPOSITION); materialCompositionGenerator.generate(context); }
                { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::LIVERY); liveryGenerator.generate(context); }
                { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::DETAILS); detailGenerator.generate(context); }
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::DETAIL);
                context.updateComplexityBudgetDebugInfo();
                context.updateSpatialBudgetDebugInfo();
                { ScopedStageTimer timer(performanceInfo, ShipGenerationPerformanceStage::PAINTING_COMPOSITION); shipPainter.paint(context); }
                if (performanceInfo != nullptr) { performanceInfo->TotalDurationNanoseconds = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - generationStart).count()); }
                return context.Ship;
            }

            if (debugInfo != nullptr)
            {
                ++debugInfo->HullValidationRejectionCount;
            }
        }

        if (performanceInfo != nullptr) { performanceInfo->TotalDurationNanoseconds = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - generationStart).count()); }
        throw std::runtime_error("Failed to generate a valid hull within the maximum number of attempts for seed = " + std::to_string(configuration.Seed));
    }
}
