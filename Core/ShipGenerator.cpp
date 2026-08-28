#include "ShipGenerator.h"

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
#include "GenerationTuningProfile.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerationPerformance.h"
#include "ShipPaletteGenerator.h"

namespace PixelShipGenerator
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

    GeneratedShip ShipGenerator::generate(const ShipGenerationSettings& settings)
    {
        return generateInternal(settings, nullptr, nullptr, nullptr);
    }

    GeneratedShip ShipGenerator::generate(const ShipGenerationSettings& settings, ShipGenerationDebugInfo* debugInfo)
    {
        return generateInternal(settings, nullptr, debugInfo, nullptr);
    }

    GeneratedShip ShipGenerator::generate(const ShipGenerationSettings& settings, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo)
    {
        return generateInternal(settings, nullptr, debugInfo, performanceInfo);
    }

    GeneratedShip ShipGenerator::generateCalibrated(const ShipGenerationSettings& settings, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo)
    {
        return generateInternal(settings, &calibrationSettings, debugInfo, nullptr);
    }

    GeneratedShip ShipGenerator::generateInternal(const ShipGenerationSettings& settings, const GenerationCalibrationSettings* calibrationSettings, ShipGenerationDebugInfo* debugInfo, ShipGenerationPerformanceInfo* performanceInfo)
    {
        if (settings.Dimensions.Width < 16u || settings.Dimensions.Height < 16u)
        {
            throw std::invalid_argument("ShipGenerationSettings dimensions must be at least 16 pixels.");
        }

        const auto generationStart = performanceInfo == nullptr ? std::chrono::steady_clock::time_point() : std::chrono::steady_clock::now();
        if (performanceInfo != nullptr) { performanceInfo->reset(); }

        const auto setupStart = performanceInfo == nullptr ? std::chrono::steady_clock::time_point() : std::chrono::steady_clock::now();
        ShipGenerationSeeds seeds = deriveShipGenerationSeeds(settings.Seed);
        seeds = applyShipGenerationSeedOverrides(seeds, settings.SeedOverrides);

        ShipGenerationProfile profile = getShipGenerationProfile(settings.Style);
        if (calibrationSettings != nullptr && calibrationSettings->TuningProfile != nullptr)
        {
            profile = applyGenerationTuningProfile(profile, settings.Style, *calibrationSettings->TuningProfile);
        }

        ShipGenerationContext context(settings, profile, seeds, debugInfo, calibrationSettings);
        context.Ship.Palette = ShipPaletteGenerator::generate(context.DomainSeeds.get(GenerationDomain::PALETTE), settings.Style, settings.Faction, profile, settings.RandomStreamMode != GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS);

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

                if (settings.AttachmentsEnabled)
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
        throw std::runtime_error("Failed to generate a valid hull within the maximum number of attempts for seed = " + std::to_string(settings.Seed));
    }
}
