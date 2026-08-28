#include "RegressionSuites.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "GenerationStatistics.h"
#include "ShipCoreTreatmentType.h"
#include "ShipFactionType.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSettings.h"
#include "ShipGenerator.h"

namespace
{
    using PixelShipGenerator::ShipStyle;
    using PixelShipGeneratorDiagnostics::DiagnosticGenerationConfiguration;
    using PixelShipGeneratorDiagnostics::GenerationStatistics;

    constexpr std::array<ShipStyle, static_cast<std::size_t>(ShipStyle::SHIP_STYLE_END)> Styles = {
        ShipStyle::SLEEK,
        ShipStyle::FIGHTER,
        ShipStyle::HEAVY,
        ShipStyle::INDUSTRIAL,
        ShipStyle::SPEARHEAD,
        ShipStyle::DELTA
    };

    constexpr std::array<PixelShipGenerator::ShipDimensions, 6u> NativeReviewDimensions = { {
        { 32u, 32u },
        { 44u, 44u },
        { 48u, 64u },
        { 64u, 48u },
        { 64u, 64u },
        { 96u, 96u }
    } };

    constexpr std::size_t index(PixelShipGenerator::ShipCoreTreatmentType type)
    {
        return static_cast<std::size_t>(type);
    }

    constexpr std::size_t index(PixelShipGenerator::WingShapeType type)
    {
        return static_cast<std::size_t>(type);
    }

    constexpr std::size_t index(PixelShipGenerator::CockpitShapeType type)
    {
        return static_cast<std::size_t>(type);
    }

    constexpr std::size_t index(PixelShipGenerator::ShipMajorFeatureType type)
    {
        return static_cast<std::size_t>(type);
    }

    constexpr std::size_t index(PixelShipGenerator::ShipWeaponType type)
    {
        return static_cast<std::size_t>(type);
    }

    constexpr std::size_t index(PixelShipGenerator::ShipWeaponHardpointRegion type)
    {
        return static_cast<std::size_t>(type);
    }

    constexpr std::size_t index(PixelShipGenerator::EngineLayoutType type)
    {
        return static_cast<std::size_t>(type);
    }

    double percent(uint64_t count, uint64_t total)
    {
        return total == 0u ? 0.0 : 100.0 * static_cast<double>(count) / static_cast<double>(total);
    }

    uint64_t totalWeaponUnits(const GenerationStatistics& statistics)
    {
        uint64_t total = 0u;
        for (const uint64_t count : statistics.WeaponTypeCounts) { total += count; }
        return total;
    }

    GenerationStatistics collectStyle(ShipStyle style, uint32_t width, uint32_t height, uint64_t samples)
    {
        DiagnosticGenerationConfiguration configuration;
        configuration.Width = width;
        configuration.Height = height;
        configuration.Style = style;
        configuration.Faction = PixelShipGenerator::ShipFactionType::MILITARY;
        configuration.Samples = samples;
        configuration.DiagnosticSeed = 0x54A11E5EEDBADC0Full;
        return PixelShipGeneratorDiagnostics::collectGenerationStatistics(configuration);
    }

    bool validateNativeSameSeedDistinctness()
    {
        PixelShipGenerator::ShipGenerator generator;

        for (const PixelShipGenerator::ShipDimensions dimensions : NativeReviewDimensions)
        {
            std::array<PixelShipGenerator::GeneratedShip, Styles.size()> ships;

            for (std::size_t styleIndex = 0u; styleIndex < Styles.size(); ++styleIndex)
            {
                PixelShipGenerator::ShipGenerationSettings settings;
                settings.Seed = 0x54C0FFEE00000000ull + static_cast<uint64_t>(dimensions.Width) * 1000ull + dimensions.Height;
                settings.Dimensions = dimensions;
                settings.Style = Styles[styleIndex];
                settings.Faction = PixelShipGenerator::ShipFactionType::MILITARY;
                ships[styleIndex] = generator.generate(settings);

                const PixelShipGenerator::GeneratedShip repeat = generator.generate(settings);
                if (repeat.FinalImage.getPixels() != ships[styleIndex].FinalImage.getPixels())
                {
                    std::cerr << "Style determinism failed at " << dimensions.Width << 'x' << dimensions.Height << ".\n";
                    return false;
                }
            }

            const std::size_t spearhead = static_cast<std::size_t>(ShipStyle::SPEARHEAD);
            const std::size_t delta = static_cast<std::size_t>(ShipStyle::DELTA);
            for (std::size_t styleIndex = 0u; styleIndex < Styles.size(); ++styleIndex)
            {
                if (styleIndex != spearhead && ships[spearhead].FinalImage.getPixels() == ships[styleIndex].FinalImage.getPixels())
                {
                    std::cerr << "SPEARHEAD matched another style at " << dimensions.Width << 'x' << dimensions.Height << ".\n";
                    return false;
                }
                if (styleIndex != delta && ships[delta].FinalImage.getPixels() == ships[styleIndex].FinalImage.getPixels())
                {
                    std::cerr << "DELTA matched another style at " << dimensions.Width << 'x' << dimensions.Height << ".\n";
                    return false;
                }
            }
        }

        return true;
    }
}

