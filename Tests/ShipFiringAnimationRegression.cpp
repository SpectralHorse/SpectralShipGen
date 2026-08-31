#include "CoreRegressionSuites.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>

#include <SpectralShipGen/ShipFiringAnimator.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipSpritesheetUtils.h>

namespace
{
    using namespace SpectralShipGen;

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    ShipGenerationSettings makeSettings(uint64_t seed, ShipDimensions dimensions, ShipStyle style, ShipFactionType faction)
    {
        ShipGenerationSettings settings;
        settings.Seed = seed;
        settings.Dimensions = dimensions;
        settings.Style = style;
        settings.Faction = faction;
        return settings;
    }

    uint32_t countMaskPixels(const PixelMask& mask)
    {
        uint32_t count = 0u;
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
                count += mask.get(x, y) ? 1u : 0u;
        return count;
    }

    bool animationsEqual(const ShipFiringAnimation& first, const ShipFiringAnimation& second)
    {
        if (first.Seed != second.Seed || first.Target.WeaponComponentIndex != second.Target.WeaponComponentIndex || first.Target.IncludeSymmetryGroup != second.Target.IncludeSymmetryGroup || first.NormalizedSampleTimes != second.NormalizedSampleTimes || first.FrameWidth != second.FrameWidth || first.FrameHeight != second.FrameHeight || first.DurationMilliseconds != second.DurationMilliseconds || std::abs(first.FrameDurationMilliseconds - second.FrameDurationMilliseconds) > 0.000001 || first.Frames.size() != second.Frames.size()) { return false; }
        if (first.Sampling.ActualFrameCount != second.Sampling.ActualFrameCount || first.Sampling.MaximumMechanicalTravelPixels != second.Sampling.MaximumMechanicalTravelPixels || first.Diagnostics.ValidTarget != second.Diagnostics.ValidTarget || first.Diagnostics.TargetSymmetryGroup != second.Diagnostics.TargetSymmetryGroup || first.Diagnostics.ActiveWeaponCount != second.Diagnostics.ActiveWeaponCount || first.Diagnostics.MaximumRecoilTravelPixels != second.Diagnostics.MaximumRecoilTravelPixels || first.Diagnostics.MaximumPreFireExtensionPixels != second.Diagnostics.MaximumPreFireExtensionPixels || first.Diagnostics.Weapons.size() != second.Diagnostics.Weapons.size()) { return false; }
        for (std::size_t index = 0u; index < first.Frames.size(); ++index) { if (!imagesEqual(first.Frames[index], second.Frames[index])) { return false; } }
        return true;
    }

