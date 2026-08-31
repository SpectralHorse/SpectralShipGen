#include <iostream>

#include <SpectralShipGen/GenerationDomainReroll.h>
#include <SpectralShipGen/ShipGenerationRecipe.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>

namespace
{
    bool imagesEqual(const SpectralShipGen::Image& a, const SpectralShipGen::Image& b)
    {
        return a.getWidth() == b.getWidth() && a.getHeight() == b.getHeight() && a.getPixels() == b.getPixels();
    }

    bool masksEqual(const SpectralShipGen::PixelMask& a, const SpectralShipGen::PixelMask& b)
    {
        if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight()) { return false; }
        for (uint32_t y = 0; y < a.getHeight(); ++y)
        {
            for (uint32_t x = 0; x < a.getWidth(); ++x)
            {
                if (a.get(x, y) != b.get(x, y)) { return false; }
            }
        }
        return true;
    }
}

int main()
{
    SpectralShipGen::ShipGenerationSettings settings;
    settings.Seed = 0x1040000000000007ull;
    settings.Dimensions = { 64u, 64u };
    settings.Style = SpectralShipGen::ShipStyle::FIGHTER;
    settings.Faction = SpectralShipGen::ShipFactionType::ASCENDANT;

    const auto originalRecipe = SpectralShipGen::makeShipGenerationRecipe(settings);
    const auto paletteRecipe = SpectralShipGen::rerollGenerationDomains(
        originalRecipe, { SpectralShipGen::GenerationDomain::PALETTE }, 0xABCDEF1234567890ull);

    SpectralShipGen::ShipGenerator generator;
    const auto original = generator.generate(originalRecipe);
    const auto recolored = generator.generate(paletteRecipe);

    const bool geometryPreserved = masksEqual(original.HullMask, recolored.HullMask);
    const bool colorsChanged = !imagesEqual(original.FinalImage, recolored.FinalImage);
    std::cout << "Palette-domain reroll changed the rendered colors while preserving generated hull geometry.\n";
    return geometryPreserved && colorsChanged ? 0 : 1;
}
