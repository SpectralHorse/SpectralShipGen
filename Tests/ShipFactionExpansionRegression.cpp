#include "CoreRegressionSuites.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include <SpectralShipGen/Diagnostics/GenerationStatistics.h>
#include <SpectralShipGen/ShipCockpitType.h>
#include <SpectralShipGen/ShipCoreTreatmentType.h>
#include <SpectralShipGen/ShipFactionPaletteProfile.h>
#include <SpectralShipGen/ShipFactionType.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipSurfaceDetailProfile.h>
#include <SpectralShipGen/ShipWeaponType.h>

namespace
{
    using SpectralShipGenDiagnostics::DiagnosticGenerationConfiguration;
    using SpectralShipGenDiagnostics::GenerationStatistics;

    constexpr std::array<SpectralShipGen::ShipFactionType, static_cast<std::size_t>(SpectralShipGen::ShipFactionType::SHIP_FACTION_TYPE_END)> Factions = {
        SpectralShipGen::ShipFactionType::FRONTIER,
        SpectralShipGen::ShipFactionType::MILITARY,
        SpectralShipGen::ShipFactionType::ASCENDANT,
        SpectralShipGen::ShipFactionType::XENO,
        SpectralShipGen::ShipFactionType::CORPORATE,
        SpectralShipGen::ShipFactionType::RELIC
    };

    constexpr std::array<SpectralShipGen::ShipDimensions, 6u> NativeReviewDimensions = { {
        { 32u, 32u },
        { 44u, 44u },
        { 48u, 64u },
        { 64u, 48u },
        { 64u, 64u },
        { 96u, 96u }
    } };

    template <typename T>
    constexpr std::size_t index(T value)
    {
        return static_cast<std::size_t>(value);
    }

    double percent(uint64_t count, uint64_t total)
    {
        return total == 0u ? 0.0 : 100.0 * static_cast<double>(count) / static_cast<double>(total);
    }

    uint64_t totalWeapons(const GenerationStatistics& statistics)
    {
        uint64_t total = 0u;
        for (uint64_t count : statistics.WeaponTypeCounts) { total += count; }
        return total;
    }

    uint64_t totalEngines(const GenerationStatistics& statistics)
    {
        uint64_t total = 0u;
        for (uint64_t count : statistics.EngineSizeCounts) { total += count; }
        return total;
    }

    GenerationStatistics collectFaction(SpectralShipGen::ShipFactionType faction, uint64_t samples = 180u)
    {
        DiagnosticGenerationConfiguration configuration;
        configuration.Width = 64u;
        configuration.Height = 64u;
        configuration.Style = SpectralShipGen::ShipStyle::FIGHTER;
        configuration.Faction = faction;
        configuration.DetailDensity = 62u;
        configuration.AsymmetricDetailChance = 14u;
        configuration.Samples = samples;
        configuration.DiagnosticSeed = 0x55FAC710C0FFEE11ull;
        return SpectralShipGenDiagnostics::collectGenerationStatistics(configuration);
    }

    bool validateSameSeedNativeDistinctness()
    {
        SpectralShipGen::ShipGenerator generator;

        for (const SpectralShipGen::ShipDimensions dimensions : NativeReviewDimensions)
        {
            std::array<SpectralShipGen::GeneratedShip, Factions.size()> ships;

            for (std::size_t factionIndex = 0u; factionIndex < Factions.size(); ++factionIndex)
            {
                SpectralShipGen::ShipGenerationSettings settings;
                settings.Seed = 0x55D1571AC7000000ull + static_cast<uint64_t>(dimensions.Width) * 1000ull + dimensions.Height;
                settings.Dimensions = dimensions;
                settings.Style = SpectralShipGen::ShipStyle::FIGHTER;
                settings.Faction = Factions[factionIndex];
                ships[factionIndex] = generator.generate(settings);

                if (Factions[factionIndex] == SpectralShipGen::ShipFactionType::CORPORATE || Factions[factionIndex] == SpectralShipGen::ShipFactionType::RELIC)
                {
                    const SpectralShipGen::GeneratedShip repeat = generator.generate(settings);
                    if (repeat.FinalImage.getPixels() != ships[factionIndex].FinalImage.getPixels())
                    {
                        std::cerr << "New faction determinism failed at " << dimensions.Width << 'x' << dimensions.Height << ".\n";
                        return false;
                    }
                }
            }

            const std::size_t corporate = index(SpectralShipGen::ShipFactionType::CORPORATE);
            const std::size_t relic = index(SpectralShipGen::ShipFactionType::RELIC);
            if (ships[corporate].FinalImage.getPixels() == ships[relic].FinalImage.getPixels())
            {
                std::cerr << "CORPORATE and RELIC matched at " << dimensions.Width << 'x' << dimensions.Height << ".\n";
                return false;
            }

            for (std::size_t factionIndex = 0u; factionIndex < 4u; ++factionIndex)
            {
                if (ships[corporate].FinalImage.getPixels() == ships[factionIndex].FinalImage.getPixels() || ships[relic].FinalImage.getPixels() == ships[factionIndex].FinalImage.getPixels())
                {
                    std::cerr << "A new faction matched an established faction at " << dimensions.Width << 'x' << dimensions.Height << ".\n";
                    return false;
                }
            }
        }

        return true;
    }
}

