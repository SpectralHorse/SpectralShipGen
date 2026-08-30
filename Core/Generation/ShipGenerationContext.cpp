#include "ShipGenerationContext.h"

#include <random>
#include <stdexcept>

#include "PixelMaskUtils.h"

namespace PixelShipGenerator
{
    namespace
    {
        ShipGenerationConfiguration copyGenerationConfiguration(const ShipGenerationSettings& settings)
        {
            ShipGenerationConfiguration configuration;
            configuration.Seed = settings.Seed;
            configuration.Dimensions = settings.Dimensions;
            configuration.Faction = settings.Faction;
            configuration.DetailDensity = settings.DetailDensity;
            configuration.AsymmetricDetailChance = settings.AsymmetricDetailChance;
            configuration.AttachmentsEnabled = settings.AttachmentsEnabled;
            configuration.SeedOverrides = settings.SeedOverrides;
            configuration.DomainSeedOverrides = settings.DomainSeedOverrides;
            configuration.RandomStreamMode = settings.RandomStreamMode;
            return configuration;
        }
    }

    ShipGenerationContext::ShipGenerationContext(const ShipGenerationConfiguration& settings, const ShipGenerationProfile& profile, const ShipFactionProfile& factionProfile, const ShipGenerationSeeds& seeds, ShipGenerationDebugInfo* debugInfo, const GenerationCalibrationSettings* calibrationSettings, ShipStyle builtInStyleProvenance)
        : Settings(settings), Profile(profile), FactionProfile(factionProfile), ScaleTraits(GenerationScaleTraits::fromDimensions(settings.Dimensions)), ComplexityBudget(GenerationComplexityBudget::create(ScaleTraits, profile, factionProfile, settings.RandomStreamMode != GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS)), Seeds(seeds), DomainSeeds(resolveGenerationDomainSeeds(seeds, settings.DomainSeedOverrides, settings.RandomStreamMode)), DebugInfo(debugInfo), CalibrationSettings(calibrationSettings), m_LegacyStructureRandomGenerator(seeds.Structure), m_LegacyPaletteRandomGenerator(seeds.Palette), m_LegacyDetailRandomGenerator(seeds.Details), m_LegacyAttachmentRandomGenerator(seeds.Attachments), m_SavedCalibrationRandomGenerator(0u)
    {
        Ship.reset(settings.Dimensions.Width, settings.Dimensions.Height, seeds);
        Ship.DomainSeeds = DomainSeeds;
        Ship.AnimationTraits = profile.AnimationTraits;
        Ship.Style = builtInStyleProvenance;
        Ship.Faction = settings.Faction;
        WingRegions.reset(settings.Dimensions.Width, settings.Dimensions.Height);
        StructuralNegativeSpace.reset(settings.Dimensions.Width, settings.Dimensions.Height);
        Cockpit.reset(settings.Dimensions.Width, settings.Dimensions.Height);
        CoreTreatment.reset(settings.Dimensions.Width, settings.Dimensions.Height);
        HullLayers.reset(settings.Dimensions.Width, settings.Dimensions.Height);
        MajorFeatures.reset(settings.Dimensions.Width, settings.Dimensions.Height);
        MaterialComposition.reset(settings.Dimensions.Width, settings.Dimensions.Height);
        Livery.reset(settings.Dimensions.Width, settings.Dimensions.Height);
        Weapons.reset(settings.Dimensions.Width, settings.Dimensions.Height);

        for (std::size_t index = 0u; index < GenerationDomainCount; ++index)
        {
            m_DomainRandomGenerators[index].seed(DomainSeeds.Values[index]);
        }
    }

    ShipGenerationContext::ShipGenerationContext(const ShipGenerationSettings& settings, const ShipGenerationProfile& profile, const ShipGenerationSeeds& seeds, ShipGenerationDebugInfo* debugInfo, const GenerationCalibrationSettings* calibrationSettings)
        : ShipGenerationContext(copyGenerationConfiguration(settings), profile, getShipFactionProfile(settings.Faction), seeds, debugInfo, calibrationSettings, settings.Style)
    {
    }

