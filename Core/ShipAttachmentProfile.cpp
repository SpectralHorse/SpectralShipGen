#include <SpectralShipGen/ShipAttachmentProfile.h>

#include <SpectralShipGen/ShipFactionProfile.h>

namespace SpectralShipGen
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

    const ShipFactionAttachmentProfile& getShipFactionAttachmentProfile(ShipFactionType faction)
    {
        return getShipFactionProfile(faction).Attachments;
    }
}
