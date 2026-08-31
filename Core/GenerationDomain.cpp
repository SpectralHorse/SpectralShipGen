#include <SpectralShipGen/GenerationDomain.h>

#include <stdexcept>

namespace SpectralShipGen
{
    GenerationSeedChannel getGenerationDomainParentChannel(GenerationDomain domain)
    {
        switch (domain)
        {
        case GenerationDomain::HULL:
        case GenerationDomain::WINGS:
        case GenerationDomain::COCKPIT:
        case GenerationDomain::ENGINES:
        case GenerationDomain::HULL_LAYERS:
        case GenerationDomain::MAJOR_FEATURES:
        case GenerationDomain::MACRO_ASYMMETRY:
            return GenerationSeedChannel::STRUCTURE;
        case GenerationDomain::WEAPONS:
        case GenerationDomain::ATTACHMENTS:
            return GenerationSeedChannel::ATTACHMENTS;
        case GenerationDomain::PALETTE:
            return GenerationSeedChannel::PALETTE;
        case GenerationDomain::DETAILS:
            return GenerationSeedChannel::DETAILS;
        default:
            throw std::invalid_argument("Unknown GenerationDomain value.");
        }
    }

    const char* getGenerationDomainName(GenerationDomain domain)
    {
        switch (domain)
        {
        case GenerationDomain::HULL: return "HULL";
        case GenerationDomain::WINGS: return "WINGS";
        case GenerationDomain::COCKPIT: return "COCKPIT";
        case GenerationDomain::ENGINES: return "ENGINES";
        case GenerationDomain::HULL_LAYERS: return "HULL_LAYERS";
        case GenerationDomain::MAJOR_FEATURES: return "MAJOR_FEATURES";
        case GenerationDomain::MACRO_ASYMMETRY: return "MACRO_ASYMMETRY";
        case GenerationDomain::WEAPONS: return "WEAPONS";
        case GenerationDomain::ATTACHMENTS: return "ATTACHMENTS";
        case GenerationDomain::PALETTE: return "PALETTE";
        case GenerationDomain::DETAILS: return "DETAILS";
        default: return "UNKNOWN";
        }
    }

    const char* getGenerationDomainDependencyDescription(GenerationDomain domain)
    {
        switch (domain)
        {
        case GenerationDomain::HULL:
            return "May change most downstream geometry and placement because later domains consume the resulting hull.";
        case GenerationDomain::WINGS:
            return "May change wing-based layers, features, weapons, attachments and details while retaining the hull-shape seed.";
        case GenerationDomain::COCKPIT:
            return "Retains upstream hull and wing intent, but later occupancy-dependent engines/features/weapons/attachments may adapt.";
        case GenerationDomain::ENGINES:
            return "Retains hull/wings/cockpit, but rear occupancy and later layers/features/details may adapt.";
        case GenerationDomain::HULL_LAYERS:
            return "Retains earlier structure, but later composition can adapt through occupancy and global/spatial budgets.";
        case GenerationDomain::MAJOR_FEATURES:
            return "Retains earlier structure, but later weapons/attachments/details may adapt to occupancy and budgets.";
        case GenerationDomain::MACRO_ASYMMETRY:
            return "Retains the symmetric base structure, but can alter the planned hull-layer/weapon/attachment focal decision and later balance details.";
        case GenerationDomain::WEAPONS:
            return "Retains earlier structure, but later attachments/details may adapt to weapon occupancy and budget consumption.";
        case GenerationDomain::ATTACHMENTS:
            return "Retains earlier structure and weapons; later surface details may adapt to attachment occupancy.";
        case GenerationDomain::PALETTE:
            return "Painting only. Structural geometry and semantic masks remain unchanged.";
        case GenerationDomain::DETAILS:
            return "Surface details only. Structural geometry and earlier semantic masks remain unchanged.";
        default:
            return "Unknown generation-domain dependency.";
        }
    }
}