    uint32_t ShipGenerationContext::getGenerationRandomUInt(GenerationDomain domain, uint32_t minimum, uint32_t maximum)
    {
        std::mt19937_64& generator = Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS
            ? getLegacyRandomGenerator(getGenerationDomainParentChannel(domain))
            : getDomainRandomGenerator(domain);
        return getRandomUInt(generator, minimum, maximum);
    }

    uint32_t ShipGenerationContext::getGenerationRandomUInt(GenerationDomain domain, const UIntRange& range)
    {
        return getGenerationRandomUInt(domain, range.Min, range.Max);
    }

    uint64_t ShipGenerationContext::getGenerationRandomUInt64(GenerationDomain domain, uint64_t minimum, uint64_t maximum)
    {
        std::mt19937_64& generator = Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS
            ? getLegacyRandomGenerator(getGenerationDomainParentChannel(domain))
            : getDomainRandomGenerator(domain);
        return getRandomUInt64(generator, minimum, maximum);
    }

    void ShipGenerationContext::beginGenerationDomainCalibrationSubstream(GenerationDomain domain, uint64_t salt)
    {
        if (m_CalibrationSubstreamActive)
        {
            throw std::logic_error("Nested generation-domain calibration substreams are not supported.");
        }

        std::mt19937_64& generator = Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS
            ? getLegacyRandomGenerator(getGenerationDomainParentChannel(domain))
            : getDomainRandomGenerator(domain);
        m_SavedCalibrationRandomGenerator = generator;
        const uint64_t baseSeed = Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS
            ? getGenerationSeedForChannel(Seeds, getGenerationDomainParentChannel(domain))
            : DomainSeeds.get(domain);
        generator.seed(mixGenerationSeed64(baseSeed ^ salt));
        m_CalibrationDomain = domain;
        m_CalibrationSubstreamActive = true;
    }

    void ShipGenerationContext::endGenerationDomainCalibrationSubstream()
    {
        if (!m_CalibrationSubstreamActive)
        {
            return;
        }

        std::mt19937_64& generator = Settings.RandomStreamMode == GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS
            ? getLegacyRandomGenerator(getGenerationDomainParentChannel(m_CalibrationDomain))
            : getDomainRandomGenerator(m_CalibrationDomain);
        generator = m_SavedCalibrationRandomGenerator;
        m_CalibrationDomain = GenerationDomain::GENERATION_DOMAIN_END;
        m_CalibrationSubstreamActive = false;
    }

    void ShipGenerationContext::resetComplexityBudget()
    {
        ComplexityBudget = GenerationComplexityBudget::create(ScaleTraits, Profile, FactionProfile, Settings.RandomStreamMode != GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS);
        updateComplexityBudgetDebugInfo();
    }

    void ShipGenerationContext::updateComplexityBudgetDebugInfo()
    {
        if (DebugInfo == nullptr)
        {
            return;
        }

        DebugInfo->ComplexityInitialBudget = ComplexityBudget.getInitialBudget();
        DebugInfo->ComplexityConsumedBudget = ComplexityBudget.getConsumedBudget();
        DebugInfo->ComplexityUnusedBudget = ComplexityBudget.getUnusedBudget();
        DebugInfo->ComplexityCategoryAllocations = ComplexityBudget.getAllocations();
        DebugInfo->ComplexityCategoryConsumed = ComplexityBudget.getConsumedByCategory();
    }

    void ShipGenerationContext::resetSpatialBudget()
    {
        SpatialBudget.initialize(Ship.HullMask, WingRegions.WingMask, WingRegions.WingRootMask, WingRegions.OuterWingMask, ScaleTraits, Profile);
        updateSpatialBudgetDebugInfo();
    }

    void ShipGenerationContext::updateSpatialBudgetDebugInfo()
    {
        if (DebugInfo == nullptr)
        {
            return;
        }

        DebugInfo->SpatialRegionMapWidth = SpatialBudget.getWidth();
        DebugInfo->SpatialRegionMapHeight = SpatialBudget.getHeight();
        DebugInfo->SpatialRegionMap = SpatialBudget.getRegionMap();
        DebugInfo->SpatialOverloadRejectionCount = SpatialBudget.getTotalRejectionCount();
        const auto& states = SpatialBudget.getRegionStates();
        for (std::size_t index = 0u; index < states.size(); ++index)
        {
            DebugInfo->SpatialRegionAreas[index] = states[index].AreaPixels;
            DebugInfo->SpatialRegionCapacities[index] = states[index].Capacity;
            DebugInfo->SpatialRegionLoads[index] = states[index].Load;
            DebugInfo->SpatialRegionDominantCounts[index] = states[index].DominantFeatureCount;
            DebugInfo->SpatialRegionRejections[index] = states[index].RejectionCount;
        }
    }

