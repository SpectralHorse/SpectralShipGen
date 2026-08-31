#include "CoreRegressionSuites.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>

#include <PixelShipGenerator/GenerationComplexityBudget.h>
#include <PixelShipGenerator/GenerationScaleTraits.h>
#include <PixelShipGenerator/ShipGenerator.h>

namespace
{
    using namespace PixelShipGenerator;

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    uint32_t sumAllocations(const GenerationComplexityBudget& budget)
    {
        uint32_t total = 0u;
        for (uint32_t value : budget.getAllocations()) { total += value; }
        return total;
    }

    struct BudgetSample
    {
        double Initial = 0.0;
        double Consumed = 0.0;
        double DominantFeatures = 0.0;
        bool FoundSparse = false;
        std::array<bool, GenerationComplexityBudget::CategoryCount> CategorySeen = {};
    };

    BudgetSample sampleGeneration(uint32_t dimension, ShipStyle style, ShipFactionType faction)
    {
        ShipGenerator generator;
        BudgetSample result;
        constexpr uint32_t Samples = 48u;

        for (uint32_t sample = 0u; sample < Samples; ++sample)
        {
            ShipGenerationSettings settings;
            settings.Seed = 0x9E3779B97F4A7C15ull ^ (static_cast<uint64_t>(sample) * 0xD1B54A32D192ED03ull);
            settings.Dimensions = { dimension, dimension };
            settings.Style = style;
            settings.Faction = faction;
            settings.DetailDensity = 65u;
            settings.AttachmentsEnabled = true;

            ShipGenerationDebugInfo debug;
            const GeneratedShip first = generator.generate(settings, &debug);
            const GeneratedShip second = generator.generate(settings);

            if (!imagesEqual(first.FinalImage, second.FinalImage))
            {
                throw std::runtime_error("Complexity-budget generation is not deterministic.");
            }

            if (debug.ComplexityInitialBudget == 0u || debug.ComplexityConsumedBudget > debug.ComplexityInitialBudget || debug.ComplexityUnusedBudget != debug.ComplexityInitialBudget - debug.ComplexityConsumedBudget)
            {
                throw std::runtime_error("Invalid complexity-budget debug accounting.");
            }

            uint32_t allocationSum = 0u;
            uint32_t consumedSum = 0u;
            for (std::size_t index = 0u; index < GenerationComplexityBudget::CategoryCount; ++index)
            {
                allocationSum += debug.ComplexityCategoryAllocations[index];
                consumedSum += debug.ComplexityCategoryConsumed[index];
            }

            if (allocationSum != debug.ComplexityInitialBudget || consumedSum != debug.ComplexityConsumedBudget)
            {
                throw std::runtime_error("Complexity-budget category accounting does not match totals.");
            }

            result.Initial += static_cast<double>(debug.ComplexityInitialBudget);
            result.Consumed += static_cast<double>(debug.ComplexityConsumedBudget);
            result.DominantFeatures += static_cast<double>(debug.AppliedHullModifiers.size() + debug.MajorFeatureCount + debug.WeaponCount + debug.AttachmentPlacedGroupCount);
            for (std::size_t index = 0u; index < result.CategorySeen.size(); ++index)
            {
                result.CategorySeen[index] = result.CategorySeen[index] || debug.ComplexityCategoryConsumed[index] > 0u;
            }
            if (debug.ComplexityConsumedBudget * 2u < debug.ComplexityInitialBudget) { result.FoundSparse = true; }
        }

        result.Initial /= static_cast<double>(Samples);
        result.Consumed /= static_cast<double>(Samples);
        result.DominantFeatures /= static_cast<double>(Samples);
        return result;
    }
}

