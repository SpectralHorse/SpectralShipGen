#include "CoreRegressionSuites.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

#include <PixelShipGenerator/ShipGenerator.h>
#include <PixelShipGenerator/ShipLongitudinalMovementAnimator.h>
#include <PixelShipGenerator/ShipSpritesheetUtils.h>

namespace
{
    using namespace PixelShipGenerator;

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
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
                    if (ship.HullMask.get(x, y) && frame.getPixel(x, y) != ship.FinalImage.getPixel(x, y)) { return false; }
                    if (debug != nullptr && !debug->ReservedNegativeSpaceMask.empty() && debug->ReservedNegativeSpaceMask.get(x, y) && frame.getPixel(x, y) != ship.FinalImage.getPixel(x, y)) { return false; }
                }
            }
        }
        return true;
    }

    bool validateEngineRoots(const GeneratedShip& ship, const ShipMovementAnimationClip& clip)
    {
        for (const ShipEngineAnimationComponent& engine : ship.IdleAnimationMetadata.EngineComponents)
        {
            if (engine.RootStartY >= ship.EngineMask.getHeight()) { return false; }
            for (const Image& frame : clip.Frames)
            {
                bool hasRootPixel = false;
                for (uint32_t x = engine.HousingStartX; x < engine.HousingStartX + engine.HousingWidth && x < ship.EngineMask.getWidth(); ++x)
                {
                    if (ship.EngineMask.get(x, engine.RootStartY) && frame.getPixel(x, engine.RootStartY).A != 0u) { hasRootPixel = true; }
                }
                if (!hasRootPixel) { return false; }
            }
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

int PixelShipGeneratorTests::runLongitudinalMovementAnimationRegression()
{
    using namespace PixelShipGenerator;

    ShipGenerator generator;
    ShipLongitudinalMovementAnimator animator;

    const GeneratedShip propulsionShip = generator.generate(makeSettings(0x6900000000000001ull, { 96u,96u }, ShipStyle::INDUSTRIAL, ShipFactionType::XENO));
    if (propulsionShip.IdleAnimationMetadata.EngineComponents.empty())
    {
        std::cerr << "Task 69 regression failed: propulsion fixture has no semantic engines.\n";
        return 1;
    }

    const ShipMovementAnimation up = animator.generate(propulsionShip, ShipAnimationType::MOVE_UP);
    const ShipMovementAnimation upRepeat = animator.generate(propulsionShip, ShipAnimationType::MOVE_UP);
    const ShipMovementAnimation down = animator.generate(propulsionShip, ShipAnimationType::MOVE_DOWN);

    if (up.Seed != upRepeat.Seed || !diagnosticsEqual(up.Diagnostics, upRepeat.Diagnostics) || !clipsEqual(up.Enter, upRepeat.Enter) || !clipsEqual(up.Sustain, upRepeat.Sustain) || !clipsEqual(up.Exit, upRepeat.Exit))
    {
        std::cerr << "Task 69 regression failed: longitudinal movement plan/output is not deterministic.\n";
        return 1;
    }

    if (up.Diagnostics.DirectionSignY != -1 || down.Diagnostics.DirectionSignY != 1 || up.Diagnostics.DirectionSignX != 0 || down.Diagnostics.DirectionSignX != 0)
    {
        std::cerr << "Task 69 regression failed: longitudinal direction semantics are invalid.\n";
        return 1;
    }

    if (!imagesEqual(up.Enter.Frames.front(), propulsionShip.FinalImage) || !imagesEqual(up.Exit.Frames.back(), propulsionShip.FinalImage) || !imagesEqual(down.Enter.Frames.front(), propulsionShip.FinalImage) || !imagesEqual(down.Exit.Frames.back(), propulsionShip.FinalImage))
    {
        std::cerr << "Task 69 regression failed: Enter time 0 or Exit endpoint lost static GeneratedShip identity.\n";
        return 1;
    }

    if (!imagesEqual(up.Enter.Frames.back(), up.Sustain.Frames.front()) || !imagesEqual(up.Sustain.Frames.front(), up.Exit.Frames.front()) || !imagesEqual(down.Enter.Frames.back(), down.Sustain.Frames.front()) || !imagesEqual(down.Sustain.Frames.front(), down.Exit.Frames.front()))
    {
        std::cerr << "Task 69 regression failed: Enter/Sustain/Exit longitudinal posture is discontinuous.\n";
        return 1;
    }

    if (imagesEqual(up.Sustain.Frames.front(), down.Sustain.Frames.front()) || up.Diagnostics.MaximumExhaustTravelPixels == 0u || down.Diagnostics.MaximumExhaustTravelPixels == 0u)
    {
        std::cerr << "Task 69 regression failed: MOVE_UP and MOVE_DOWN do not produce distinct propulsion/braking posture.\n";
        return 1;
    }

    if (!validateProtectedGeometry(propulsionShip, nullptr, up.Enter) || !validateProtectedGeometry(propulsionShip, nullptr, up.Sustain) || !validateProtectedGeometry(propulsionShip, nullptr, up.Exit) || !validateProtectedGeometry(propulsionShip, nullptr, down.Enter) || !validateProtectedGeometry(propulsionShip, nullptr, down.Sustain) || !validateProtectedGeometry(propulsionShip, nullptr, down.Exit) || !validateEngineRoots(propulsionShip, up.Sustain) || !validateEngineRoots(propulsionShip, down.Sustain))
    {
        std::cerr << "Task 69 regression failed: propulsion animation changed hull/cockpit geometry or invalidated engine roots.\n";
        return 1;
    }

    ShipMovementAnimationSettings exact10;
    exact10.SamplingMode = AnimationSamplingMode::EXACT_FRAME_COUNT;
    exact10.TransitionFrameCount = 10u;
    exact10.SustainFrameCount = 10u;
    ShipMovementAnimationSettings exact20 = exact10;
    exact20.TransitionFrameCount = 20u;
    exact20.SustainFrameCount = 20u;
    const Image density10 = animator.evaluateFrameAtNormalizedTime(propulsionShip, ShipAnimationType::MOVE_UP, ShipMovementAnimationPhase::SUSTAIN, 0.37, exact10);
    const Image density20 = animator.evaluateFrameAtNormalizedTime(propulsionShip, ShipAnimationType::MOVE_UP, ShipMovementAnimationPhase::SUSTAIN, 0.37, exact20);
    if (!imagesEqual(density10, density20))
    {
        std::cerr << "Task 69 regression failed: equivalent normalized longitudinal time depends on sampling density.\n";
        return 1;
    }

    if (!validateSpritesheet(up.Enter) || !validateSpritesheet(up.Sustain) || !validateSpritesheet(up.Exit) || !validateSpritesheet(down.Enter) || !validateSpritesheet(down.Sustain) || !validateSpritesheet(down.Exit))
    {
        std::cerr << "Task 69 regression failed: deterministic longitudinal spritesheet export is invalid.\n";
        return 1;
    }

    ShipGenerationDebugInfo negativeSpaceDebug;
    const GeneratedShip negativeSpaceShip = generator.generate(makeSettings(0x5700030040400010ull, { 64u,64u }, ShipStyle::INDUSTRIAL, ShipFactionType::CORPORATE), &negativeSpaceDebug);
    if (countMaskPixels(negativeSpaceDebug.ReservedNegativeSpaceMask) == 0u)
    {
        std::cerr << "Task 69 regression failed: Task-57 compatibility fixture no longer contains reserved negative space.\n";
        return 1;
    }
    const ShipMovementAnimation negativeSpaceUp = animator.generate(negativeSpaceShip, ShipAnimationType::MOVE_UP);
    const ShipMovementAnimation negativeSpaceDown = animator.generate(negativeSpaceShip, ShipAnimationType::MOVE_DOWN);
    if (!validateProtectedGeometry(negativeSpaceShip, &negativeSpaceDebug, negativeSpaceUp.Sustain) || !validateProtectedGeometry(negativeSpaceShip, &negativeSpaceDebug, negativeSpaceDown.Sustain))
    {
        std::cerr << "Task 69 regression failed: longitudinal animation entered reserved Task-57 negative space.\n";
        return 1;
    }

    const GeneratedShip rectangularShip = generator.generate(makeSettings(0x6900000000000048ull, { 80u,48u }, ShipStyle::DELTA, ShipFactionType::MILITARY));
    const ShipMovementAnimation rectangularMovement = animator.generate(rectangularShip, ShipAnimationType::MOVE_DOWN);
    if (rectangularMovement.Enter.FrameWidth != 80u || rectangularMovement.Enter.FrameHeight != 48u || rectangularMovement.Sustain.FrameWidth != 80u || rectangularMovement.Sustain.FrameHeight != 48u || !validateProtectedGeometry(rectangularShip, nullptr, rectangularMovement.Sustain))
    {
        std::cerr << "Task 69 regression failed: rectangular longitudinal animation dimensions/safety are invalid.\n";
        return 1;
    }

    const GeneratedShip simpleShip = generator.generate(makeSettings(0x6900000000000100ull, { 44u,44u }, ShipStyle::SLEEK, ShipFactionType::CORPORATE));
    const GeneratedShip complexShip = generator.generate(makeSettings(0x6900000000000101ull, { 96u,96u }, ShipStyle::INDUSTRIAL, ShipFactionType::XENO));
    const ShipMovementAnimation simpleMovement = animator.generate(simpleShip, ShipAnimationType::MOVE_UP);
    const ShipMovementAnimation complexMovement = animator.generate(complexShip, ShipAnimationType::MOVE_UP);
    if (complexMovement.Sustain.Sampling.ActualFrameCount < simpleMovement.Sustain.Sampling.ActualFrameCount || complexMovement.Sustain.Sampling.MaximumExhaustTravelPixels < simpleMovement.Sustain.Sampling.MaximumExhaustTravelPixels)
    {
        std::cerr << "Task 69 regression failed: adaptive longitudinal sampling does not reflect propulsion complexity.\n";
        return 1;
    }

    constexpr std::array<ShipStyle, 6u> Styles = { ShipStyle::SLEEK, ShipStyle::FIGHTER, ShipStyle::HEAVY, ShipStyle::INDUSTRIAL, ShipStyle::SPEARHEAD, ShipStyle::DELTA };
    constexpr std::array<ShipFactionType, 6u> Factions = { ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT, ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC };
    uint64_t seed = 0x6900000000001000ull;
    for (ShipStyle style : Styles)
    {
        for (ShipFactionType faction : Factions)
        {
            const GeneratedShip ship = generator.generate(makeSettings(seed++, { 44u,44u }, style, faction));
            const ShipAnimationType type = (seed & 1ull) == 0ull ? ShipAnimationType::MOVE_UP : ShipAnimationType::MOVE_DOWN;
            const ShipMovementAnimation movement = animator.generate(ship, type);
            if (movement.Enter.Frames.empty() || movement.Sustain.Frames.empty() || movement.Exit.Frames.empty() || !imagesEqual(movement.Enter.Frames.front(), ship.FinalImage) || !imagesEqual(movement.Exit.Frames.back(), ship.FinalImage) || !validateProtectedGeometry(ship, nullptr, movement.Sustain) || !validateEngineRoots(ship, movement.Sustain))
            {
                std::cerr << "Task 69 regression failed: style/faction coverage produced invalid longitudinal animation.\n";
                return 1;
            }
        }
    }

    std::cout << "Task 69 longitudinal movement animation regression passed.\n";
    return 0;
}