    void ShipGenerationContext::updateVisualHierarchyDebugInfo()
    {
        if (DebugInfo == nullptr) { return; }
        DebugInfo->PrimaryVisualAnchor = VisualHierarchy.PrimaryAnchor;
        DebugInfo->SecondaryVisualAnchor = VisualHierarchy.SecondaryAnchor;
        DebugInfo->VisualAnchorTargetRegion = VisualHierarchy.TargetRegion;
        DebugInfo->VisualHierarchyReservedComplexity = ComplexityBudget.getHierarchyReservedBudget();
        DebugInfo->VisualHierarchyFallbackOccurred = VisualHierarchy.FallbackOccurred;
    }

    void ShipGenerationContext::updateMaterialCompositionDebugInfo()
    {
        if (DebugInfo == nullptr) { return; }
        DebugInfo->MaterialZoneCount = static_cast<uint32_t>(MaterialComposition.Placements.size());
        DebugInfo->MaterialSecondaryHullPixelCount = PixelMaskUtils::getMaskPixelCount(MaterialComposition.SecondaryHullMask);
        DebugInfo->MaterialMechanicalPixelCount = PixelMaskUtils::getMaskPixelCount(MaterialComposition.MechanicalMask);
        DebugInfo->MaterialZoneTypeCounts = MaterialComposition.TypeCounts;
        DebugInfo->MaterialSecondaryHullMask = MaterialComposition.SecondaryHullMask;
        DebugInfo->MaterialMechanicalMask = MaterialComposition.MechanicalMask;
    }

    void ShipGenerationContext::updateLiveryDebugInfo()
    {
        if (DebugInfo == nullptr) { return; }
        DebugInfo->LiveryMarkingCount = static_cast<uint32_t>(Livery.Placements.size());
        DebugInfo->LiveryPrimaryPixelCount = PixelMaskUtils::getMaskPixelCount(Livery.PrimaryMarkingMask);
        DebugInfo->LiverySecondaryPixelCount = PixelMaskUtils::getMaskPixelCount(Livery.SecondaryMarkingMask);
        PixelMask combinedLivery = Livery.PrimaryMarkingMask;
        PixelMaskUtils::mergeMask(combinedLivery, Livery.SecondaryMarkingMask);
        const uint32_t hullPixels = PixelMaskUtils::getMaskPixelCount(Ship.HullMask);
        const uint32_t combinedPixels = PixelMaskUtils::getMaskPixelCount(combinedLivery);
        const uint32_t largestConnectedPixels = PixelMaskUtils::getLargestConnectedMaskPixelCount(combinedLivery);
        DebugInfo->LiveryPrimaryCoveragePermille = hullPixels == 0u ? 0u : static_cast<uint32_t>((static_cast<uint64_t>(DebugInfo->LiveryPrimaryPixelCount) * 1000u + hullPixels / 2u) / hullPixels);
        DebugInfo->LiverySecondaryCoveragePermille = hullPixels == 0u ? 0u : static_cast<uint32_t>((static_cast<uint64_t>(DebugInfo->LiverySecondaryPixelCount) * 1000u + hullPixels / 2u) / hullPixels);
        DebugInfo->LiveryCoveragePermille = hullPixels == 0u ? 0u : static_cast<uint32_t>((static_cast<uint64_t>(combinedPixels) * 1000u + hullPixels / 2u) / hullPixels);
        DebugInfo->LiveryLargestConnectedCoveragePermille = hullPixels == 0u ? 0u : static_cast<uint32_t>((static_cast<uint64_t>(largestConnectedPixels) * 1000u + hullPixels / 2u) / hullPixels);
        const uint32_t secondaryMaterialPixels = PixelMaskUtils::getMaskPixelCount(MaterialComposition.SecondaryHullMask);
        const uint32_t mechanicalMaterialPixels = PixelMaskUtils::getMaskPixelCount(MaterialComposition.MechanicalMask);
        const uint32_t secondaryMaterialOverlap = PixelMaskUtils::getMaskOverlapPixelCount(combinedLivery, MaterialComposition.SecondaryHullMask);
        const uint32_t mechanicalMaterialOverlap = PixelMaskUtils::getMaskOverlapPixelCount(combinedLivery, MaterialComposition.MechanicalMask);
        DebugInfo->LiverySecondaryMaterialCoveragePermille = secondaryMaterialPixels == 0u ? 0u : static_cast<uint32_t>((static_cast<uint64_t>(secondaryMaterialOverlap) * 1000u + secondaryMaterialPixels / 2u) / secondaryMaterialPixels);
        DebugInfo->LiveryMechanicalMaterialCoveragePermille = mechanicalMaterialPixels == 0u ? 0u : static_cast<uint32_t>((static_cast<uint64_t>(mechanicalMaterialOverlap) * 1000u + mechanicalMaterialPixels / 2u) / mechanicalMaterialPixels);
        DebugInfo->LiveryCoverageRejectionCount = Livery.CoverageRejectionCount;
        DebugInfo->LiveryMaterialPreservationRejectionCount = Livery.MaterialPreservationRejectionCount;
        DebugInfo->LiveryTypeCounts = Livery.TypeCounts;
        DebugInfo->LiveryPrimaryMask = Livery.PrimaryMarkingMask;
        DebugInfo->LiverySecondaryMask = Livery.SecondaryMarkingMask;
    }

