#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include <SpectralShipGen/ShipAttachmentProfile.h>
#include <SpectralShipGen/ShipFactionPaletteProfile.h>
#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipFactionProfileValidation.h>
#include <SpectralShipGen/ShipSurfaceDetailProfile.h>

namespace
{
    using namespace SpectralShipGen;

    struct ExpectedFactionCoreData
    {
        uint32_t WeaponChance;
        int32_t WeaponSymmetryOffset;
        std::array<uint32_t, 5u> WeaponWeights;
        uint32_t WeaponEmissiveChance;
        std::array<uint32_t, 5u> EngineLayoutWeights;
        std::array<uint32_t, 3u> EngineSizeWeights;
        uint32_t EngineNacelleChance;
        uint32_t EngineExternalHeight;
        uint32_t MajorFeatureChance;
        std::array<uint32_t, 6u> MajorFeatureWeights;
        uint32_t NegativeSpaceChance;
        uint32_t MacroAsymmetryChance;
        uint32_t ComplexityBudgetPercent;
    };

    constexpr std::array<ExpectedFactionCoreData, 6u> Expected = {{
        { 100u, -15, { 125u, 85u, 90u, 70u, 135u }, 10u, { 100u, 100u, 100u, 100u, 100u }, { 100u, 100u, 100u }, 100u, 100u, 110u, { 80u, 120u, 130u, 130u, 100u, 50u }, 110u, 125u, 106u },
        { 120u,  15, { 120u, 145u, 105u, 110u, 115u }, 20u, { 100u, 100u, 100u, 100u, 100u }, { 100u, 100u, 100u }, 100u, 100u, 100u, { 100u, 130u, 90u, 110u, 120u, 70u }, 95u, 65u, 98u },
        {  85u,  10, { 60u, 80u, 125u, 165u, 55u }, 80u, { 100u, 100u, 100u, 100u, 100u }, { 100u, 100u, 100u }, 100u, 100u, 90u, { 130u, 80u, 60u, 50u, 90u, 160u }, 88u, 72u, 92u },
        { 100u,  -5, { 75u, 95u, 135u, 145u, 115u }, 65u, { 100u, 100u, 100u, 100u, 100u }, { 100u, 100u, 100u }, 100u, 100u, 100u, { 90u, 70u, 70u, 80u, 110u, 160u }, 105u, 135u, 102u },
        { 108u,  24, { 82u, 142u, 100u, 88u, 155u }, 28u, { 78u, 132u, 112u, 108u, 118u }, { 55u, 170u, 70u }, 180u, 92u, 98u, { 100u, 135u, 115u, 68u, 122u, 95u }, 92u, 58u, 98u },
        {  94u,   4, { 58u, 78u, 145u, 175u, 62u }, 76u, { 148u, 82u, 55u, 138u, 68u }, { 15u, 55u, 330u }, 25u, 60u, 112u, { 175u, 150u, 120u, 38u, 72u, 185u }, 115u, 88u, 104u }
    }};

