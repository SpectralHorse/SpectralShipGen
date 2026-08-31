#include <cstdint>
#include <stdexcept>

#include <SpectralShipGen/ShipFactionProfileValidation.h>
#include <SpectralShipGen/ShipGenerationProfileValidation.h>
#include <SpectralShipGen/ShipPaletteGenerationProfileValidation.h>
#include <SpectralShipGen/ShipGenerator.h>

using namespace SpectralShipGen;

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



GeneratedShip generateWithCustomGeneratedPalette()
{
    ExplicitShipGenerationConfiguration configuration;
    configuration.Seed = 0x86A55A1234567890ull;
    configuration.Dimensions = { 96u, 64u };
    configuration.PaletteConfiguration.Mode = ShipPaletteSourceMode::EXPLICIT_GENERATED;

    ShipGenerationProfile structuralProfile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
    ShipFactionProfile factionProfile = getShipFactionProfile(ShipFactionType::CORPORATE);

    // Copy a familiar palette language, then customize it independently of the
    // structural/faction profiles used by the generation call.
    ShipPaletteGenerationProfile paletteProfile = getShipPaletteGenerationProfile(ShipFactionType::ASCENDANT);
    paletteProfile.Ranges.HullHue = { 165u, 220u };
    paletteProfile.Ranges.HullSaturation = { 35u, 65u };
    paletteProfile.Ranges.Accent.Saturation = { 75u, 100u };
    configuration.PaletteConfiguration.Generated = paletteProfile;

    if (!validateShipPaletteGenerationProfile(paletteProfile).isValid())
    {
        throw std::invalid_argument("Custom ShipPaletteGenerationProfile is invalid.");
    }

    return ShipGenerator{}.generate(configuration, structuralProfile, factionProfile);
}

GeneratedShip generateWithFixedPalette()
{
    ExplicitShipGenerationConfiguration configuration;
    configuration.Seed = 0x86F1E0D2C3B4A596ull;
    configuration.Dimensions = { 64u, 96u };
    configuration.PaletteConfiguration.Mode = ShipPaletteSourceMode::FIXED;

    ShipPalette& palette = configuration.PaletteConfiguration.Fixed;
    palette.HullBase = Color(46u, 58u, 78u, 255u);
    palette.HullShadow = Color(28u, 36u, 52u, 255u);
    palette.HullHighlight = Color(82u, 98u, 124u, 255u);
    palette.HullAccent = Color(232u, 92u, 62u, 255u);
    palette.LightBase = Color(68u, 218u, 198u, 255u);
    palette.LightHighlight = Color(176u, 255u, 236u, 255u);

    ShipGenerationProfile structuralProfile = getShipGenerationProfile(ShipStyle::DELTA);
    ShipFactionProfile factionProfile = getShipFactionProfile(ShipFactionType::XENO);

    // The exact semantic colors above are used directly. Palette-domain rerolls
    // intentionally leave a FIXED palette unchanged.
    return ShipGenerator{}.generate(configuration, structuralProfile, factionProfile);
}
