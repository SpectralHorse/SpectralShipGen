#include "CoreRegressionSuites.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

#include <PixelShipGenerator/ShipGenerator.h>
#include <PixelShipGenerator/ShipLateralMovementAnimator.h>
#include <PixelShipGenerator/ShipSpritesheetUtils.h>

namespace
{
    using namespace PixelShipGenerator;

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    bool masksSymmetric(const PixelMask& mask)
    {
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (mask.get(x, y) != mask.get(mask.getWidth() - 1u - x, y)) { return false; }
            }
        }
        return true;
    }

    uint32_t countMaskPixels(const PixelMask& mask)
    {
        uint32_t count = 0u;
        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
                count += mask.get(x, y) ? 1u : 0u;
        return count;
    }

    bool clipsEqual(const ShipMovementAnimationClip& first, const ShipMovementAnimationClip& second)
    {
        if (first.Type != second.Type || first.Phase != second.Phase || first.Looping != second.Looping || first.NormalizedSampleTimes != second.NormalizedSampleTimes || first.FrameWidth != second.FrameWidth || first.FrameHeight != second.FrameHeight || first.DurationMilliseconds != second.DurationMilliseconds || std::abs(first.FrameDurationMilliseconds - second.FrameDurationMilliseconds) > 0.000001 || first.Frames.size() != second.Frames.size()) { return false; }
        if (first.Sampling.ActualFrameCount != second.Sampling.ActualFrameCount || first.Sampling.MaximumMechanicalTravelPixels != second.Sampling.MaximumMechanicalTravelPixels || first.Sampling.MaximumExhaustTravelPixels != second.Sampling.MaximumExhaustTravelPixels || first.Sampling.ActiveAnimatedComponentCount != second.Sampling.ActiveAnimatedComponentCount || first.Sampling.IndependentPhaseGroupCount != second.Sampling.IndependentPhaseGroupCount) { return false; }
        for (std::size_t index = 0u; index < first.Frames.size(); ++index) { if (!imagesEqual(first.Frames[index], second.Frames[index])) { return false; } }
        return true;
    }

    bool diagnosticsEqual(const ShipMovementAnimationDiagnostics& first, const ShipMovementAnimationDiagnostics& second)
    {
        if (first.DirectionSignX != second.DirectionSignX || first.DirectionSignY != second.DirectionSignY || first.ActiveEngineCount != second.ActiveEngineCount || first.ActiveWeaponCount != second.ActiveWeaponCount || first.ActiveAttachmentCount != second.ActiveAttachmentCount || first.ActiveBrakingComponentCount != second.ActiveBrakingComponentCount || first.MaximumMechanicalTravelPixels != second.MaximumMechanicalTravelPixels || first.MaximumExhaustTravelPixels != second.MaximumExhaustTravelPixels || first.IndependentPhaseGroupCount != second.IndependentPhaseGroupCount || first.Components.size() != second.Components.size()) { return false; }
        for (std::size_t index = 0u; index < first.Components.size(); ++index)
        {
            const ShipMovementComponentDiagnostic& a = first.Components[index];
            const ShipMovementComponentDiagnostic& b = second.Components[index];
            if (a.Type != b.Type || a.SemanticGroup != b.SemanticGroup || a.SourcePixelCount != b.SourcePixelCount || a.MaximumOffsetX != b.MaximumOffsetX || a.MaximumOffsetY != b.MaximumOffsetY || a.MaximumExhaustLengthDelta != b.MaximumExhaustLengthDelta || std::abs(a.SustainPhaseOffset - b.SustainPhaseOffset) > 0.000001) { return false; }
        }
        return true;
    }

    ShipGenerationSettings makeSettings(uint64_t seed, ShipDimensions dimensions, ShipStyle style, ShipFactionType faction, bool attachments = true)
    {
        ShipGenerationSettings settings;
        settings.Seed = seed;
        settings.Dimensions = dimensions;
        settings.Style = style;
        settings.Faction = faction;
        settings.AttachmentsEnabled = attachments;
        return settings;
    }

    bool validateProtectedGeometry(const GeneratedShip& ship, const ShipGenerationDebugInfo* debug, const ShipMovementAnimationClip& clip)
    {
        for (const Image& frame : clip.Frames)
        {
            if (frame.getWidth() != ship.FinalImage.getWidth() || frame.getHeight() != ship.FinalImage.getHeight()) { return false; }

            for (uint32_t y = 0u; y < frame.getHeight(); ++y)
            {
                for (uint32_t x = 0u; x < frame.getWidth(); ++x)
                {
                    if (ship.CockpitMask.get(x, y) && frame.getPixel(x, y) != ship.FinalImage.getPixel(x, y)) { return false; }
                    if (ship.HullMask.get(x, y) && frame.getPixel(x, y).A == 0u) { return false; }
                    if (debug != nullptr && !debug->ReservedNegativeSpaceMask.empty() && debug->ReservedNegativeSpaceMask.get(x, y) && frame.getPixel(x, y) != ship.FinalImage.getPixel(x, y)) { return false; }
                }
            }
        }
        return true;
    }

    bool validateSymmetricSemanticMirroring(const ShipMovementAnimation& left, const ShipMovementAnimation& right)
    {
        if (left.Diagnostics.DirectionSignX != -1 || right.Diagnostics.DirectionSignX != 1) { return false; }
        if (left.Diagnostics.Components.size() != right.Diagnostics.Components.size()) { return false; }

        using Key = std::tuple<uint32_t, uint32_t, uint32_t>;
        std::vector<std::pair<Key, ShipMovementComponentDiagnostic>> leftComponents;
        std::vector<std::pair<Key, ShipMovementComponentDiagnostic>> rightComponents;

        auto append = [](const ShipMovementAnimationDiagnostics& diagnostics, std::vector<std::pair<Key, ShipMovementComponentDiagnostic>>& output)
            {
                for (const ShipMovementComponentDiagnostic& component : diagnostics.Components)
                {
                    const Key key{ static_cast<uint32_t>(component.Type), component.SemanticGroup, component.SourcePixelCount };
                    output.push_back({ key, component });
                }
                std::sort(output.begin(), output.end(), [](const auto& first, const auto& second) { return first.first < second.first; });
            };

        append(left.Diagnostics, leftComponents);
        append(right.Diagnostics, rightComponents);
        if (leftComponents.size() != rightComponents.size()) { return false; }

        for (std::size_t index = 0u; index < leftComponents.size(); ++index)
        {
            if (leftComponents[index].first != rightComponents[index].first) { return false; }
            const ShipMovementComponentDiagnostic& a = leftComponents[index].second;
            const ShipMovementComponentDiagnostic& b = rightComponents[index].second;
            if (a.MaximumOffsetX != -b.MaximumOffsetX || std::abs(a.SustainPhaseOffset - b.SustainPhaseOffset) > 0.000001) { return false; }
        }
        return true;
    }

    bool validateSpritesheet(const ShipMovementAnimationClip& clip)
    {
        const Image sheet = createHorizontalSpritesheet(clip);
        if (clip.Frames.empty()) { return sheet.empty(); }
        if (sheet.getWidth() != clip.FrameWidth * clip.Frames.size() || sheet.getHeight() != clip.FrameHeight) { return false; }
        for (uint32_t frameIndex = 0u; frameIndex < clip.Frames.size(); ++frameIndex)
            for (uint32_t y = 0u; y < clip.FrameHeight; ++y)
                for (uint32_t x = 0u; x < clip.FrameWidth; ++x)
                    if (sheet.getPixel(frameIndex * clip.FrameWidth + x, y) != clip.Frames[frameIndex].getPixel(x, y)) { return false; }
        return true;
    }
}