    bool checkCanonicalProfiles()
    {
        bool success = true;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipFactionType::SHIP_FACTION_TYPE_END); ++index)
        {
            const ShipFactionType faction = static_cast<ShipFactionType>(index);
            const ShipFactionProfile& profile = getShipFactionProfile(faction);
            const ExpectedFactionCoreData& expected = Expected[index];

            if (!validateShipFactionProfile(profile).isValid())
            {
                std::cerr << "Built-in ShipFactionProfile failed validation for faction index " << index << ".\n";
                success = false;
            }
            if (&profile.Palette != &getShipFactionPaletteProfile(faction) ||
                &profile.SurfaceDetails != &getShipFactionSurfaceDetailProfile(faction) ||
                &profile.Attachments != &getShipFactionAttachmentProfile(faction))
            {
                std::cerr << "Legacy faction getter is not a view into canonical profile for faction index " << index << ".\n";
                success = false;
            }

            const std::array<uint32_t, 5u> weaponWeights = {
                profile.Weapons.WeightMultipliersPercent.SingleCannon,
                profile.Weapons.WeightMultipliersPercent.TwinCannon,
                profile.Weapons.WeightMultipliersPercent.CompactTurret,
                profile.Weapons.WeightMultipliersPercent.RailWeapon,
                profile.Weapons.WeightMultipliersPercent.WeaponPod
            };
            const std::array<uint32_t, 5u> engineLayoutWeights = {
                profile.Engines.LayoutWeightMultipliersPercent.Central,
                profile.Engines.LayoutWeightMultipliersPercent.Twin,
                profile.Engines.LayoutWeightMultipliersPercent.Quad,
                profile.Engines.LayoutWeightMultipliersPercent.CentralAuxiliary,
                profile.Engines.LayoutWeightMultipliersPercent.WideBank
            };
            const std::array<uint32_t, 3u> engineSizeWeights = {
                profile.Engines.SizeWeightMultipliersPercent.Small,
                profile.Engines.SizeWeightMultipliersPercent.Medium,
                profile.Engines.SizeWeightMultipliersPercent.Large
            };
            const std::array<uint32_t, 6u> majorWeights = {
                profile.MajorFeatures.WeightMultipliersPercent.CentralSpine,
                profile.MajorFeatures.WeightMultipliersPercent.ArmorPlate,
                profile.MajorFeatures.WeightMultipliersPercent.RecessedBay,
                profile.MajorFeatures.WeightMultipliersPercent.VentBank,
                profile.MajorFeatures.WeightMultipliersPercent.WingPlate,
                profile.MajorFeatures.WeightMultipliersPercent.TechCore
            };

            if (profile.Weapons.ChancePercent != expected.WeaponChance || profile.Weapons.SymmetryChanceOffset != expected.WeaponSymmetryOffset ||
                weaponWeights != expected.WeaponWeights || profile.Weapons.EmissiveChance != expected.WeaponEmissiveChance ||
                engineLayoutWeights != expected.EngineLayoutWeights || engineSizeWeights != expected.EngineSizeWeights ||
                profile.Engines.NacelleChancePercent != expected.EngineNacelleChance || profile.Engines.ExternalHeightPercent != expected.EngineExternalHeight ||
                profile.MajorFeatures.ChancePercent != expected.MajorFeatureChance || majorWeights != expected.MajorFeatureWeights ||
                profile.Hull.NegativeSpaceChancePercent != expected.NegativeSpaceChance ||
                profile.MacroAsymmetry.ChancePercent != expected.MacroAsymmetryChance ||
                profile.Complexity.TotalBudgetPercent != expected.ComplexityBudgetPercent)
            {
                std::cerr << "Promoted faction tuning differs from Task-82 source values for faction index " << index << ".\n";
                success = false;
            }
        }
        return success;
    }

    bool checkRepresentedSpecialCases()
    {
        bool success = true;
        const ShipFactionProfile& corporate = getShipFactionProfile(ShipFactionType::CORPORATE);
        if (corporate.PaletteBehavior.HullValueMode != ShipFactionHullValueMode::ALTERNATING_BRIGHT_DARK_RANGES ||
            corporate.PaletteBehavior.BrightHullValue.Min != 68u || corporate.PaletteBehavior.BrightHullValue.Max != 84u ||
            corporate.PaletteBehavior.DarkHullValue.Min != 32u || corporate.PaletteBehavior.DarkHullValue.Max != 48u ||
            corporate.PaletteBehavior.MinimumAccentHueDistance != 65u ||
            corporate.Finish.ForceAxialRidgeEdgeHighlight != true || corporate.Finish.CockpitFrameRole != ShipFactionPaintColorRole::HULL_HIGHLIGHT)
        {
            std::cerr << "Corporate palette/finish special cases are not represented by semantic profile data.\n";
            success = false;
        }

        const ShipFactionProfile& relic = getShipFactionProfile(ShipFactionType::RELIC);
        if (relic.SurfaceDetails.LuminousChannelCoreRegionBiasChance != 72u || relic.HullLayers.MaximumLayerCount != 2u ||
            relic.CoreTreatment.CoreChannelLuminousPattern != ShipFactionCoreChannelLuminousPattern::EVERY_THIRD_ROW ||
            relic.Animation.LateralMovement.SquareTransitionInput != true ||
            relic.Animation.Firing.DurationAdditionMilliseconds != 65)
        {
            std::cerr << "Relic static/animation special cases are not represented by semantic profile data.\n";
            success = false;
        }

        const ShipFactionProfile& xeno = getShipFactionProfile(ShipFactionType::XENO);
        if (!xeno.Hull.PreferAlternateArticulationOrder || !xeno.Animation.Idle.AlternateTechCorePhases ||
            xeno.Finish.WeaponRaisedHighlightRole != ShipFactionPaintColorRole::HULL_ACCENT_HIGHLIGHT)
        {
            std::cerr << "Xeno articulation/finish/animation semantics are incomplete.\n";
            success = false;
        }

        const ShipFactionProfile& military = getShipFactionProfile(ShipFactionType::MILITARY);
        if (military.Livery.AllowAsymmetricGeometricInsignia || military.Livery.AsymmetricChanceDivisor != 3u ||
            military.Animation.Firing.DurationScale.Numerator != 9u || military.Animation.Firing.DurationScale.Denominator != 10u)
        {
            std::cerr << "Military livery/animation semantics are incomplete.\n";
            success = false;
        }
        return success;
    }


}

namespace SpectralShipGenTests
{
    int runFactionProfileRegression()
    {
        bool success = checkCanonicalProfiles() && checkRepresentedSpecialCases();

        ShipFactionProfile extreme = getShipFactionProfile(ShipFactionType::CORPORATE);
        extreme.Weapons.ChancePercent = 450u;
        extreme.Weapons.WeightMultipliersPercent.RailWeapon = 5000u;
        extreme.Engines.NacelleChancePercent = 600u;
        if (!validateShipFactionProfile(extreme).isValid())
        {
            std::cerr << "Extreme but safe faction multipliers were rejected.\n";
            success = false;
        }

        ShipFactionProfile invalid = getShipFactionProfile(ShipFactionType::FRONTIER);
        invalid.Palette.HullSaturation.Max = 101u;
        invalid.Engines.LayoutWeightMultipliersPercent = {};
        invalid.Livery.AsymmetricChanceDivisor = 0u;
        invalid.Animation.Firing.DurationScale.Denominator = 0u;
        if (validateShipFactionProfile(invalid).isValid())
        {
            std::cerr << "Invalid faction profile contracts were accepted.\n";
            success = false;
        }

        bool threw = false;
        try { (void)getShipFactionProfile(ShipFactionType::SHIP_FACTION_TYPE_END); }
        catch (const std::invalid_argument&) { threw = true; }
        if (!threw)
        {
            std::cerr << "SHIP_FACTION_TYPE_END was accepted as a built-in profile.\n";
            success = false;
        }

        return success ? 0 : 1;
    }
}