int PixelShipGeneratorTests::runGenerationComplexityBudgetRegression()
{
    using namespace PixelShipGenerator;

    try
    {
        const GenerationScaleTraits tinyTraits = GenerationScaleTraits::fromDimensions({ 24u, 24u });
        const GenerationScaleTraits mediumTraits = GenerationScaleTraits::fromDimensions({ 64u, 64u });
        const GenerationScaleTraits largeTraits = GenerationScaleTraits::fromDimensions({ 160u, 160u });

        const GenerationComplexityBudget tiny = GenerationComplexityBudget::create(tinyTraits, ShipStyle::FIGHTER, ShipFactionType::MILITARY);
        const GenerationComplexityBudget medium = GenerationComplexityBudget::create(mediumTraits, ShipStyle::FIGHTER, ShipFactionType::MILITARY);
        const GenerationComplexityBudget large = GenerationComplexityBudget::create(largeTraits, ShipStyle::FIGHTER, ShipFactionType::MILITARY);

        if (!(tiny.getInitialBudget() < medium.getInitialBudget() && medium.getInitialBudget() < large.getInitialBudget()))
        {
            std::cerr << "Scale-aware total complexity budget is not progressive.\n";
            return 1;
        }

        if (sumAllocations(tiny) != tiny.getInitialBudget() || sumAllocations(medium) != medium.getInitialBudget() || sumAllocations(large) != large.getInitialBudget())
        {
            std::cerr << "Complexity category allocations do not exactly cover the total budget.\n";
            return 1;
        }

        const GenerationComplexityBudget sleek = GenerationComplexityBudget::create(mediumTraits, ShipStyle::SLEEK, ShipFactionType::MILITARY);
        const GenerationComplexityBudget fighter = GenerationComplexityBudget::create(mediumTraits, ShipStyle::FIGHTER, ShipFactionType::MILITARY);
        const GenerationComplexityBudget industrial = GenerationComplexityBudget::create(mediumTraits, ShipStyle::INDUSTRIAL, ShipFactionType::MILITARY);
        if (!(sleek.getInitialBudget() < fighter.getInitialBudget() && fighter.getInitialBudget() < industrial.getInitialBudget()))
        {
            std::cerr << "Style complexity tendencies are not reflected in total budget.\n";
            return 1;
        }

        GenerationComplexityBudget reservation = GenerationComplexityBudget::create(mediumTraits, ShipStyle::FIGHTER, ShipFactionType::FRONTIER);
        const uint32_t weaponAllocation = reservation.getCategoryAllocation(GenerationComplexityCategory::LARGE_WEAPON);
        const uint32_t weaponAvailableBefore = reservation.getCategoryAvailable(GenerationComplexityCategory::LARGE_WEAPON);
        if (weaponAvailableBefore != weaponAllocation)
        {
            std::cerr << "Future categories can consume unreleased earlier reservations.\n";
            return 1;
        }

        reservation.finalizeCategory(GenerationComplexityCategory::SILHOUETTE);
        if (reservation.getCategoryAvailable(GenerationComplexityCategory::LARGE_WEAPON) <= weaponAllocation)
        {
            std::cerr << "Unused finalized category budget was not released to later stages.\n";
            return 1;
        }

        GenerationComplexityBudget hierarchyReservation = GenerationComplexityBudget::create(mediumTraits, ShipStyle::FIGHTER, ShipFactionType::FRONTIER);
        const uint32_t baselineCockpitAllocation = hierarchyReservation.getCategoryAllocation(GenerationComplexityCategory::COCKPIT_STRUCTURE);
        const uint32_t hierarchyReserved = hierarchyReservation.applyHierarchyReservation(GenerationComplexityCategory::COCKPIT_STRUCTURE, 6u);
        if (hierarchyReserved == 0u || hierarchyReservation.getHierarchyReservedBudget() != hierarchyReserved || hierarchyReservation.getCategoryAllocation(GenerationComplexityCategory::COCKPIT_STRUCTURE) <= baselineCockpitAllocation || sumAllocations(hierarchyReservation) != hierarchyReservation.getInitialBudget())
        {
            std::cerr << "Visual hierarchy reservation did not reallocate Task-46 capacity coherently.\n";
            return 1;
        }

        GenerationComplexityBudget hierarchyHold = GenerationComplexityBudget::create(mediumTraits, ShipStyle::FIGHTER, ShipFactionType::FRONTIER);
        const auto holdAllocations = hierarchyHold.getAllocations();
        const uint32_t held = hierarchyHold.applyHierarchyReservation(GenerationComplexityCategory::GENERATION_COMPLEXITY_CATEGORY_END, 6u);
        if (held == 0u || hierarchyHold.getAllocations() != holdAllocations || hierarchyHold.getHierarchyReservedBudget() != held)
        {
            std::cerr << "Visual hierarchy global hold unexpectedly changed category allocations.\n";
            return 1;
        }

        GenerationComplexityBudget noWeapon = GenerationComplexityBudget::create(mediumTraits, ShipStyle::FIGHTER, ShipFactionType::FRONTIER);
        GenerationComplexityBudget withWeapon = noWeapon;
        for (GenerationComplexityCategory category : { GenerationComplexityCategory::SILHOUETTE, GenerationComplexityCategory::MAJOR_FEATURE })
        {
            noWeapon.finalizeCategory(category);
            withWeapon.finalizeCategory(category);
        }
        const uint32_t weaponSpend = std::min(18u, withWeapon.getCategoryAvailable(GenerationComplexityCategory::LARGE_WEAPON));
        if (weaponSpend == 0u || !withWeapon.tryConsume(GenerationComplexityCategory::LARGE_WEAPON, weaponSpend))
        {
            std::cerr << "Representative weapon cost could not consume its reserved budget.\n";
            return 1;
        }
        noWeapon.finalizeCategory(GenerationComplexityCategory::LARGE_WEAPON);
        withWeapon.finalizeCategory(GenerationComplexityCategory::LARGE_WEAPON);
        if (withWeapon.getCategoryAvailable(GenerationComplexityCategory::DETAIL) >= noWeapon.getCategoryAvailable(GenerationComplexityCategory::DETAIL))
        {
            std::cerr << "Large weapon spending does not reduce later optional clutter opportunity.\n";
            return 1;
        }

        const BudgetSample tinySample = sampleGeneration(24u, ShipStyle::INDUSTRIAL, ShipFactionType::FRONTIER);
        const BudgetSample mediumSample = sampleGeneration(64u, ShipStyle::INDUSTRIAL, ShipFactionType::FRONTIER);
        const BudgetSample largeSample = sampleGeneration(160u, ShipStyle::INDUSTRIAL, ShipFactionType::FRONTIER);

        if (!(tinySample.Initial < mediumSample.Initial && mediumSample.Initial < largeSample.Initial))
        {
            std::cerr << "Generated ships did not receive progressive scale-aware budgets.\n";
            return 1;
        }

        if (!(tinySample.DominantFeatures < largeSample.DominantFeatures))
        {
            std::cerr << "Small ships are not compositionally simpler than large ships across the sample.\n";
            return 1;
        }

        for (std::size_t index = 0u; index < mediumSample.CategorySeen.size(); ++index)
        {
            if (!mediumSample.CategorySeen[index])
            {
                std::cerr << "A complexity category was permanently starved across the medium-size sample.\n";
                return 1;
            }
        }

        const BudgetSample sparseSample = sampleGeneration(64u, ShipStyle::SLEEK, ShipFactionType::ASCENDANT);
        if (!sparseSample.FoundSparse)
        {
            std::cerr << "The budget system eliminated intentionally sparse medium ships.\n";
            return 1;
        }

        constexpr std::array<ShipDimensions, 6u> RectangularDimensions =
        { {
            { 32u, 44u }, { 44u, 32u }, { 48u, 64u }, { 64u, 48u }, { 64u, 96u }, { 96u, 64u }
        } };

        ShipGenerator generator;
        for (std::size_t index = 0u; index < RectangularDimensions.size(); ++index)
        {
            ShipGenerationSettings settings;
            settings.Seed = 0xA4093822299F31D0ull ^ static_cast<uint64_t>(index);
            settings.Dimensions = RectangularDimensions[index];
            settings.Style = static_cast<ShipStyle>(index % static_cast<std::size_t>(ShipStyle::SHIP_STYLE_END));
            settings.Faction = static_cast<ShipFactionType>(index % static_cast<std::size_t>(ShipFactionType::SHIP_FACTION_TYPE_END));

            ShipGenerationDebugInfo debug;
            const GeneratedShip ship = generator.generate(settings, &debug);
            if (ship.FinalImage.getWidth() != settings.Dimensions.Width || ship.FinalImage.getHeight() != settings.Dimensions.Height || debug.ComplexityInitialBudget == 0u)
            {
                std::cerr << "Complexity budget failed on rectangular dimensions.\n";
                return 1;
            }
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Generation complexity budget regression failed: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
