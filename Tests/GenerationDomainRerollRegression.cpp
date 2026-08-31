#include "CoreRegressionSuites.h"
#include "ShipGenerationContextTestUtils.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <SpectralShipGen/GenerationDomainReroll.h>
#include <SpectralShipGen/ShipGenerationRecipeSerializer.h>
#include "ShipGenerationContext.h"
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerator.h>

namespace
{
    using namespace SpectralShipGen;

    bool masksEqual(const PixelMask& first, const PixelMask& second)
    {
        if (first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight()) { return false; }
        for (uint32_t y = 0u; y < first.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < first.getWidth(); ++x)
            {
                if (first.get(x, y) != second.get(x, y)) { return false; }
            }
        }
        return true;
    }

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    ShipGenerationRecipe makeRecipe(ShipDimensions dimensions = { 96u, 96u })
    {
        ShipGenerationRecipe recipe;
        recipe.Seeds = deriveShipGenerationSeeds(0x0123456789ABCDEFull);
        recipe.Dimensions = dimensions;
        recipe.StructuralPreset = ShipStyle::HEAVY;
        recipe.FactionPreset = ShipFactionType::FRONTIER;
        recipe.DetailDensity = 68u;
        recipe.AsymmetricDetailChance = 19u;
        recipe.AttachmentsEnabled = true;
        return recipe;
    }

    ShipGenerationSettings makeSettings(const ShipGenerationRecipe& recipe)
    {
        ShipGenerationSettings settings;
        settings.Seed = recipe.Seeds.Master;
        settings.Dimensions = recipe.Dimensions;
        settings.Style = recipe.StructuralPreset.value_or(ShipStyle::FIGHTER);
        settings.Faction = recipe.FactionPreset.value_or(ShipFactionType::FRONTIER);
        settings.DetailDensity = recipe.DetailDensity;
        settings.AsymmetricDetailChance = recipe.AsymmetricDetailChance;
        settings.AttachmentsEnabled = recipe.AttachmentsEnabled;
        settings.SeedOverrides.Structure = recipe.Seeds.Structure;
        settings.SeedOverrides.Palette = recipe.Seeds.Palette;
        settings.SeedOverrides.Details = recipe.Seeds.Details;
        settings.SeedOverrides.Attachments = recipe.Seeds.Attachments;
        settings.DomainSeedOverrides = recipe.DomainSeedOverrides;
        return settings;
    }

    bool weaponDebugEqual(const ShipGenerationDebugInfo& first, const ShipGenerationDebugInfo& second)
    {
        if (first.WeaponCount != second.WeaponCount || first.WeaponTypeCounts != second.WeaponTypeCounts || first.WeaponUnits.size() != second.WeaponUnits.size()) { return false; }
        for (std::size_t index = 0u; index < first.WeaponUnits.size(); ++index)
        {
            const auto& a = first.WeaponUnits[index];
            const auto& b = second.WeaponUnits[index];
            if (a.Type != b.Type || a.Region != b.Region || a.AnchorX != b.AnchorX || a.AnchorY != b.AnchorY || a.BodyMinX != b.BodyMinX || a.BodyMaxX != b.BodyMaxX || a.BodyMinY != b.BodyMinY || a.BodyMaxY != b.BodyMaxY || a.MuzzleX != b.MuzzleX || a.MuzzleY != b.MuzzleY || a.SymmetryGroup != b.SymmetryGroup) { return false; }
        }
        return true;
    }

    bool generate(const ShipGenerationRecipe& recipe, GeneratedShip& ship, ShipGenerationDebugInfo& debugInfo)
    {
        try
        {
            ShipGenerator generator;
            ship = generator.generate(makeSettings(recipe), &debugInfo);
            return true;
        }
        catch (const std::exception& exception)
        {
            std::cerr << "Generation failed: " << exception.what() << '\n';
            return false;
        }
    }

    bool checkDomainSeedArchitecture()
    {
        bool success = true;
        const ShipGenerationSeeds top = deriveShipGenerationSeeds(0xA54FF53A5F1D36F1ull);
        GenerationDomainSeedOverrides none;
        const GenerationDomainSeeds first = resolveGenerationDomainSeeds(top, none);
        const GenerationDomainSeeds second = resolveGenerationDomainSeeds(top, none);
        if (first.Values != second.Values)
        {
            std::cerr << "Domain derivation is not stable.\n";
            success = false;
        }

        for (std::size_t index = 0u; index < GenerationDomainCount; ++index)
        {
            const GenerationDomain domain = static_cast<GenerationDomain>(index);
            const uint64_t parent = getGenerationSeedForChannel(top, getGenerationDomainParentChannel(domain));
            if (first.Values[index] != deriveGenerationDomainSeed(parent, domain))
            {
                std::cerr << getGenerationDomainName(domain) << " did not derive from its declared parent channel.\n";
                success = false;
            }
            if (std::string(getGenerationDomainDependencyDescription(domain)).empty())
            {
                std::cerr << getGenerationDomainName(domain) << " is missing dependency information.\n";
                success = false;
            }
        }

        if (first.get(GenerationDomain::WEAPONS) == first.get(GenerationDomain::ATTACHMENTS) || first.get(GenerationDomain::HULL) == first.get(GenerationDomain::WINGS))
        {
            std::cerr << "Distinct domains unexpectedly share the same derived seed.\n";
            success = false;
        }
        return success;
    }


    bool checkIndependentDomainStreams()
    {
        ShipGenerationSettings settings;
        settings.Seed = 0x243F6A8885A308D3ull;
        settings.Dimensions = { 64u, 64u };
        settings.Style = ShipStyle::FIGHTER;
        settings.Faction = ShipFactionType::FRONTIER;
        const ShipGenerationSeeds seeds = deriveShipGenerationSeeds(settings.Seed);
        const ShipGenerationProfile profile = getShipGenerationProfile(settings.Style);

        ShipGenerationContext attachmentReference(SpectralShipGenTests::makeTestExplicitGenerationConfiguration(settings), profile, getShipFactionProfile(settings.Faction), seeds);
        const uint32_t expectedAttachmentRoll = attachmentReference.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, 1000000u);
        ShipGenerationContext attachmentAfterWeapons(SpectralShipGenTests::makeTestExplicitGenerationConfiguration(settings), profile, getShipFactionProfile(settings.Faction), seeds);
        for (uint32_t index = 0u; index < 32u; ++index)
        {
            attachmentAfterWeapons.getGenerationRandomUInt(GenerationDomain::WEAPONS, 0u, 1000000u);
        }
        const uint32_t actualAttachmentRoll = attachmentAfterWeapons.getGenerationRandomUInt(GenerationDomain::ATTACHMENTS, 0u, 1000000u);
        if (expectedAttachmentRoll != actualAttachmentRoll)
        {
            std::cerr << "Weapon-domain draws advanced the Attachment domain stream.\n";
            return false;
        }

        ShipGenerationContext engineReference(SpectralShipGenTests::makeTestExplicitGenerationConfiguration(settings), profile, getShipFactionProfile(settings.Faction), seeds);
        const uint32_t expectedEngineRoll = engineReference.getGenerationRandomUInt(GenerationDomain::ENGINES, 0u, 1000000u);
        ShipGenerationContext engineAfterCockpit(SpectralShipGenTests::makeTestExplicitGenerationConfiguration(settings), profile, getShipFactionProfile(settings.Faction), seeds);
        for (uint32_t index = 0u; index < 32u; ++index)
        {
            engineAfterCockpit.getGenerationRandomUInt(GenerationDomain::COCKPIT, 0u, 1000000u);
        }
        const uint32_t actualEngineRoll = engineAfterCockpit.getGenerationRandomUInt(GenerationDomain::ENGINES, 0u, 1000000u);
        if (expectedEngineRoll != actualEngineRoll)
        {
            std::cerr << "Cockpit-domain draws advanced the Engine domain stream.\n";
            return false;
        }

        ShipGenerationContext wingReference(SpectralShipGenTests::makeTestExplicitGenerationConfiguration(settings), profile, getShipFactionProfile(settings.Faction), seeds);
        const uint32_t expectedWingRoll = wingReference.getGenerationRandomUInt(GenerationDomain::WINGS, 0u, 1000000u);
        ShipGenerationContext wingAfterHull(SpectralShipGenTests::makeTestExplicitGenerationConfiguration(settings), profile, getShipFactionProfile(settings.Faction), seeds);
        for (uint32_t index = 0u; index < 32u; ++index)
        {
            wingAfterHull.getGenerationRandomUInt(GenerationDomain::HULL, 0u, 1000000u);
        }
        const uint32_t actualWingRoll = wingAfterHull.getGenerationRandomUInt(GenerationDomain::WINGS, 0u, 1000000u);
        if (expectedWingRoll != actualWingRoll)
        {
            std::cerr << "Hull-domain draws advanced the Wings domain stream.\n";
            return false;
        }

        return true;
    }

    bool checkPerDomainRerollSeeds()
    {
        bool success = true;
        const ShipGenerationRecipe original = makeRecipe();
        const GenerationDomainSeeds originalEffective = resolveGenerationDomainSeeds(original.Seeds, original.DomainSeedOverrides);

        for (std::size_t selectedIndex = 0u; selectedIndex < GenerationDomainCount; ++selectedIndex)
        {
            const GenerationDomain selectedDomain = static_cast<GenerationDomain>(selectedIndex);
            const std::vector<GenerationDomain> selected = { selectedDomain };
            const uint64_t rerollSeed = 0xD1B54A32D192ED03ull + static_cast<uint64_t>(selectedIndex) * 0x9E3779B97F4A7C15ull;
            const ShipGenerationRecipe first = rerollGenerationDomains(original, selected, rerollSeed);
            const ShipGenerationRecipe second = rerollGenerationDomains(original, selected, rerollSeed);
            if (first != second)
            {
                std::cerr << getGenerationDomainName(selectedDomain) << " reroll recipe is not deterministic.\n";
                success = false;
                continue;
            }
            if (!(first.Seeds == original.Seeds))
            {
                std::cerr << getGenerationDomainName(selectedDomain) << " reroll changed a top-level seed.\n";
                success = false;
            }

            const GenerationDomainSeeds rerolledEffective = resolveGenerationDomainSeeds(first.Seeds, first.DomainSeedOverrides);
            for (std::size_t domainIndex = 0u; domainIndex < GenerationDomainCount; ++domainIndex)
            {
                if (domainIndex == selectedIndex)
                {
                    if (rerolledEffective.Values[domainIndex] == originalEffective.Values[domainIndex])
                    {
                        std::cerr << getGenerationDomainName(selectedDomain) << " reroll did not change its effective domain seed.\n";
                        success = false;
                    }
                }
                else if (rerolledEffective.Values[domainIndex] != originalEffective.Values[domainIndex])
                {
                    std::cerr << getGenerationDomainName(selectedDomain) << " reroll changed an unselected domain seed.\n";
                    success = false;
                }
            }

            GeneratedShip shipA;
            GeneratedShip shipB;
            ShipGenerationDebugInfo debugA;
            ShipGenerationDebugInfo debugB;
            if (!generate(first, shipA, debugA) || !generate(first, shipB, debugB) || !imagesEqual(shipA.FinalImage, shipB.FinalImage))
            {
                std::cerr << getGenerationDomainName(selectedDomain) << " rerolled recipe did not reproduce exactly.\n";
                success = false;
            }
        }
        return success;
    }

    bool checkMultiDomainReroll()
    {
        const ShipGenerationRecipe original = makeRecipe({ 96u, 64u });
        const std::vector<GenerationDomain> firstOrder = { GenerationDomain::COCKPIT, GenerationDomain::WINGS, GenerationDomain::HULL_LAYERS };
        const std::vector<GenerationDomain> secondOrder = { GenerationDomain::HULL_LAYERS, GenerationDomain::COCKPIT, GenerationDomain::WINGS };
        const uint64_t rerollSeed = 0xC6BC279692B5CC83ull;
        const ShipGenerationRecipe first = rerollGenerationDomains(original, firstOrder, rerollSeed);
        const ShipGenerationRecipe second = rerollGenerationDomains(original, secondOrder, rerollSeed);
        if (first != second)
        {
            std::cerr << "Multi-domain reroll depends on selection ordering.\n";
            return false;
        }

        const GenerationDomainSeeds before = resolveGenerationDomainSeeds(original.Seeds, original.DomainSeedOverrides);
        const GenerationDomainSeeds after = resolveGenerationDomainSeeds(first.Seeds, first.DomainSeedOverrides);
        for (std::size_t index = 0u; index < GenerationDomainCount; ++index)
        {
            const GenerationDomain domain = static_cast<GenerationDomain>(index);
            const bool selected = domain == GenerationDomain::COCKPIT || domain == GenerationDomain::WINGS || domain == GenerationDomain::HULL_LAYERS;
            if ((after.Values[index] != before.Values[index]) != selected)
            {
                std::cerr << "Multi-domain reroll changed the wrong effective seed at " << getGenerationDomainName(domain) << ".\n";
                return false;
            }
        }
        return true;
    }

    bool checkFocusedPreservation()
    {
        bool success = true;
        const ShipGenerationRecipe original = makeRecipe({ 96u, 96u });
        GeneratedShip baseShip;
        ShipGenerationDebugInfo baseDebug;
        if (!generate(original, baseShip, baseDebug)) { return false; }

        const auto checkUpstream = [&](GenerationDomain domain, uint64_t rerollSeed, bool requireHull, bool requireCockpit, bool requireEngines) -> bool
            {
                const ShipGenerationRecipe rerolled = rerollGenerationDomains(original, { domain }, rerollSeed);
                GeneratedShip ship;
                ShipGenerationDebugInfo debug;
                if (!generate(rerolled, ship, debug)) { return false; }
                if (requireHull && !masksEqual(baseShip.HullMask, ship.HullMask)) { return false; }
                if (requireCockpit && !masksEqual(baseShip.CockpitMask, ship.CockpitMask)) { return false; }
                if (requireEngines && (!masksEqual(baseShip.EngineMask, ship.EngineMask) || !masksEqual(baseShip.EngineExhaustMask, ship.EngineExhaustMask))) { return false; }
                return true;
            };

        const ShipGenerationRecipe paletteRecipe = rerollGenerationDomains(original, { GenerationDomain::PALETTE }, 0x1111222233334444ull);
        GeneratedShip paletteShip;
        ShipGenerationDebugInfo paletteDebug;
        if (!generate(paletteRecipe, paletteShip, paletteDebug) || !masksEqual(baseShip.HullMask, paletteShip.HullMask) || !masksEqual(baseShip.CockpitMask, paletteShip.CockpitMask) || !masksEqual(baseShip.EngineMask, paletteShip.EngineMask) || !masksEqual(baseShip.EngineExhaustMask, paletteShip.EngineExhaustMask) || !masksEqual(baseShip.AttachmentMask, paletteShip.AttachmentMask) || !masksEqual(baseShip.AccentMask, paletteShip.AccentMask) || !masksEqual(baseShip.MechanicalDetailMask, paletteShip.MechanicalDetailMask) || !masksEqual(baseShip.LightMask, paletteShip.LightMask) || !weaponDebugEqual(baseDebug, paletteDebug))
        {
            std::cerr << "Palette-only reroll changed geometry or semantic masks.\n";
            success = false;
        }

        const ShipGenerationRecipe detailRecipe = rerollGenerationDomains(original, { GenerationDomain::DETAILS }, 0x5555666677778888ull);
        GeneratedShip detailShip;
        ShipGenerationDebugInfo detailDebug;
        if (!generate(detailRecipe, detailShip, detailDebug) || !masksEqual(baseShip.HullMask, detailShip.HullMask) || !masksEqual(baseShip.CockpitMask, detailShip.CockpitMask) || !masksEqual(baseShip.EngineMask, detailShip.EngineMask) || !masksEqual(baseShip.EngineExhaustMask, detailShip.EngineExhaustMask) || !masksEqual(baseShip.AttachmentMask, detailShip.AttachmentMask) || !weaponDebugEqual(baseDebug, detailDebug))
        {
            std::cerr << "Details-only reroll changed structural geometry.\n";
            success = false;
        }

        if (!checkUpstream(GenerationDomain::COCKPIT, 0x90123456789ABCDEull, true, false, false))
        {
            std::cerr << "Cockpit-only reroll changed Hull/Wing geometry.\n";
            success = false;
        }
        if (!checkUpstream(GenerationDomain::ENGINES, 0xA0123456789ABCDEull, true, true, false))
        {
            std::cerr << "Engine-only reroll changed Hull/Cockpit geometry.\n";
            success = false;
        }
        if (!checkUpstream(GenerationDomain::HULL_LAYERS, 0xB0123456789ABCDEull, true, true, true))
        {
            std::cerr << "Hull-layer-only reroll changed earlier structure.\n";
            success = false;
        }
        if (!checkUpstream(GenerationDomain::WEAPONS, 0xC0123456789ABCDEull, true, true, true))
        {
            std::cerr << "Weapon-only reroll changed earlier structure.\n";
            success = false;
        }

        return success;
    }

    bool checkRecipeRoundTrip()
    {
        ShipGenerationRecipe recipe = makeRecipe({ 64u, 96u });
        recipe = rerollGenerationDomains(recipe, { GenerationDomain::COCKPIT, GenerationDomain::WEAPONS }, 0xF1357AEA2E62A9C5ull);
        ShipGenerationRecipeDocument document;
        document.Recipe = recipe;
        const std::string serialized = serializeShipGenerationRecipe(document);
        if (serialized.find("\"rng_mode\"") != std::string::npos || serialized.find("\"cockpit\"") == std::string::npos || serialized.find("\"weapons\"") == std::string::npos)
        {
            std::cerr << "Current recipe serialization did not preserve clean domain state.\n";
            return false;
        }
        const ShipGenerationRecipeLoadResult loaded = deserializeShipGenerationRecipe(serialized);
        if (!loaded.Success || loaded.Document.Recipe != recipe)
        {
            std::cerr << "Task-50 recipe JSON round-trip changed domain state.\n";
            return false;
        }
        return true;
    }

    bool checkUnsupportedPreV6RecipeRejection()
    {
        constexpr uint64_t MasterSeed = 0x6A09E667F3BCC909ull;
        const ShipGenerationSeeds seeds = deriveShipGenerationSeeds(MasterSeed);
        std::ostringstream json;
        json << "{\n"
            << "  \"format_version\": 2,\n"
            << "  \"ship\": {\n"
            << "    \"dimensions\": { \"width\": 64, \"height\": 64 },\n"
            << "    \"style\": \"SLEEK\",\n"
            << "    \"faction\": \"FRONTIER\",\n"
            << "    \"seeds\": { \"master\": " << seeds.Master << ", \"structure\": " << seeds.Structure << ", \"palette\": " << seeds.Palette << ", \"details\": " << seeds.Details << ", \"attachments\": " << seeds.Attachments << " },\n"
            << "    \"settings\": { \"detail_density\": 50, \"asymmetric_detail_chance\": 10, \"attachments_enabled\": true }\n"
            << "  }\n"
            << "}\n";

        const ShipGenerationRecipeLoadResult loaded = deserializeShipGenerationRecipe(json.str());
        if (loaded.Success || loaded.Error.find("Unsupported SpectralShipGen recipe format version") == std::string::npos)
        {
            std::cerr << "Unsupported pre-1.0 recipe was not rejected cleanly.\n";
            return false;
        }
        return true;
    }

    bool checkOrdinaryGenerationAndRectangular()
    {
        ShipGenerator generator;
        ShipGenerationSettings settings;
        settings.Seed = 0xBB67AE8584CAA73Bull;
        settings.Dimensions = { 48u, 64u };
        settings.Style = ShipStyle::FIGHTER;
        settings.Faction = ShipFactionType::MILITARY;
        const GeneratedShip first = generator.generate(settings);
        const GeneratedShip second = generator.generate(settings);
        if (!imagesEqual(first.FinalImage, second.FinalImage) || first.DomainSeeds.Values != second.DomainSeeds.Values)
        {
            std::cerr << "Ordinary no-override rectangular generation is not deterministic.\n";
            return false;
        }
        if (first.FinalImage.getWidth() != 48u || first.FinalImage.getHeight() != 64u)
        {
            std::cerr << "Rectangular generation dimensions changed.\n";
            return false;
        }
        return true;
    }
}

int SpectralShipGenTests::runGenerationDomainRerollRegression()
{
    bool success = true;
    success = checkDomainSeedArchitecture() && success;
    success = checkIndependentDomainStreams() && success;
    success = checkPerDomainRerollSeeds() && success;
    success = checkMultiDomainReroll() && success;
    success = checkFocusedPreservation() && success;
    success = checkRecipeRoundTrip() && success;
    success = checkUnsupportedPreV6RecipeRejection() && success;
    success = checkOrdinaryGenerationAndRectangular() && success;
    return success ? 0 : 1;
}