    bool validateProtectedGeometry(const GeneratedShip& ship, const ShipGenerationDebugInfo* debug, const ShipFiringAnimation& animation)
    {
        std::set<uint32_t> sourceColors;
        for (const Color& color : ship.FinalImage.getPixels())
        {
            if (color.A != 0u) { sourceColors.insert((static_cast<uint32_t>(color.R) << 24u) | (static_cast<uint32_t>(color.G) << 16u) | (static_cast<uint32_t>(color.B) << 8u) | color.A); }
        }

        for (const Image& frame : animation.Frames)
        {
            if (frame.getWidth() != ship.FinalImage.getWidth() || frame.getHeight() != ship.FinalImage.getHeight()) { return false; }
            for (uint32_t y = 0u; y < frame.getHeight(); ++y)
            {
                for (uint32_t x = 0u; x < frame.getWidth(); ++x)
                {
                    if ((ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y) || ship.AttachmentMask.get(x, y)) && frame.getPixel(x, y) != ship.FinalImage.getPixel(x, y)) { return false; }
                    if (debug != nullptr && !debug->ReservedNegativeSpaceMask.empty() && debug->ReservedNegativeSpaceMask.get(x, y) && frame.getPixel(x, y) != ship.FinalImage.getPixel(x, y)) { return false; }
                    const Color color = frame.getPixel(x, y);
                    if (color.A != 0u)
                    {
                        const uint32_t packed = (static_cast<uint32_t>(color.R) << 24u) | (static_cast<uint32_t>(color.G) << 16u) | (static_cast<uint32_t>(color.B) << 8u) | color.A;
                        if (sourceColors.count(packed) == 0u) { return false; }
                    }
                }
            }
        }
        return true;
    }

    GeneratedShip makeSyntheticWeaponShip(ShipDimensions dimensions, ShipStyle style, ShipFactionType faction, ShipWeaponType type, ShipWeaponHardpointRegion region, uint32_t barrelLength)
    {
        GeneratedShip ship;
        const ShipGenerationSeeds seeds = deriveShipGenerationSeeds(0x70F1A1A1ull + dimensions.Width * 257ull + dimensions.Height);
        ship.reset(dimensions.Width, dimensions.Height, seeds);
        ship.DomainSeeds = resolveGenerationDomainSeeds(seeds, {});
        ship.AnimationTraits = getShipGenerationProfile(style).AnimationTraits;
        ship.Provenance.StructuralPreset = style;
        ship.Provenance.FactionPreset = faction;

        const Color hullColor(70u, 75u, 88u, 255u);
        const Color weaponColor(185u, 190u, 202u, 255u);
        const uint32_t centerX = dimensions.Width / 2u;
        const uint32_t bodyMinY = std::max(4u, dimensions.Height / 2u - 4u);
        const uint32_t bodyMaxY = bodyMinY + 2u;
        const uint32_t barrelMaxY = bodyMinY - 1u;
        const uint32_t actualBarrelLength = std::min(barrelLength, barrelMaxY);
        const uint32_t barrelMinY = barrelMaxY + 1u - actualBarrelLength;

        for (uint32_t y = bodyMaxY + 1u; y < dimensions.Height - 2u; ++y)
        {
            for (uint32_t x = dimensions.Width / 4u; x < dimensions.Width - dimensions.Width / 4u; ++x)
            {
                ship.HullMask.set(x, y, true);
                ship.FinalImage.setPixel(x, y, hullColor);
            }
        }

        for (uint32_t y = bodyMinY; y <= bodyMaxY; ++y)
        {
            for (uint32_t x = centerX - 1u; x <= centerX + 1u; ++x)
            {
                ship.IdleAnimationMetadata.WeaponOccupiedMask.set(x, y, true);
                ship.FinalImage.setPixel(x, y, weaponColor);
            }
        }
        for (uint32_t y = barrelMinY; y <= barrelMaxY; ++y)
        {
            ship.IdleAnimationMetadata.WeaponOccupiedMask.set(centerX, y, true);
            ship.IdleAnimationMetadata.WeaponMovableMask.set(centerX, y, true);
            ship.FinalImage.setPixel(centerX, y, weaponColor);
        }
        ship.IdleAnimationMetadata.WeaponMuzzleMask.set(centerX, barrelMinY, true);

        ShipWeaponAnimationComponent component;
        component.Type = type;
        component.Region = region;
        component.Direction = ShipAttachmentDirection::UP;
        component.AnchorX = centerX;
        component.AnchorY = bodyMaxY;
        component.MinimumX = centerX - 1u;
        component.MaximumX = centerX + 1u;
        component.MinimumY = barrelMinY;
        component.MaximumY = bodyMaxY;
        component.SymmetryGroup = 0u;
        component.MovableBarrel = true;
        ship.IdleAnimationMetadata.WeaponComponents.push_back(component);
        return ship;
    }
}

