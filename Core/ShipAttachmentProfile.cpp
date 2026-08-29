#include "ShipAttachmentProfile.h"

namespace PixelShipGenerator
{
    uint32_t ShipAttachmentWeights::getWeight(ShipAttachmentType type) const
    {
        switch (type)
        {
        case ShipAttachmentType::WEAPON_MOUNT: return WeaponMount;
        case ShipAttachmentType::SENSOR_ARRAY: return SensorArray;
        case ShipAttachmentType::AUXILIARY_POD: return AuxiliaryPod;
        case ShipAttachmentType::RADIATOR: return Radiator;
        case ShipAttachmentType::ARMOR_FIN: return ArmorFin;
        case ShipAttachmentType::TECHNOLOGY_NODE: return TechnologyNode;
        default: return 0u;
        }
    }

    namespace
    {
        ShipFactionAttachmentProfile createFrontierAttachmentProfile()
        {
            ShipFactionAttachmentProfile profile;

            profile.WeightMultipliersPercent.WeaponMount = 90u;
            profile.WeightMultipliersPercent.SensorArray = 110u;
            profile.WeightMultipliersPercent.AuxiliaryPod = 150u;
            profile.WeightMultipliersPercent.Radiator = 145u;
            profile.WeightMultipliersPercent.ArmorFin = 95u;
            profile.WeightMultipliersPercent.TechnologyNode = 45u;
            profile.AttachmentChancePercent = 105u;
            profile.SymmetryChanceOffset = -5;

            return profile;
        }

        ShipFactionAttachmentProfile createMilitaryAttachmentProfile()
        {
            ShipFactionAttachmentProfile profile;

            profile.WeightMultipliersPercent.WeaponMount = 155u;
            profile.WeightMultipliersPercent.SensorArray = 95u;
            profile.WeightMultipliersPercent.AuxiliaryPod = 80u;
            profile.WeightMultipliersPercent.Radiator = 55u;
            profile.WeightMultipliersPercent.ArmorFin = 150u;
            profile.WeightMultipliersPercent.TechnologyNode = 45u;
            profile.AttachmentChancePercent = 105u;
            profile.SymmetryChanceOffset = 15;

            return profile;
        }

        ShipFactionAttachmentProfile createAscendantAttachmentProfile()
        {
            ShipFactionAttachmentProfile profile;

            profile.WeightMultipliersPercent.WeaponMount = 80u;
            profile.WeightMultipliersPercent.SensorArray = 130u;
            profile.WeightMultipliersPercent.AuxiliaryPod = 55u;
            profile.WeightMultipliersPercent.Radiator = 35u;
            profile.WeightMultipliersPercent.ArmorFin = 75u;
            profile.WeightMultipliersPercent.TechnologyNode = 200u;
            profile.AttachmentChancePercent = 100u;
            profile.SymmetryChanceOffset = 10;

            return profile;
        }

        ShipFactionAttachmentProfile createXenoAttachmentProfile()
        {
            ShipFactionAttachmentProfile profile;

            profile.WeightMultipliersPercent.WeaponMount = 95u;
            profile.WeightMultipliersPercent.SensorArray = 115u;
            profile.WeightMultipliersPercent.AuxiliaryPod = 45u;
            profile.WeightMultipliersPercent.Radiator = 40u;
            profile.WeightMultipliersPercent.ArmorFin = 115u;
            profile.WeightMultipliersPercent.TechnologyNode = 225u;
            profile.AttachmentChancePercent = 100u;
            profile.SymmetryChanceOffset = -20;

            return profile;
        }

        ShipFactionAttachmentProfile createCorporateAttachmentProfile()
        {
            ShipFactionAttachmentProfile profile;

            profile.WeightMultipliersPercent.WeaponMount = 110u;
            profile.WeightMultipliersPercent.SensorArray = 120u;
            profile.WeightMultipliersPercent.AuxiliaryPod = 135u;
            profile.WeightMultipliersPercent.Radiator = 55u;
            profile.WeightMultipliersPercent.ArmorFin = 82u;
            profile.WeightMultipliersPercent.TechnologyNode = 95u;
            profile.AttachmentChancePercent = 92u;
            profile.SymmetryChanceOffset = 20;

            return profile;
        }

        ShipFactionAttachmentProfile createRelicAttachmentProfile()
        {
            ShipFactionAttachmentProfile profile;

            profile.WeightMultipliersPercent.WeaponMount = 70u;
            profile.WeightMultipliersPercent.SensorArray = 72u;
            profile.WeightMultipliersPercent.AuxiliaryPod = 42u;
            profile.WeightMultipliersPercent.Radiator = 28u;
            profile.WeightMultipliersPercent.ArmorFin = 105u;
            profile.WeightMultipliersPercent.TechnologyNode = 175u;
            profile.AttachmentChancePercent = 72u;
            profile.SymmetryChanceOffset = 4;

            return profile;
        }
    }

    const ShipFactionAttachmentProfile& getShipFactionAttachmentProfile(ShipFactionType faction)
    {
        static const ShipFactionAttachmentProfile FrontierProfile = createFrontierAttachmentProfile();
        static const ShipFactionAttachmentProfile MilitaryProfile = createMilitaryAttachmentProfile();
        static const ShipFactionAttachmentProfile AscendantProfile = createAscendantAttachmentProfile();
        static const ShipFactionAttachmentProfile XenoProfile = createXenoAttachmentProfile();
        static const ShipFactionAttachmentProfile CorporateProfile = createCorporateAttachmentProfile();
        static const ShipFactionAttachmentProfile RelicProfile = createRelicAttachmentProfile();

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
}