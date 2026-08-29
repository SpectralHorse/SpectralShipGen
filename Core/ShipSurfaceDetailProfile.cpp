#include "ShipSurfaceDetailProfile.h"

namespace PixelShipGenerator
{
    namespace
    {
        ShipFactionSurfaceDetailProfile createFrontierSurfaceDetailProfile()
        {
            ShipFactionSurfaceDetailProfile profile;

            profile.DetailDensityPercent = 115u;
            profile.MechanicalPatternCountPercent = 135u;
            profile.LightPatternCountPercent = 70u;
            profile.AccentPanelWeightPercent = 135u;
            profile.AccentStripeWeightPercent = 70u;
            profile.AccentArmorWeightPercent = 110u;
            profile.HorizontalVentChancePercent = 145u;
            profile.SupplementalWeightMultipliersPercent.PanelSeam = 160u;
            profile.SupplementalWeightMultipliersPercent.GeometricMarking = 45u;
            profile.SupplementalWeightMultipliersPercent.MechanicalExposure = 170u;
            profile.SupplementalWeightMultipliersPercent.RepeatingMotif = 40u;
            profile.MotifWeightMultipliersPercent.PairedVents = 135u;
            profile.MotifWeightMultipliersPercent.TripleVentBank = 120u;
            profile.MotifWeightMultipliersPercent.PairedLights = 75u;
            profile.MotifWeightMultipliersPercent.ThreeNodeLights = 70u;
            profile.MotifWeightMultipliersPercent.ParallelSeams = 95u;
            profile.MotifWeightMultipliersPercent.RepeatedDashes = 110u;
            profile.MotifWeightMultipliersPercent.RecessedSlot = 135u;
            profile.MotifRepeatPercent = 92u;
            profile.AsymmetricDetailChanceOffset = 8;

            return profile;
        }

        ShipFactionSurfaceDetailProfile createMilitarySurfaceDetailProfile()
        {
            ShipFactionSurfaceDetailProfile profile;

            profile.DetailDensityPercent = 90u;
            profile.MechanicalPatternCountPercent = 80u;
            profile.LightPatternCountPercent = 55u;
            profile.AccentPanelWeightPercent = 110u;
            profile.AccentStripeWeightPercent = 175u;
            profile.AccentArmorWeightPercent = 120u;
            profile.HorizontalVentChancePercent = 65u;
            profile.SupplementalWeightMultipliersPercent.PanelSeam = 150u;
            profile.SupplementalWeightMultipliersPercent.GeometricMarking = 85u;
            profile.SupplementalWeightMultipliersPercent.MechanicalExposure = 35u;
            profile.SupplementalWeightMultipliersPercent.RepeatingMotif = 70u;
            profile.MotifWeightMultipliersPercent.PairedVents = 125u;
            profile.MotifWeightMultipliersPercent.TripleVentBank = 115u;
            profile.MotifWeightMultipliersPercent.PairedLights = 120u;
            profile.MotifWeightMultipliersPercent.ThreeNodeLights = 110u;
            profile.MotifWeightMultipliersPercent.ParallelSeams = 130u;
            profile.MotifWeightMultipliersPercent.RepeatedDashes = 100u;
            profile.MotifWeightMultipliersPercent.RecessedSlot = 110u;
            profile.MotifRepeatPercent = 112u;
            profile.AsymmetricDetailChanceOffset = -8;

            return profile;
        }

        ShipFactionSurfaceDetailProfile createAscendantSurfaceDetailProfile()
        {
            ShipFactionSurfaceDetailProfile profile;

            profile.DetailDensityPercent = 65u;
            profile.MechanicalPatternCountPercent = 25u;
            profile.LightPatternCountPercent = 80u;
            profile.AccentPanelWeightPercent = 40u;
            profile.AccentStripeWeightPercent = 90u;
            profile.AccentArmorWeightPercent = 30u;
            profile.HorizontalVentChancePercent = 15u;
            profile.SupplementalWeightMultipliersPercent.PanelSeam = 35u;
            profile.SupplementalWeightMultipliersPercent.GeometricMarking = 210u;
            profile.SupplementalWeightMultipliersPercent.MechanicalExposure = 10u;
            profile.SupplementalWeightMultipliersPercent.RepeatingMotif = 90u;
            profile.MotifWeightMultipliersPercent.PairedVents = 35u;
            profile.MotifWeightMultipliersPercent.TripleVentBank = 20u;
            profile.MotifWeightMultipliersPercent.PairedLights = 145u;
            profile.MotifWeightMultipliersPercent.ThreeNodeLights = 155u;
            profile.MotifWeightMultipliersPercent.ParallelSeams = 150u;
            profile.MotifWeightMultipliersPercent.RepeatedDashes = 95u;
            profile.MotifWeightMultipliersPercent.RecessedSlot = 55u;
            profile.MotifRepeatPercent = 88u;
            profile.AsymmetricDetailChanceOffset = -6;

            return profile;
        }