int SpectralShipGenTests::runFiringAnimationRegression()
{
    using namespace SpectralShipGen;

    ShipGenerator generator;
    ShipFiringAnimator animator;

    const GeneratedShip pairedShip = generator.generate(makeSettings(0x7000000000000000ull, { 96u,96u }, ShipStyle::INDUSTRIAL, ShipFactionType::FRONTIER));
    const std::vector<ShipFiringAnimationTarget> pairedTargets = animator.getAvailableTargets(pairedShip);
    if (pairedShip.IdleAnimationMetadata.WeaponComponents.size() != 2u || pairedTargets.size() != 1u)
    {
        std::cerr << "Task 70 regression failed: paired weapon fixture/target grouping changed.\n";
        return 1;
    }

    const ShipFiringAnimation firing = animator.generate(pairedShip, pairedTargets.front());
    const ShipFiringAnimation firingRepeat = animator.generate(pairedShip, pairedTargets.front());
    if (!animationsEqual(firing, firingRepeat) || !firing.Diagnostics.ValidTarget || !firing.Diagnostics.PairedOrGrouped || firing.Diagnostics.ActiveWeaponCount != 2u || firing.Diagnostics.TargetSymmetryGroup == 0u)
    {
        std::cerr << "Task 70 regression failed: firing plan/output is not deterministic or paired targeting is invalid.\n";
        return 1;
    }

    if (firing.Frames.empty() || !imagesEqual(firing.Frames.front(), pairedShip.FinalImage) || !imagesEqual(animator.evaluateFrameAtNormalizedTime(pairedShip, pairedTargets.front(), 0.0), pairedShip.FinalImage) || !imagesEqual(animator.evaluateFrameAtNormalizedTime(pairedShip, pairedTargets.front(), 1.0), pairedShip.FinalImage))
    {
        std::cerr << "Task 70 regression failed: one-shot rest identity is invalid.\n";
        return 1;
    }

    if (getFiringAnimationPhase(0.0) != ShipFiringAnimationPhase::REST || getFiringAnimationPhase(0.10) != ShipFiringAnimationPhase::PRE_FIRE || getFiringAnimationPhase(0.25) != ShipFiringAnimationPhase::RECOIL || getFiringAnimationPhase(0.70) != ShipFiringAnimationPhase::RECOVERY || getFiringAnimationPhase(1.0) != ShipFiringAnimationPhase::REST)
    {
        std::cerr << "Task 70 regression failed: firing semantic phase timeline is invalid.\n";
        return 1;
    }

    const Image maximumRecoil = animator.evaluateFrameAtNormalizedTime(pairedShip, pairedTargets.front(), 0.28);
    if (imagesEqual(maximumRecoil, pairedShip.FinalImage) || firing.Diagnostics.MaximumRecoilTravelPixels == 0u)
    {
        std::cerr << "Task 70 regression failed: firing recoil does not produce mechanical change.\n";
        return 1;
    }

    if (!validateProtectedGeometry(pairedShip, nullptr, firing))
    {
        std::cerr << "Task 70 regression failed: firing changed protected ship geometry or introduced non-mechanical colors.\n";
        return 1;
    }

    // Paired weapons share one semantic firing group and therefore use the same recoil timing.
    const auto& firstComponent = pairedShip.IdleAnimationMetadata.WeaponComponents[0u];
    const auto& secondComponent = pairedShip.IdleAnimationMetadata.WeaponComponents[1u];
    auto changedCountInBounds = [&](const ShipWeaponAnimationComponent& component)
    {
        uint32_t changed = 0u;
        for (uint32_t y = component.MinimumY; y <= component.MaximumY; ++y)
            for (uint32_t x = component.MinimumX; x <= component.MaximumX; ++x)
                changed += maximumRecoil.getPixel(x, y) != pairedShip.FinalImage.getPixel(x, y) ? 1u : 0u;
        return changed;
    };
    if (changedCountInBounds(firstComponent) == 0u || changedCountInBounds(firstComponent) != changedCountInBounds(secondComponent))
    {
        std::cerr << "Task 70 regression failed: paired recoil is not synchronized.\n";
        return 1;
    }

    const GeneratedShip multiTargetShip = generator.generate(makeSettings(0x7000000000000103ull, { 96u,96u }, ShipStyle::DELTA, ShipFactionType::XENO));
    const auto multiTargets = animator.getAvailableTargets(multiTargetShip);
    if (multiTargets.size() < 2u)
    {
        std::cerr << "Task 70 regression failed: multi-target weapon fixture changed.\n";
        return 1;
    }
    const ShipFiringAnimation targetA = animator.generate(multiTargetShip, multiTargets[0u]);
    const ShipFiringAnimation targetB = animator.generate(multiTargetShip, multiTargets[1u]);
    if (targetA.Target.WeaponComponentIndex == targetB.Target.WeaponComponentIndex || imagesEqual(animator.evaluateFrameAtNormalizedTime(multiTargetShip, multiTargets[0u], 0.28), animator.evaluateFrameAtNormalizedTime(multiTargetShip, multiTargets[1u], 0.28)))
    {
        std::cerr << "Task 70 regression failed: explicit weapon target selection is ineffective.\n";
        return 1;
    }

    const Image targetAPeak = animator.evaluateFrameAtNormalizedTime(multiTargetShip, multiTargets[0u], 0.28);
    const ShipWeaponAnimationComponent& untargetedComponent = multiTargetShip.IdleAnimationMetadata.WeaponComponents[multiTargets[1u].WeaponComponentIndex];
    for (uint32_t y = untargetedComponent.MinimumY; y <= untargetedComponent.MaximumY; ++y)
    {
        for (uint32_t x = untargetedComponent.MinimumX; x <= untargetedComponent.MaximumX; ++x)
        {
            if (targetAPeak.getPixel(x, y) != multiTargetShip.FinalImage.getPixel(x, y))
            {
                std::cerr << "Task 70 regression failed: firing one weapon target modified an unrelated weapon group.\n";
                return 1;
            }
        }
    }

    auto findGeneratedTarget = [&](ShipWeaponHardpointRegion desiredRegion, ShipStyle style, ShipFactionType faction, uint64_t seedBase)
        -> std::optional<std::pair<GeneratedShip, ShipFiringAnimationTarget>>
        {
            for (uint32_t attempt = 0u; attempt < 64u; ++attempt)
            {
                GeneratedShip ship = generator.generate(makeSettings(seedBase + attempt, { 96u,96u }, style, faction));
                for (const ShipFiringAnimationTarget target : animator.getAvailableTargets(ship))
                {
                    if (ship.IdleAnimationMetadata.WeaponComponents[target.WeaponComponentIndex].Region == desiredRegion)
                    {
                        return std::make_pair(std::move(ship), target);
                    }
                }
            }
            return std::nullopt;
        };

    const auto centralFixture = findGeneratedTarget(ShipWeaponHardpointRegion::CENTRAL_NOSE, ShipStyle::INDUSTRIAL, ShipFactionType::FRONTIER, 0x7000000000000000ull);
    const auto wingFixture = findGeneratedTarget(ShipWeaponHardpointRegion::OUTER_WING, ShipStyle::FIGHTER, ShipFactionType::MILITARY, 0x7100000000000000ull);
    if (!centralFixture.has_value() || !wingFixture.has_value())
    {
        std::cerr << "Task 70 regression failed: unable to find deterministic central/wing weapon fixtures.\n";
        return 1;
    }
    const GeneratedShip& centralShip = centralFixture->first;
    const ShipFiringAnimationTarget centralTarget = centralFixture->second;
    const GeneratedShip& wingShip = wingFixture->first;
    const ShipFiringAnimationTarget wingTarget = wingFixture->second;
    const ShipFiringAnimation centralFire = animator.generate(centralShip, centralTarget);
    const ShipFiringAnimation wingFire = animator.generate(wingShip, wingTarget);
    if (centralFire.Diagnostics.Weapons.front().Region != ShipWeaponHardpointRegion::CENTRAL_NOSE || wingFire.Diagnostics.Weapons.front().Region != ShipWeaponHardpointRegion::OUTER_WING || imagesEqual(animator.evaluateFrameAtNormalizedTime(centralShip, centralTarget, 0.28), centralShip.FinalImage) || imagesEqual(animator.evaluateFrameAtNormalizedTime(wingShip, wingTarget, 0.28), wingShip.FinalImage))
    {
        std::cerr << "Task 70 regression failed: central/wing firing variants are not represented by actual weapon semantics.\n";
        return 1;
    }

    ShipFiringAnimationSettings exactSettings;
    exactSettings.SamplingMode = AnimationSamplingMode::EXACT_FRAME_COUNT;
    exactSettings.ExactFrameCount = 20u;
    const ShipFiringAnimation exact20 = animator.generate(pairedShip, pairedTargets.front(), exactSettings);
    if (exact20.Frames.size() != 20u || exact20.NormalizedSampleTimes.front() != 0.0 || exact20.NormalizedSampleTimes.back() >= 1.0)
    {
        std::cerr << "Task 70 regression failed: firing sampling endpoint convention is invalid.\n";
        return 1;
    }

    const GeneratedShip simpleSynthetic = makeSyntheticWeaponShip({ 32u,32u }, ShipStyle::SLEEK, ShipFactionType::CORPORATE, ShipWeaponType::SINGLE_CANNON, ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE, 3u);
    const GeneratedShip complexSynthetic = makeSyntheticWeaponShip({ 128u,128u }, ShipStyle::HEAVY, ShipFactionType::RELIC, ShipWeaponType::RAIL_WEAPON, ShipWeaponHardpointRegion::CENTRAL_BODY, 10u);
    const ShipFiringAnimation simpleFire = animator.generate(simpleSynthetic, { 0u, false });
    const ShipFiringAnimation complexFire = animator.generate(complexSynthetic, { 0u, false });
    if (complexFire.DurationMilliseconds <= simpleFire.DurationMilliseconds || complexFire.Diagnostics.MaximumRecoilTravelPixels < simpleFire.Diagnostics.MaximumRecoilTravelPixels || complexFire.Sampling.ActualFrameCount <= simpleFire.Sampling.ActualFrameCount)
    {
        std::cerr << "Task 70 regression failed: adaptive firing duration/sampling does not reflect weapon scale/complexity.\n";
        return 1;
    }

    const GeneratedShip preFireSynthetic = makeSyntheticWeaponShip({ 64u,48u }, ShipStyle::INDUSTRIAL, ShipFactionType::FRONTIER, ShipWeaponType::RAIL_WEAPON, ShipWeaponHardpointRegion::CENTRAL_NOSE, 7u);
    const ShipFiringAnimation preFire = animator.generate(preFireSynthetic, { 0u, false });
    if (preFire.FrameWidth != 64u || preFire.FrameHeight != 48u || preFire.Diagnostics.MaximumPreFireExtensionPixels == 0u || imagesEqual(animator.evaluateFrameAtNormalizedTime(preFireSynthetic, { 0u, false }, 0.15), preFireSynthetic.FinalImage) || !validateProtectedGeometry(preFireSynthetic, nullptr, preFire))
    {
        std::cerr << "Task 70 regression failed: rectangular/pre-fire mechanical animation is invalid.\n";
        return 1;
    }

    ShipGenerationDebugInfo negativeSpaceDebug;
    const GeneratedShip negativeSpaceShip = generator.generate(makeSettings(0x5700030040400017ull, { 64u,64u }, ShipStyle::INDUSTRIAL, ShipFactionType::CORPORATE), &negativeSpaceDebug);
    const auto negativeTargets = animator.getAvailableTargets(negativeSpaceShip);
    if (countMaskPixels(negativeSpaceDebug.ReservedNegativeSpaceMask) == 0u || negativeTargets.empty())
    {
        std::cerr << "Task 70 regression failed: negative-space firing fixture changed.\n";
        return 1;
    }
    const ShipFiringAnimation negativeFire = animator.generate(negativeSpaceShip, negativeTargets.front());
    if (!validateProtectedGeometry(negativeSpaceShip, &negativeSpaceDebug, negativeFire))
    {
        std::cerr << "Task 70 regression failed: firing entered reserved Task-57 negative space.\n";
        return 1;
    }

    constexpr std::array<ShipStyle, 6u> Styles = { ShipStyle::SLEEK, ShipStyle::FIGHTER, ShipStyle::HEAVY, ShipStyle::INDUSTRIAL, ShipStyle::SPEARHEAD, ShipStyle::DELTA };
    constexpr std::array<ShipFactionType, 6u> Factions = { ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT, ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC };
    for (ShipStyle style : Styles)
    {
        for (ShipFactionType faction : Factions)
        {
            GeneratedShip ship = makeSyntheticWeaponShip({ 44u,44u }, style, faction, ShipWeaponType::TWIN_CANNON, ShipWeaponHardpointRegion::WING_ROOT, 5u);
            const ShipFiringAnimation animation = animator.generate(ship, { 0u, false });
            if (animation.Frames.empty() || !imagesEqual(animation.Frames.front(), ship.FinalImage) || !imagesEqual(animator.evaluateFrameAtNormalizedTime(ship, { 0u, false }, 1.0), ship.FinalImage) || !validateProtectedGeometry(ship, nullptr, animation))
            {
                std::cerr << "Task 70 regression failed: style/faction firing coverage produced invalid output.\n";
                return 1;
            }
        }
    }

    const Image sheet = createHorizontalSpritesheet(firing);
    const Image repeatSheet = createHorizontalSpritesheet(firingRepeat);
    if (!imagesEqual(sheet, repeatSheet) || sheet.getWidth() != firing.FrameWidth * firing.Frames.size() || sheet.getHeight() != firing.FrameHeight)
    {
        std::cerr << "Task 70 regression failed: firing spritesheet export is not deterministic or uses wrong dimensions.\n";
        return 1;
    }

    std::cout << "Task 70 weapon firing animation regression passed.\n";
    return 0;
}
