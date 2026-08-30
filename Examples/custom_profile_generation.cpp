#include <cstdint>
#include <stdexcept>

#include "ShipFactionProfileValidation.h"
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

GeneratedShip generateCustomStructuralAndFactionProfiles()
{
    ExplicitShipGenerationConfiguration configuration;
    configuration.Seed = 0xA55A1234567890FFull;
    configuration.Dimensions = { 96u, 64u };

    ShipGenerationProfile structuralProfile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
    structuralProfile.LargeWeaponChance = 90u;
    structuralProfile.LargeWeaponScalePercent = 135u;

    ShipFactionProfile factionProfile = getShipFactionProfile(ShipFactionType::CORPORATE);
    factionProfile.Animation.Idle.TechPulseStrength = 3u;
    factionProfile.Animation.Firing.ResponseStrengthScale = { 4u, 5u };

    if (!validateShipGenerationProfile(structuralProfile).isValid() || !validateShipFactionProfile(factionProfile).isValid())
    {
        throw std::invalid_argument("Custom ship configuration is invalid.");
    }

    // Neither a ShipStyle nor a ShipFactionType is selected on this path.
    // GeneratedShip::Style/Faction will both contain their *_END provenance sentinels.
    return ShipGenerator{}.generate(configuration, structuralProfile, factionProfile);
}