    void ShipGenerationContext::updateDetailMotifDebugInfo()
    {
        if (DebugInfo == nullptr) { return; }
        DebugInfo->PrimaryDetailMotif = DetailMotifs.PrimaryMotif;
        DebugInfo->SecondaryDetailMotif = DetailMotifs.SecondaryMotif;
        DebugInfo->PrimaryDetailMotifRegion = DetailMotifs.PrimaryPreferredRegion;
        DebugInfo->SecondaryDetailMotifRegion = DetailMotifs.SecondaryPreferredRegion;
        DebugInfo->PrimaryDetailMotifOccurrenceCount = DetailMotifs.PrimaryOccurrences;
        DebugInfo->SecondaryDetailMotifOccurrenceCount = DetailMotifs.SecondaryOccurrences;
        DebugInfo->DetailMotifRejectedPlacementCount = DetailMotifs.RejectedPlacements;
        DebugInfo->PrimaryDetailMotifMask = DetailMotifs.PrimaryMask;
        DebugInfo->SecondaryDetailMotifMask = DetailMotifs.SecondaryMask;
    }

    uint32_t ShipGenerationContext::getRandomUInt(std::mt19937_64& randomGenerator, uint32_t minimum, uint32_t maximum)
    {
        if (minimum == maximum)
        {
            return minimum;
        }

        std::uniform_int_distribution<uint32_t> distribution(minimum, maximum);
        return distribution(randomGenerator);
    }

    uint64_t ShipGenerationContext::getRandomUInt64(std::mt19937_64& randomGenerator, uint64_t minimum, uint64_t maximum)
    {
        if (minimum == maximum)
        {
            return minimum;
        }

        std::uniform_int_distribution<uint64_t> distribution(minimum, maximum);
        return distribution(randomGenerator);
    }

    std::mt19937_64& ShipGenerationContext::getDomainRandomGenerator(GenerationDomain domain)
    {
        const std::size_t index = static_cast<std::size_t>(domain);
        if (index >= m_DomainRandomGenerators.size())
        {
            throw std::invalid_argument("Unknown GenerationDomain value.");
        }
        return m_DomainRandomGenerators[index];
    }

    std::mt19937_64& ShipGenerationContext::getLegacyRandomGenerator(GenerationSeedChannel channel)
    {
        switch (channel)
        {
        case GenerationSeedChannel::STRUCTURE: return m_LegacyStructureRandomGenerator;
        case GenerationSeedChannel::PALETTE: return m_LegacyPaletteRandomGenerator;
        case GenerationSeedChannel::DETAILS: return m_LegacyDetailRandomGenerator;
        case GenerationSeedChannel::ATTACHMENTS: return m_LegacyAttachmentRandomGenerator;
        default: throw std::invalid_argument("Unknown GenerationSeedChannel value.");
        }
    }
}
