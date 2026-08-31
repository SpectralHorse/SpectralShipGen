#pragma once

#include <array>
#include <cstdint>
#include <random>

#include "CockpitData.h"
#include "CoreTreatmentData.h"
#include "DetailMotifPlan.h"
#include <SpectralShipGen/GeneratedShip.h>
#include <SpectralShipGen/GenerationComplexityBudget.h>
#include <SpectralShipGen/GenerationDomain.h>
#include <SpectralShipGen/GenerationScaleTraits.h>
#include <SpectralShipGen/GenerationSpatialBudget.h>
#include <SpectralShipGen/GenerationTuningProfile.h>
#include "HullLayerData.h"
#include "MajorFeatureData.h"
#include "MaterialCompositionData.h"
#include "LiveryData.h"
#include <SpectralShipGen/MacroAsymmetry.h>
#include <SpectralShipGen/ShipGenerationDebugInfo.h>
#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include "StructuralNegativeSpaceData.h"
#include "VisualHierarchyPlan.h"
#include "WingRegionData.h"
#include "WeaponData.h"

namespace SpectralShipGen
{
    struct ShipGenerationContext
    {
        ShipGenerationContext(const ExplicitShipGenerationConfiguration& settings,
            const ShipGenerationProfile& profile,
            const ShipFactionProfile& factionProfile,
            const ShipGenerationSeeds& seeds,
            ShipGenerationDebugInfo* debugInfo = nullptr,
            const GenerationCalibrationSettings* calibrationSettings = nullptr,
            ShipStyle builtInStyleProvenance = ShipStyle::SHIP_STYLE_END,
            ShipFactionType builtInFactionProvenance = ShipFactionType::SHIP_FACTION_TYPE_END);

        // Internal compatibility entry for generation-focused regressions/helpers
        // that still construct a context from the legacy preset settings.
        ShipGenerationContext(const ShipGenerationSettings& settings,
            const ShipGenerationProfile& profile,
            const ShipGenerationSeeds& seeds,
            ShipGenerationDebugInfo* debugInfo = nullptr,
            const GenerationCalibrationSettings* calibrationSettings = nullptr);

        uint32_t getGenerationRandomUInt(GenerationDomain domain, uint32_t minimum, uint32_t maximum);
        uint32_t getGenerationRandomUInt(GenerationDomain domain, const UIntRange& range);
        uint64_t getGenerationRandomUInt64(GenerationDomain domain, uint64_t minimum, uint64_t maximum);
        void beginGenerationDomainCalibrationSubstream(GenerationDomain domain, uint64_t salt);
        void endGenerationDomainCalibrationSubstream();
        void resetComplexityBudget();
        void updateComplexityBudgetDebugInfo();
        void resetSpatialBudget();
        void updateSpatialBudgetDebugInfo();
        void updateVisualHierarchyDebugInfo();
        void updateMaterialCompositionDebugInfo();
        void updateLiveryDebugInfo();
        void updateDetailMotifDebugInfo();

        ExplicitShipGenerationConfiguration Settings;
        const ShipGenerationProfile& Profile;
        const ShipFactionProfile& FactionProfile;
        GenerationScaleTraits ScaleTraits;
        GenerationComplexityBudget ComplexityBudget;
        GenerationSpatialBudget SpatialBudget;
        ShipGenerationSeeds Seeds;
        GenerationDomainSeeds DomainSeeds;
        GeneratedShip Ship;
        WingRegionData WingRegions;
        StructuralNegativeSpaceData StructuralNegativeSpace;
        VisualHierarchyPlan VisualHierarchy;
        CockpitData Cockpit;
        CoreTreatmentData CoreTreatment;
        HullLayerData HullLayers;
        MajorFeatureData MajorFeatures;
        MaterialCompositionData MaterialComposition;
        LiveryData Livery;
        DetailMotifPlan DetailMotifs;
        MacroAsymmetryPlan MacroAsymmetry;
        WeaponData Weapons;
        ShipGenerationDebugInfo* DebugInfo = nullptr;
        const GenerationCalibrationSettings* CalibrationSettings = nullptr;

    private:
        uint32_t getRandomUInt(std::mt19937_64& randomGenerator, uint32_t minimum, uint32_t maximum);
        uint64_t getRandomUInt64(std::mt19937_64& randomGenerator, uint64_t minimum, uint64_t maximum);
        std::mt19937_64& getDomainRandomGenerator(GenerationDomain domain);
        std::mt19937_64& getLegacyRandomGenerator(GenerationSeedChannel channel);

        std::array<std::mt19937_64, GenerationDomainCount> m_DomainRandomGenerators;
        std::mt19937_64 m_LegacyStructureRandomGenerator;
        std::mt19937_64 m_LegacyPaletteRandomGenerator;
        std::mt19937_64 m_LegacyDetailRandomGenerator;
        std::mt19937_64 m_LegacyAttachmentRandomGenerator;
        std::mt19937_64 m_SavedCalibrationRandomGenerator;
        GenerationDomain m_CalibrationDomain = GenerationDomain::GENERATION_DOMAIN_END;
        bool m_CalibrationSubstreamActive = false;
    };
}