        ShipFactionSurfaceDetailProfile createXenoSurfaceDetailProfile()
        {
            ShipFactionSurfaceDetailProfile profile;

            profile.DetailDensityPercent = 95u;
            profile.MechanicalPatternCountPercent = 50u;
            profile.LightPatternCountPercent = 105u;
            profile.AccentPanelWeightPercent = 35u;
            profile.AccentStripeWeightPercent = 45u;
            profile.AccentArmorWeightPercent = 55u;
            profile.HorizontalVentChancePercent = 25u;
            profile.SupplementalWeightMultipliersPercent.PanelSeam = 45u;
            profile.SupplementalWeightMultipliersPercent.GeometricMarking = 180u;
            profile.SupplementalWeightMultipliersPercent.MechanicalExposure = 45u;
            profile.SupplementalWeightMultipliersPercent.RepeatingMotif = 260u;
            profile.MotifWeightMultipliersPercent.PairedVents = 55u;
            profile.MotifWeightMultipliersPercent.TripleVentBank = 45u;
            profile.MotifWeightMultipliersPercent.PairedLights = 90u;
            profile.MotifWeightMultipliersPercent.ThreeNodeLights = 150u;
            profile.MotifWeightMultipliersPercent.ParallelSeams = 65u;
            profile.MotifWeightMultipliersPercent.RepeatedDashes = 165u;
            profile.MotifWeightMultipliersPercent.RecessedSlot = 130u;
            profile.MotifRepeatPercent = 106u;
            profile.AsymmetricDetailChanceOffset = 18;

            return profile;
        }

        ShipFactionSurfaceDetailProfile createCorporateSurfaceDetailProfile()
        {
            ShipFactionSurfaceDetailProfile profile;

            profile.DetailDensityPercent = 92u;
            profile.MechanicalPatternCountPercent = 58u;
            profile.LightPatternCountPercent = 72u;
            profile.AccentPanelWeightPercent = 70u;
            profile.AccentStripeWeightPercent = 220u;
            profile.AccentArmorWeightPercent = 72u;
            profile.HorizontalVentChancePercent = 72u;
            profile.SupplementalWeightMultipliersPercent.PanelSeam = 190u;
            profile.SupplementalWeightMultipliersPercent.GeometricMarking = 135u;
            profile.SupplementalWeightMultipliersPercent.MechanicalExposure = 12u;
            profile.SupplementalWeightMultipliersPercent.RepeatingMotif = 115u;
            profile.SupplementalWeightMultipliersPercent.IdentificationMarking = 185u;
            profile.SupplementalWeightMultipliersPercent.LuminousChannel = 18u;
            profile.MotifWeightMultipliersPercent.PairedVents = 90u;
            profile.MotifWeightMultipliersPercent.TripleVentBank = 75u;
            profile.MotifWeightMultipliersPercent.PairedLights = 140u;
            profile.MotifWeightMultipliersPercent.ThreeNodeLights = 115u;
            profile.MotifWeightMultipliersPercent.ParallelSeams = 165u;
            profile.MotifWeightMultipliersPercent.RepeatedDashes = 135u;
            profile.MotifWeightMultipliersPercent.RecessedSlot = 120u;
            profile.MotifRepeatPercent = 122u;
            profile.AsymmetricDetailChanceOffset = -14;

            return profile;
        }

