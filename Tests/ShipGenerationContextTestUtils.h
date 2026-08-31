#pragma once

#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationSettings.h>

namespace SpectralShipGenTests
{
    inline SpectralShipGen::ExplicitShipGenerationConfiguration makeTestExplicitGenerationConfiguration(const SpectralShipGen::ShipGenerationSettings& settings)
    {
        SpectralShipGen::ExplicitShipGenerationConfiguration configuration;
        configuration.Seed = settings.Seed;
        configuration.Dimensions = settings.Dimensions;
        configuration.DetailDensity = settings.DetailDensity;
        configuration.AsymmetricDetailChance = settings.AsymmetricDetailChance;
        configuration.AttachmentsEnabled = settings.AttachmentsEnabled;
        configuration.SeedOverrides = settings.SeedOverrides;
        configuration.DomainSeedOverrides = settings.DomainSeedOverrides;
        return configuration;
    }
}
