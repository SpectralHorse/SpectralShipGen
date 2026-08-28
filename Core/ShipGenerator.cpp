#include "ShipGenerator.h"

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

        uint64_t calibrationSalt(const GenerationCalibrationSettings& settings, uint64_t stageSalt, uint64_t attempt = 0u)
        {
            return mixGenerationSeed64(stageSalt ^ settings.IsolationSalt ^ (attempt * 0x9E3779B97F4A7C15ull));
        }
    }

    ShipGenerator::ShipGenerator() = default;

    GeneratedShip ShipGenerator::generate(const ShipGenerationSettings& settings)
    {
        return generateInternal(settings, nullptr, nullptr);
    }

    GeneratedShip ShipGenerator::generate(const ShipGenerationSettings& settings, ShipGenerationDebugInfo* debugInfo)
    {
        return generateInternal(settings, nullptr, debugInfo);
    }

    GeneratedShip ShipGenerator::generateCalibrated(const ShipGenerationSettings& settings, const GenerationCalibrationSettings& calibrationSettings, ShipGenerationDebugInfo* debugInfo)
    {
        return generateInternal(settings, &calibrationSettings, debugInfo);
    }

    GeneratedShip ShipGenerator::generateInternal(const ShipGenerationSettings& settings, const GenerationCalibrationSettings* calibrationSettings, ShipGenerationDebugInfo* debugInfo)
    {
        if (settings.Dimensions.Width < 16u || settings.Dimensions.Height < 16u)
        {
            throw std::invalid_argument("ShipGenerationSettings dimensions must be at least 16 pixels.");
        }

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

        if (debugInfo != nullptr)
        {
            *debugInfo = ShipGenerationDebugInfo();
            debugInfo->ScaleTraits = context.ScaleTraits;
        }

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
            hullGenerator.generate(context);
            if (isolateHull) { context.endGenerationDomainCalibrationSubstream(); }

            if (hullGenerator.validate(context))
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
                macroAsymmetryPlanner.createPlan(context);
                cockpitGenerator.generate(context);
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::COCKPIT_STRUCTURE);

                const bool isolateEngine = calibrationSettings != nullptr && calibrationSettings->IsolatedGroup.has_value() && groupUsesEngineStage(*calibrationSettings->IsolatedGroup);
                if (isolateEngine) { context.beginGenerationDomainCalibrationSubstream(GenerationDomain::ENGINES, calibrationSalt(*calibrationSettings, EngineCalibrationSalt)); }
                engineGenerator.generate(context);
                if (isolateEngine) { context.endGenerationDomainCalibrationSubstream(); }

                coreTreatmentGenerator.generate(context);
                hullLayerGenerator.generate(context);
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::HULL_LAYER);

                const bool isolateMajorFeature = calibrationSettings != nullptr && calibrationSettings->IsolatedGroup.has_value() && groupUsesMajorFeatureStage(*calibrationSettings->IsolatedGroup);
                if (isolateMajorFeature) { context.beginGenerationDomainCalibrationSubstream(GenerationDomain::MAJOR_FEATURES, calibrationSalt(*calibrationSettings, MajorFeatureCalibrationSalt)); }
                majorFeatureGenerator.generate(context);
                if (isolateMajorFeature) { context.endGenerationDomainCalibrationSubstream(); }
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::MAJOR_FEATURE);

                const bool isolateWeapon = calibrationSettings != nullptr && calibrationSettings->IsolatedGroup.has_value() && groupUsesWeaponStage(*calibrationSettings->IsolatedGroup);
                if (isolateWeapon) { context.beginGenerationDomainCalibrationSubstream(GenerationDomain::WEAPONS, calibrationSalt(*calibrationSettings, WeaponCalibrationSalt)); }
                weaponGenerator.generate(context);
                if (isolateWeapon) { context.endGenerationDomainCalibrationSubstream(); }
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::LARGE_WEAPON);

                if (settings.AttachmentsEnabled)
                {
                    const bool isolateAttachment = calibrationSettings != nullptr && calibrationSettings->IsolatedGroup.has_value() && groupUsesAttachmentStage(*calibrationSettings->IsolatedGroup);
                    if (isolateAttachment) { context.beginGenerationDomainCalibrationSubstream(GenerationDomain::ATTACHMENTS, calibrationSalt(*calibrationSettings, AttachmentCalibrationSalt)); }
                    attachmentGenerator.generate(context);
                    if (isolateAttachment) { context.endGenerationDomainCalibrationSubstream(); }
                }
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::ATTACHMENT);

                materialCompositionGenerator.generate(context);
                liveryGenerator.generate(context);
                detailGenerator.generate(context);
                context.ComplexityBudget.finalizeCategory(GenerationComplexityCategory::DETAIL);
                context.updateComplexityBudgetDebugInfo();
                context.updateSpatialBudgetDebugInfo();
                shipPainter.paint(context);
                return context.Ship;
            }

            if (debugInfo != nullptr)
            {
                ++debugInfo->HullValidationRejectionCount;
            }
        }

        throw std::runtime_error("Failed to generate a valid hull within the maximum number of attempts for seed = " + std::to_string(settings.Seed));
    }
}