int PixelShipGeneratorTests::runLateralMovementAnimationRegression()
{
    using namespace PixelShipGenerator;

    ShipGenerator generator;
    ShipLateralMovementAnimator animator;

    const GeneratedShip symmetricShip = generator.generate(makeSettings(0u, { 64u,64u }, ShipStyle::FIGHTER, ShipFactionType::MILITARY));
    if (!masksSymmetric(symmetricShip.HullMask) || !masksSymmetric(symmetricShip.EngineMask) || !masksSymmetric(symmetricShip.AttachmentMask))
    {
        std::cerr << "Task 68 regression failed: symmetric semantic fixture is no longer symmetric.\n";
        return 1;
    }

    const ShipMovementAnimation left = animator.generate(symmetricShip, ShipAnimationType::MOVE_LEFT);
    const ShipMovementAnimation leftRepeat = animator.generate(symmetricShip, ShipAnimationType::MOVE_LEFT);
    const ShipMovementAnimation right = animator.generate(symmetricShip, ShipAnimationType::MOVE_RIGHT);

    if (left.Seed != leftRepeat.Seed || !diagnosticsEqual(left.Diagnostics, leftRepeat.Diagnostics) || !clipsEqual(left.Enter, leftRepeat.Enter) || !clipsEqual(left.Sustain, leftRepeat.Sustain) || !clipsEqual(left.Exit, leftRepeat.Exit))
    {
        std::cerr << "Task 68 regression failed: lateral movement plan/output is not deterministic.\n";
        return 1;
    }

    if (!imagesEqual(left.Enter.Frames.front(), symmetricShip.FinalImage) || !imagesEqual(left.Exit.Frames.back(), symmetricShip.FinalImage))
    {
        std::cerr << "Task 68 regression failed: Enter time 0 or Exit endpoint lost static GeneratedShip identity.\n";
        return 1;
    }

    if (left.Sustain.Frames.empty() || std::any_of(left.Sustain.Frames.begin(), left.Sustain.Frames.end(), [&](const Image& frame) { return imagesEqual(frame, symmetricShip.FinalImage); }))
    {
        std::cerr << "Task 68 regression failed: sustained lateral posture collapses back to neutral.\n";
        return 1;
    }

    if (!imagesEqual(left.Enter.Frames.back(), left.Sustain.Frames.front()) || !imagesEqual(left.Sustain.Frames.front(), left.Exit.Frames.front()))
    {
        std::cerr << "Task 68 regression failed: Enter/Sustain/Exit movement posture is discontinuous at the sustained-state boundary.\n";
        return 1;
    }

    if (!validateSymmetricSemanticMirroring(left, right))
    {
        std::cerr << "Task 68 regression failed: symmetric LEFT/RIGHT movement diagnostics are not semantic mirrors.\n";
        return 1;
    }

    if (!validateSpritesheet(left.Enter) || !validateSpritesheet(left.Sustain) || !validateSpritesheet(left.Exit))
    {
        std::cerr << "Task 68 regression failed: movement spritesheet export dimensions/order are invalid.\n";
        return 1;
    }

    ShipMovementAnimationSettings exact10;
    exact10.SamplingMode = AnimationSamplingMode::EXACT_FRAME_COUNT;
    exact10.TransitionFrameCount = 10u;
    exact10.SustainFrameCount = 10u;
    ShipMovementAnimationSettings exact20 = exact10;
    exact20.TransitionFrameCount = 20u;
    exact20.SustainFrameCount = 20u;
    const Image density10 = animator.evaluateFrameAtNormalizedTime(symmetricShip, ShipAnimationType::MOVE_LEFT, ShipMovementAnimationPhase::SUSTAIN, 0.37, exact10);
    const Image density20 = animator.evaluateFrameAtNormalizedTime(symmetricShip, ShipAnimationType::MOVE_LEFT, ShipMovementAnimationPhase::SUSTAIN, 0.37, exact20);
    if (!imagesEqual(density10, density20))
    {
        std::cerr << "Task 68 regression failed: equivalent normalized movement time depends on sampling density.\n";
        return 1;
    }

    const GeneratedShip noOpTiny = generator.generate(makeSettings(5u, { 24u,24u }, ShipStyle::SLEEK, ShipFactionType::CORPORATE));
    const ShipMovementAnimation noOpTinyMovement = animator.generate(noOpTiny, ShipAnimationType::MOVE_LEFT);
    const uint32_t noOpActiveComponents = noOpTinyMovement.Diagnostics.ActiveEngineCount + noOpTinyMovement.Diagnostics.ActiveWeaponCount + noOpTinyMovement.Diagnostics.ActiveAttachmentCount;
    if (noOpActiveComponents == 0u && (noOpTinyMovement.Enter.Sampling.ActualFrameCount != 1u || noOpTinyMovement.Sustain.Sampling.ActualFrameCount != 1u || noOpTinyMovement.Exit.Sampling.ActualFrameCount != 1u))
    {
        std::cerr << "Task 68 regression failed: an animation with no safe active components inflated into redundant frames.\n";
        return 1;
    }

    const GeneratedShip simpleLarge = generator.generate(makeSettings(2u, { 128u,128u }, ShipStyle::SLEEK, ShipFactionType::CORPORATE));
    const GeneratedShip complexSmaller = generator.generate(makeSettings(3u, { 96u,96u }, ShipStyle::INDUSTRIAL, ShipFactionType::XENO));
    const ShipMovementAnimation simpleAnimation = animator.generate(simpleLarge, ShipAnimationType::MOVE_LEFT);
    const ShipMovementAnimation complexAnimation = animator.generate(complexSmaller, ShipAnimationType::MOVE_LEFT);
    if (complexAnimation.Sustain.Sampling.ActualFrameCount <= simpleAnimation.Sustain.Sampling.ActualFrameCount)
    {
        std::cerr << "Task 68 regression failed: adaptive lateral sampling collapsed to resolution-only ordering.\n";
        return 1;
    }

    // Asymmetry is generation-dependent. Search deterministically for a ship that actually
    // exercises direction-specific safe fallback instead of pinning that semantic to one seed.
    bool foundAsymmetricFallbackFixture = false;
    for (uint64_t attempt = 0u; attempt < 32u && !foundAsymmetricFallbackFixture; ++attempt)
    {
        const GeneratedShip asymmetricShip = generator.generate(makeSettings(attempt, { 96u,96u }, ShipStyle::INDUSTRIAL, ShipFactionType::FRONTIER));
        if (masksSymmetric(asymmetricShip.HullMask) && masksSymmetric(asymmetricShip.AttachmentMask))
        {
            continue;
        }

        const ShipMovementAnimation asymmetricLeft = animator.generate(asymmetricShip, ShipAnimationType::MOVE_LEFT);
        const ShipMovementAnimation asymmetricRight = animator.generate(asymmetricShip, ShipAnimationType::MOVE_RIGHT);
        if (asymmetricLeft.Diagnostics.ActiveAttachmentCount == asymmetricRight.Diagnostics.ActiveAttachmentCount && asymmetricLeft.Diagnostics.Components.size() == asymmetricRight.Diagnostics.Components.size())
        {
            continue;
        }

        if (!validateProtectedGeometry(asymmetricShip, nullptr, asymmetricLeft.Enter) || !validateProtectedGeometry(asymmetricShip, nullptr, asymmetricLeft.Sustain) || !validateProtectedGeometry(asymmetricShip, nullptr, asymmetricLeft.Exit) || !validateProtectedGeometry(asymmetricShip, nullptr, asymmetricRight.Enter) || !validateProtectedGeometry(asymmetricShip, nullptr, asymmetricRight.Sustain) || !validateProtectedGeometry(asymmetricShip, nullptr, asymmetricRight.Exit))
        {
            std::cerr << "Task 68 regression failed: asymmetric movement changed protected hull/cockpit geometry.\n";
            return 1;
        }

        foundAsymmetricFallbackFixture = true;
    }

    if (!foundAsymmetricFallbackFixture)
    {
        std::cerr << "Task 68 regression failed: deterministic fixture search found no direction-specific asymmetric fallback case.\n";
        return 1;
    }

    ShipGenerationDebugInfo negativeSpaceDebug;
    const GeneratedShip negativeSpaceShip = generator.generate(makeSettings(0x5700030040400010ull, { 64u,64u }, ShipStyle::INDUSTRIAL, ShipFactionType::CORPORATE), &negativeSpaceDebug);
    if (countMaskPixels(negativeSpaceDebug.ReservedNegativeSpaceMask) == 0u)
    {
        std::cerr << "Task 68 regression failed: Task-57 compatibility fixture no longer contains reserved negative space.\n";
        return 1;
    }
    const ShipMovementAnimation negativeSpaceMovement = animator.generate(negativeSpaceShip, ShipAnimationType::MOVE_RIGHT);
    if (!validateProtectedGeometry(negativeSpaceShip, &negativeSpaceDebug, negativeSpaceMovement.Enter) || !validateProtectedGeometry(negativeSpaceShip, &negativeSpaceDebug, negativeSpaceMovement.Sustain) || !validateProtectedGeometry(negativeSpaceShip, &negativeSpaceDebug, negativeSpaceMovement.Exit))
    {
        std::cerr << "Task 68 regression failed: lateral animation entered reserved Task-57 negative space or modified cockpit/hull occupancy.\n";
        return 1;
    }

    const GeneratedShip rectangularShip = generator.generate(makeSettings(0x6800000000000048ull, { 80u,48u }, ShipStyle::DELTA, ShipFactionType::MILITARY));
    const ShipMovementAnimation rectangularMovement = animator.generate(rectangularShip, ShipAnimationType::MOVE_LEFT);
    if (rectangularMovement.Enter.FrameWidth != 80u || rectangularMovement.Enter.FrameHeight != 48u || rectangularMovement.Sustain.FrameWidth != 80u || rectangularMovement.Sustain.FrameHeight != 48u || !validateProtectedGeometry(rectangularShip, nullptr, rectangularMovement.Sustain))
    {
        std::cerr << "Task 68 regression failed: rectangular lateral animation dimensions/safety are invalid.\n";
        return 1;
    }

    constexpr std::array<ShipStyle, 6u> Styles = { ShipStyle::SLEEK, ShipStyle::FIGHTER, ShipStyle::HEAVY, ShipStyle::INDUSTRIAL, ShipStyle::SPEARHEAD, ShipStyle::DELTA };
    constexpr std::array<ShipFactionType, 6u> Factions = { ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT, ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC };
    uint64_t seed = 0x6800000000001000ull;
    for (ShipStyle style : Styles)
    {
        for (ShipFactionType faction : Factions)
        {
            const GeneratedShip ship = generator.generate(makeSettings(seed++, { 44u,44u }, style, faction));
            const ShipMovementAnimation movement = animator.generate(ship, (seed & 1ull) == 0ull ? ShipAnimationType::MOVE_LEFT : ShipAnimationType::MOVE_RIGHT);
            if (movement.Enter.Frames.empty() || movement.Sustain.Frames.empty() || movement.Exit.Frames.empty() || !imagesEqual(movement.Enter.Frames.front(), ship.FinalImage) || !imagesEqual(movement.Exit.Frames.back(), ship.FinalImage) || !validateProtectedGeometry(ship, nullptr, movement.Sustain))
            {
                std::cerr << "Task 68 regression failed: style/faction coverage produced invalid lateral animation.\n";
                return 1;
            }
        }
    }

    std::cout << "Task 68 lateral movement animation regression passed.\n";
    return 0;
}