int SpectralShipGenTests::runFactionExpansionRegression()
{
    using namespace SpectralShipGen;

    static_assert(static_cast<std::size_t>(ShipFactionType::SHIP_FACTION_TYPE_END) == 6u, "Task 55 expects six factions.");

    const ShipFactionPaletteProfile& corporatePalette = getShipFactionPaletteProfile(ShipFactionType::CORPORATE);
    const ShipFactionPaletteProfile& relicPalette = getShipFactionPaletteProfile(ShipFactionType::RELIC);
    if (corporatePalette.Accent.Saturation.Min < 65u || corporatePalette.MechanicalValue.Max >= corporatePalette.HullValue.Max || relicPalette.HullValue.Max > 50u || relicPalette.Light.Value.Min < 75u)
    {
        std::cerr << "Task 55 palette profiles do not preserve Corporate/Relic material identities.\n";
        return 1;
    }

    const ShipFactionSurfaceDetailProfile& corporateDetails = getShipFactionSurfaceDetailProfile(ShipFactionType::CORPORATE);
    const ShipFactionSurfaceDetailProfile& relicDetails = getShipFactionSurfaceDetailProfile(ShipFactionType::RELIC);
    if (corporateDetails.SupplementalWeightMultipliersPercent.IdentificationMarking <= corporateDetails.SupplementalWeightMultipliersPercent.LuminousChannel * 4u ||
        relicDetails.SupplementalWeightMultipliersPercent.LuminousChannel <= relicDetails.SupplementalWeightMultipliersPercent.IdentificationMarking * 10u)
    {
        std::cerr << "Task 55 surface-detail languages are not faction-specific enough.\n";
        return 1;
    }

    for (uint32_t faction = 0u; faction < 4u; ++faction)
    {
        const ShipFactionSurfaceDetailProfile& profile = getShipFactionSurfaceDetailProfile(static_cast<ShipFactionType>(faction));
        if (profile.SupplementalWeightMultipliersPercent.IdentificationMarking != 0u || profile.SupplementalWeightMultipliersPercent.LuminousChannel != 0u)
        {
            std::cerr << "Task 55 detail motifs leaked into an established faction.\n";
            return 1;
        }
    }

    if (!validateSameSeedNativeDistinctness())
    {
        return 1;
    }

    constexpr uint64_t Samples = 180u;
    const GenerationStatistics frontier = collectFaction(ShipFactionType::FRONTIER, Samples);
    const GenerationStatistics corporate = collectFaction(ShipFactionType::CORPORATE, Samples);
    const GenerationStatistics relic = collectFaction(ShipFactionType::RELIC, Samples);
    if (frontier.SuccessfulGenerations != Samples || corporate.SuccessfulGenerations != Samples || relic.SuccessfulGenerations != Samples)
    {
        std::cerr << "Task 55 statistics comparison generation failure.\n";
        return 1;
    }

    const uint64_t corporateIdentification = corporate.SupplementalSurfaceDetailCounts[index(SupplementalSurfaceDetailType::IDENTIFICATION_MARKING)];
    const uint64_t corporateLuminous = corporate.SupplementalSurfaceDetailCounts[index(SupplementalSurfaceDetailType::LUMINOUS_CHANNEL)];
    const uint64_t relicIdentification = relic.SupplementalSurfaceDetailCounts[index(SupplementalSurfaceDetailType::IDENTIFICATION_MARKING)];
    const uint64_t relicLuminous = relic.SupplementalSurfaceDetailCounts[index(SupplementalSurfaceDetailType::LUMINOUS_CHANNEL)];
    if (!(corporateIdentification > corporateLuminous * 3u && relicLuminous > relicIdentification * 5u && corporateIdentification > 0u && relicLuminous > 0u))
    {
        std::cerr << "Generated detail statistics do not distinguish Corporate markings from Relic luminous channels.\n";
        return 1;
    }

    const uint64_t corporateAncientBridge = corporate.CockpitShapeCounts[index(CockpitShapeType::DORSAL_BRIDGE)] + corporate.CockpitShapeCounts[index(CockpitShapeType::LAYERED_BRIDGE)];
    const uint64_t relicAncientBridge = relic.CockpitShapeCounts[index(CockpitShapeType::DORSAL_BRIDGE)] + relic.CockpitShapeCounts[index(CockpitShapeType::LAYERED_BRIDGE)];
    if (!(percent(relicAncientBridge, relic.CockpitPlacementSuccessCount) > percent(corporateAncientBridge, corporate.CockpitPlacementSuccessCount) + 8.0))
    {
        std::cerr << "Relic bridge vocabulary is not appearing strongly enough.\n";
        return 1;
    }

    if (!(relic.HullLayerCount.average() > corporate.HullLayerCount.average() * 1.10 && relic.HullLayerPixelCount.average() > corporate.HullLayerPixelCount.average() * 1.10))
    {
        std::cerr << "Relic hull layers are not reading as broader/more dominant than Corporate modular armor.\n";
        return 1;
    }

    const uint64_t corporateCoreChannels = corporate.CoreTreatmentTypeCounts[index(ShipCoreTreatmentType::CORE_CHANNEL)] + corporate.CoreTreatmentTypeCounts[index(ShipCoreTreatmentType::CENTRAL_SPINE)];
    const uint64_t relicCoreChannels = relic.CoreTreatmentTypeCounts[index(ShipCoreTreatmentType::CORE_CHANNEL)] + relic.CoreTreatmentTypeCounts[index(ShipCoreTreatmentType::CENTRAL_SPINE)];
    if (!(relic.CoreTreatmentCount.average() > corporate.CoreTreatmentCount.average() && relicCoreChannels > corporateCoreChannels && relic.CoreLuminousPixelCount.average() > corporate.CoreLuminousPixelCount.average()))
    {
        std::cerr << "Task 53 core treatment is not giving RELIC stronger central/luminous hierarchy.\n";
        return 1;
    }

    const uint64_t corporateEngines = totalEngines(corporate);
    const uint64_t relicEngines = totalEngines(relic);
    const double corporateNacelles = percent(corporate.NacelleEngineCount, corporateEngines);
    const double relicNacelles = percent(relic.NacelleEngineCount, relicEngines);
    const double corporateLargeEngines = percent(corporate.EngineSizeCounts[index(EngineSizeClass::LARGE)], corporateEngines);
    const double relicLargeEngines = percent(relic.EngineSizeCounts[index(EngineSizeClass::LARGE)], relicEngines);
    if (!(corporateNacelles > relicNacelles + 12.0 && relicLargeEngines > corporateLargeEngines + 12.0))
    {
        std::cerr << "Engine distributions do not distinguish clean Corporate nacelles from large integrated Relic propulsion.\n";
        return 1;
    }

    const uint64_t corporateWeapons = totalWeapons(corporate);
    const uint64_t relicWeapons = totalWeapons(relic);
    const double corporateModularWeapons = percent(corporate.WeaponTypeCounts[index(ShipWeaponType::TWIN_CANNON)] + corporate.WeaponTypeCounts[index(ShipWeaponType::WEAPON_POD)], corporateWeapons);
    const double relicModularWeapons = percent(relic.WeaponTypeCounts[index(ShipWeaponType::TWIN_CANNON)] + relic.WeaponTypeCounts[index(ShipWeaponType::WEAPON_POD)], relicWeapons);
    const double corporateIntegratedWeapons = percent(corporate.WeaponTypeCounts[index(ShipWeaponType::COMPACT_TURRET)] + corporate.WeaponTypeCounts[index(ShipWeaponType::RAIL_WEAPON)], corporateWeapons);
    const double relicIntegratedWeapons = percent(relic.WeaponTypeCounts[index(ShipWeaponType::COMPACT_TURRET)] + relic.WeaponTypeCounts[index(ShipWeaponType::RAIL_WEAPON)], relicWeapons);
    if (!(corporateModularWeapons > relicModularWeapons + 8.0 && relicIntegratedWeapons > corporateIntegratedWeapons + 8.0))
    {
        std::cerr << "Weapon distributions do not reinforce Corporate modular mounts and Relic integrated emitters.\n";
        return 1;
    }

    if (!(corporate.MacroAsymmetryPlannedCount < frontier.MacroAsymmetryPlannedCount && relic.MacroAsymmetryPlannedCount > corporate.MacroAsymmetryPlannedCount))
    {
        std::cerr << "Macro-asymmetry tendencies do not preserve Corporate discipline / Relic monumentality.\n";
        return 1;
    }

    std::cout << "Task 55 faction expansion regression passed.\n";
    return 0;
}