        ShipFactionSurfaceDetailProfile createRelicSurfaceDetailProfile()
        {
            ShipFactionSurfaceDetailProfile profile;

            profile.DetailDensityPercent = 72u;
            profile.MechanicalPatternCountPercent = 30u;
            profile.LightPatternCountPercent = 118u;
            profile.AccentPanelWeightPercent = 32u;
            profile.AccentStripeWeightPercent = 45u;
            profile.AccentArmorWeightPercent = 82u;
            profile.HorizontalVentChancePercent = 18u;
            profile.SupplementalWeightMultipliersPercent.PanelSeam = 72u;
            profile.SupplementalWeightMultipliersPercent.GeometricMarking = 155u;
            profile.SupplementalWeightMultipliersPercent.MechanicalExposure = 12u;
            profile.SupplementalWeightMultipliersPercent.RepeatingMotif = 230u;
            profile.SupplementalWeightMultipliersPercent.IdentificationMarking = 8u;
            profile.SupplementalWeightMultipliersPercent.LuminousChannel = 250u;
            profile.MotifWeightMultipliersPercent.PairedVents = 25u;
            profile.MotifWeightMultipliersPercent.TripleVentBank = 20u;
            profile.MotifWeightMultipliersPercent.PairedLights = 95u;
            profile.MotifWeightMultipliersPercent.ThreeNodeLights = 145u;
            profile.MotifWeightMultipliersPercent.ParallelSeams = 125u;
            profile.MotifWeightMultipliersPercent.RepeatedDashes = 80u;
            profile.MotifWeightMultipliersPercent.RecessedSlot = 155u;
            profile.MotifRepeatPercent = 78u;
            profile.AsymmetricDetailChanceOffset = -2;

            return profile;
        }
    }

    uint32_t ShipDetailMotifWeights::getWeight(ShipDetailMotifType type) const
    {
        switch (type)
        {
        case ShipDetailMotifType::PAIRED_VENTS: return PairedVents;
        case ShipDetailMotifType::TRIPLE_VENT_BANK: return TripleVentBank;
        case ShipDetailMotifType::PAIRED_LIGHTS: return PairedLights;
        case ShipDetailMotifType::THREE_NODE_LIGHTS: return ThreeNodeLights;
        case ShipDetailMotifType::PARALLEL_SEAMS: return ParallelSeams;
        case ShipDetailMotifType::REPEATED_DASHES: return RepeatedDashes;
        case ShipDetailMotifType::RECESSED_SLOT: return RecessedSlot;
        default: return 0u;
        }
    }

    uint32_t SupplementalSurfaceDetailWeights::getWeight(SupplementalSurfaceDetailType type) const
    {
        switch (type)
        {
        case SupplementalSurfaceDetailType::PANEL_SEAM: return PanelSeam;
        case SupplementalSurfaceDetailType::GEOMETRIC_MARKING: return GeometricMarking;
        case SupplementalSurfaceDetailType::MECHANICAL_EXPOSURE: return MechanicalExposure;
        case SupplementalSurfaceDetailType::REPEATING_MOTIF: return RepeatingMotif;
        case SupplementalSurfaceDetailType::IDENTIFICATION_MARKING: return IdentificationMarking;
        case SupplementalSurfaceDetailType::LUMINOUS_CHANNEL: return LuminousChannel;
        default: return 0u;
        }
    }

    const ShipFactionSurfaceDetailProfile& getShipFactionSurfaceDetailProfile(ShipFactionType faction)
    {
        static const ShipFactionSurfaceDetailProfile FrontierProfile = createFrontierSurfaceDetailProfile();
        static const ShipFactionSurfaceDetailProfile MilitaryProfile = createMilitarySurfaceDetailProfile();
        static const ShipFactionSurfaceDetailProfile AscendantProfile = createAscendantSurfaceDetailProfile();
        static const ShipFactionSurfaceDetailProfile XenoProfile = createXenoSurfaceDetailProfile();
        static const ShipFactionSurfaceDetailProfile CorporateProfile = createCorporateSurfaceDetailProfile();
        static const ShipFactionSurfaceDetailProfile RelicProfile = createRelicSurfaceDetailProfile();

        switch (faction)
        {
        case ShipFactionType::FRONTIER: return FrontierProfile;
        case ShipFactionType::MILITARY: return MilitaryProfile;
        case ShipFactionType::ASCENDANT: return AscendantProfile;
        case ShipFactionType::XENO: return XenoProfile;
        case ShipFactionType::CORPORATE: return CorporateProfile;
        case ShipFactionType::RELIC: return RelicProfile;
        default: return FrontierProfile;
        }
    }
} // namespace PixelShipGenerator
