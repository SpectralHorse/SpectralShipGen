#include <cstdint>
#include <stdexcept>

#include "ShipGenerationProfileValidation.h"
#include "ShipGenerator.h"

using namespace PixelShipGenerator;

GeneratedShip generateBuiltInFighter()
{
    ShipGenerationSettings settings;
    settings.Seed = 0x123456789ABCDEF0ull;
    settings.Dimensions = { 64u, 64u };
    settings.Style = ShipStyle::FIGHTER;
    settings.Faction = ShipFactionType::MILITARY;

    return ShipGenerator{}.generate(settings);
}

GeneratedShip generateModifiedIndustrialProfile()
{
    ShipGenerationConfiguration configuration;
    configuration.Seed = 0x123456789ABCDEF0ull;
    configuration.Dimensions = { 96u, 64u };
    configuration.Faction = ShipFactionType::MILITARY;

    ShipGenerationProfile profile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
    profile.LargeWeaponChance = 85u;
    profile.MaximumLargeWeaponGroups = 2u;
    profile.LargeWeaponScalePercent = 140u;

    const ShipGenerationProfileValidationResult validation = validateShipGenerationProfile(profile);
    if (!validation.isValid())
    {
        throw std::invalid_argument(validation.Errors.front().Field + ": " + validation.Errors.front().Message);
    }

    // No ShipStyle is selected or fabricated on this path.
    return ShipGenerator{}.generate(configuration, profile);
}

GeneratedShip generateDefaultCustomProfile()
{
    ShipGenerationConfiguration configuration;
    configuration.Seed = 42u;
    configuration.Dimensions = { 64u, 64u };

    ShipGenerationProfile profile;
    if (!validateShipGenerationProfile(profile).isValid())
    {
        throw std::invalid_argument("Default ShipGenerationProfile is invalid.");
    }

    return ShipGenerator{}.generate(configuration, profile);
}