int PixelShipGeneratorTests::runStyleExpansionRegression()
{
    static_assert(static_cast<std::size_t>(ShipStyle::SHIP_STYLE_END) == 6u, "Task 54 expects six styles.");

    const auto& spearProfile = PixelShipGenerator::getShipGenerationProfile(ShipStyle::SPEARHEAD);
    const auto& deltaProfile = PixelShipGenerator::getShipGenerationProfile(ShipStyle::DELTA);
    if (spearProfile.SweptWingWeight <= spearProfile.BroadWingWeight || spearProfile.WingLongitudinalOffsetPercent <= 0 ||
        deltaProfile.BroadWingWeight <= deltaProfile.SweptWingWeight || deltaProfile.WingLongitudinalOffsetPercent >= 0)
    {
        std::cerr << "Task 54 profile identity regression.\n";
        return 1;
    }

    if (!validateNativeSameSeedDistinctness())
    {
        return 1;
    }

    constexpr uint64_t Samples = 160u;
    const GenerationStatistics sleek = collectStyle(ShipStyle::SLEEK, 64u, 64u, Samples);
    const GenerationStatistics heavy = collectStyle(ShipStyle::HEAVY, 64u, 64u, Samples);
    const GenerationStatistics spear = collectStyle(ShipStyle::SPEARHEAD, 64u, 64u, Samples);
    const GenerationStatistics delta = collectStyle(ShipStyle::DELTA, 64u, 64u, Samples);

    if (sleek.SuccessfulGenerations != Samples || heavy.SuccessfulGenerations != Samples || spear.SuccessfulGenerations != Samples || delta.SuccessfulGenerations != Samples)
    {
        std::cerr << "Style comparison generation failure.\n";
        return 1;
    }

    const double spearAspect = spear.HullNormalizedHeight.average() / spear.HullNormalizedWidth.average();
    const double sleekAspect = sleek.HullNormalizedHeight.average() / sleek.HullNormalizedWidth.average();
    if (!(spear.HullNormalizedHeight.average() > sleek.HullNormalizedHeight.average() + 0.05 && spearAspect > sleekAspect * 1.04))
    {
        std::cerr << "SPEARHEAD did not establish a stronger longitudinal silhouette.\n";
        return 1;
    }

    if (!(spear.WingStartNormalizedY.average() > sleek.WingStartNormalizedY.average() + 0.08 &&
        percent(spear.WingShapeCounts[index(PixelShipGenerator::WingShapeType::SWEPT)], spear.SuccessfulGenerations) > 55.0))
    {
        std::cerr << "SPEARHEAD wing identity is too close to the existing styles.\n";
        return 1;
    }

    if (!(delta.HullNormalizedWidth.average() > heavy.HullNormalizedWidth.average() + 0.045 &&
        delta.HullNormalizedHeight.average() < heavy.HullNormalizedHeight.average() - 0.035 &&
        delta.WingMaximumSpan.average() > heavy.WingMaximumSpan.average() + 4.0 &&
        percent(delta.WingShapeCounts[index(PixelShipGenerator::WingShapeType::BROAD)], delta.SuccessfulGenerations) > 60.0))
    {
        std::cerr << "DELTA did not establish a broad wing-dominant silhouette.\n";
        return 1;
    }

    const double spearElongatedCockpit = percent(spear.CockpitShapeCounts[index(PixelShipGenerator::CockpitShapeType::ELONGATED_CANOPY)], spear.CockpitPlacementSuccessCount);
    const double deltaWideCockpit = percent(delta.CockpitShapeCounts[index(PixelShipGenerator::CockpitShapeType::WIDE_COMMAND_DECK)], delta.CockpitPlacementSuccessCount) +
        percent(delta.CockpitShapeCounts[index(PixelShipGenerator::CockpitShapeType::SPLIT_CANOPY)], delta.CockpitPlacementSuccessCount);
    if (!(spearElongatedCockpit > 55.0 && deltaWideCockpit > 50.0 && delta.CockpitNormalizedWidth.average() > spear.CockpitNormalizedWidth.average() * 2.0))
    {
        std::cerr << "Task 52 cockpit vocabulary is not differentiating the new styles strongly enough.\n";
        return 1;
    }

    const uint64_t spearAxialCore = spear.CoreTreatmentTypeCounts[index(PixelShipGenerator::ShipCoreTreatmentType::CENTRAL_SPINE)] +
        spear.CoreTreatmentTypeCounts[index(PixelShipGenerator::ShipCoreTreatmentType::LONGITUDINAL_ARMOR_BAND)];
    const uint64_t deltaAxialCore = delta.CoreTreatmentTypeCounts[index(PixelShipGenerator::ShipCoreTreatmentType::CENTRAL_SPINE)] +
        delta.CoreTreatmentTypeCounts[index(PixelShipGenerator::ShipCoreTreatmentType::LONGITUDINAL_ARMOR_BAND)];
    const uint64_t deltaBroadCore = delta.CoreTreatmentTypeCounts[index(PixelShipGenerator::ShipCoreTreatmentType::RAISED_CORE_PLATE)] +
        delta.CoreTreatmentTypeCounts[index(PixelShipGenerator::ShipCoreTreatmentType::LATERAL_RECESSES)] +
        delta.CoreTreatmentTypeCounts[index(PixelShipGenerator::ShipCoreTreatmentType::COCKPIT_SURROUND)];
    const uint64_t spearBroadCore = spear.CoreTreatmentTypeCounts[index(PixelShipGenerator::ShipCoreTreatmentType::RAISED_CORE_PLATE)] +
        spear.CoreTreatmentTypeCounts[index(PixelShipGenerator::ShipCoreTreatmentType::LATERAL_RECESSES)] +
        spear.CoreTreatmentTypeCounts[index(PixelShipGenerator::ShipCoreTreatmentType::COCKPIT_SURROUND)];
    if (!(spearAxialCore > deltaAxialCore * 2u && deltaBroadCore > spearBroadCore * 2u))
    {
        std::cerr << "Task 53 core treatments are not style-distinct enough.\n";
        return 1;
    }

    if (!(spear.MajorFeatureTypeCounts[index(PixelShipGenerator::ShipMajorFeatureType::CENTRAL_SPINE)] > delta.MajorFeatureTypeCounts[index(PixelShipGenerator::ShipMajorFeatureType::CENTRAL_SPINE)] &&
        delta.MajorFeatureTypeCounts[index(PixelShipGenerator::ShipMajorFeatureType::WING_PLATE)] > spear.MajorFeatureTypeCounts[index(PixelShipGenerator::ShipMajorFeatureType::WING_PLATE)] * 3u))
    {
        std::cerr << "Major Feature distributions do not reinforce the new structural identities.\n";
        return 1;
    }

    const uint64_t spearWeaponUnits = totalWeaponUnits(spear);
    const uint64_t deltaWeaponUnits = totalWeaponUnits(delta);
    const double spearRail = percent(spear.WeaponTypeCounts[index(PixelShipGenerator::ShipWeaponType::RAIL_WEAPON)], spearWeaponUnits);
    const double deltaRail = percent(delta.WeaponTypeCounts[index(PixelShipGenerator::ShipWeaponType::RAIL_WEAPON)], deltaWeaponUnits);
    const uint64_t spearLateralCount = spear.WeaponRegionCounts[index(PixelShipGenerator::ShipWeaponHardpointRegion::WING_ROOT)] +
        spear.WeaponRegionCounts[index(PixelShipGenerator::ShipWeaponHardpointRegion::OUTER_WING)] +
        spear.WeaponRegionCounts[index(PixelShipGenerator::ShipWeaponHardpointRegion::FORWARD_SHOULDER)];
    const uint64_t deltaLateralCount = delta.WeaponRegionCounts[index(PixelShipGenerator::ShipWeaponHardpointRegion::WING_ROOT)] +
        delta.WeaponRegionCounts[index(PixelShipGenerator::ShipWeaponHardpointRegion::OUTER_WING)] +
        delta.WeaponRegionCounts[index(PixelShipGenerator::ShipWeaponHardpointRegion::FORWARD_SHOULDER)];
    if (!(spearRail > deltaRail + 15.0 && percent(deltaLateralCount, deltaWeaponUnits) > percent(spearLateralCount, spearWeaponUnits) + 15.0))
    {
        std::cerr << "Weapon type/region distributions do not reinforce SPEARHEAD vs DELTA.\n";
        return 1;
    }

    const double spearBroadPropulsion = percent(spear.EngineLayoutCounts[index(PixelShipGenerator::EngineLayoutType::QUAD)] + spear.EngineLayoutCounts[index(PixelShipGenerator::EngineLayoutType::WIDE_BANK)], spear.SuccessfulGenerations);
    const double deltaBroadPropulsion = percent(delta.EngineLayoutCounts[index(PixelShipGenerator::EngineLayoutType::QUAD)] + delta.EngineLayoutCounts[index(PixelShipGenerator::EngineLayoutType::WIDE_BANK)], delta.SuccessfulGenerations);
    if (!(deltaBroadPropulsion > spearBroadPropulsion + 35.0))
    {
        std::cerr << "Engine layout distributions do not distinguish axial and broad propulsion.\n";
        return 1;
    }

    const GenerationStatistics spearTall = collectStyle(ShipStyle::SPEARHEAD, 48u, 64u, 64u);
    const GenerationStatistics deltaWide = collectStyle(ShipStyle::DELTA, 64u, 48u, 64u);
    if (spearTall.SuccessfulGenerations != 64u || deltaWide.SuccessfulGenerations != 64u ||
        spearTall.HullNormalizedHeight.average() < 0.84 || deltaWide.HullNormalizedWidth.average() < 0.77)
    {
        std::cerr << "Rectangular-dimension style response regression.\n";
        return 1;
    }

    std::cout << "SPEARHEAD / DELTA style expansion regression passed.\n";
    return 0;
}
