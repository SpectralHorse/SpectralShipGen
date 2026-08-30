#include "ShipFiringAnimator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "AnimationSamplingPlanner.h"
#include "GenerationDomain.h"
#include "GenerationScaleTraits.h"
#include "ShipGenerationSeeds.h"

namespace
{
    using namespace PixelShipGenerator;

    constexpr uint64_t FiringSeedSalt = 0x4E86C291AD735BF0ull;

    struct PixelCoordinate
    {
        uint32_t X = 0u;
        uint32_t Y = 0u;
    };

    struct FiringWeaponPlan
    {
        uint32_t ComponentIndex = 0u;
        ShipWeaponAnimationComponent Component;
        std::vector<PixelCoordinate> MovablePixels;
        std::vector<PixelCoordinate> FrontEdgePixels;
        uint32_t MaximumRecoilTravel = 0u;
        uint32_t MaximumPreFireExtension = 0u;
        int32_t UnderlyingOffsetX = 0;
        int32_t UnderlyingOffsetY = 0;
    };

    struct FiringProfile
    {
        uint32_t BaseDurationMilliseconds = 210u;
        uint32_t RecoilLimit = 1u;
        uint32_t PreFireExtensionLimit = 0u;
        uint32_t ResponseStrengthPercent = 100u;
        bool HeavyResponse = false;
        bool Responsive = false;
    };

    struct FiringPlan
    {
        uint64_t Seed = 0u;
        ShipFiringAnimationTarget Target;
        FiringProfile Profile;
        uint32_t DurationMilliseconds = 0u;
        std::vector<FiringWeaponPlan> Weapons;
        ShipFiringAnimationDiagnostics Diagnostics;
        AnimationSamplingRequirements SamplingRequirements;
    };

    double clampNormalizedTime(double normalizedTime)
    {
        if (!std::isfinite(normalizedTime)) { return 0.0; }
        return std::clamp(normalizedTime, 0.0, 1.0);
    }

    double smoothStep(double value)
    {
        const double t = std::clamp(value, 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }

    double easeOutCubic(double value)
    {
        const double t = std::clamp(value, 0.0, 1.0);
        const double inverse = 1.0 - t;
        return 1.0 - inverse * inverse * inverse;
    }

    FiringProfile getFiringProfile(const GeneratedShip& ship, ShipWeaponType type)
    {
        FiringProfile profile;
        switch (type)
        {
        case ShipWeaponType::SINGLE_CANNON:
            profile.BaseDurationMilliseconds = 180u;
            profile.RecoilLimit = 1u;
            profile.Responsive = true;
            break;
        case ShipWeaponType::TWIN_CANNON:
            profile.BaseDurationMilliseconds = 210u;
            profile.RecoilLimit = 2u;
            profile.PreFireExtensionLimit = 1u;
            profile.Responsive = true;
            break;
        case ShipWeaponType::COMPACT_TURRET:
            profile.BaseDurationMilliseconds = 220u;
            profile.RecoilLimit = 1u;
            profile.PreFireExtensionLimit = 1u;
            break;
        case ShipWeaponType::RAIL_WEAPON:
            profile.BaseDurationMilliseconds = 310u;
            profile.RecoilLimit = 3u;
            profile.PreFireExtensionLimit = 1u;
            profile.HeavyResponse = true;
            break;
        case ShipWeaponType::WEAPON_POD:
            profile.BaseDurationMilliseconds = 260u;
            profile.RecoilLimit = 2u;
            profile.PreFireExtensionLimit = 1u;
            break;
        default:
            break;
        }

        const ShipFiringAnimationTraits& traits = ship.AnimationTraits.Firing;
        profile.ResponseStrengthPercent = traits.ResponseStrengthPercent;
        profile.BaseDurationMilliseconds += traits.DurationAdditionMilliseconds;
        if (traits.MaximumRecoilLimit != 0u) { profile.RecoilLimit = std::min(profile.RecoilLimit, traits.MaximumRecoilLimit); }
        profile.RecoilLimit += traits.AdditionalRecoilLimit;
        if (type == ShipWeaponType::RAIL_WEAPON) { profile.RecoilLimit += traits.RailWeaponAdditionalRecoilLimit; }
        profile.PreFireExtensionLimit = std::max(profile.PreFireExtensionLimit, traits.MinimumPreFireExtensionLimit);
        profile.HeavyResponse = profile.HeavyResponse || traits.HeavyResponse;
        profile.Responsive = profile.Responsive || traits.Responsive;

        switch (ship.Faction)
        {
        case ShipFactionType::MILITARY:
            profile.BaseDurationMilliseconds = profile.BaseDurationMilliseconds * 9u / 10u;
            break;
        case ShipFactionType::CORPORATE:
            profile.ResponseStrengthPercent = profile.ResponseStrengthPercent * 9u / 10u;
            break;
        case ShipFactionType::ASCENDANT:
            profile.ResponseStrengthPercent = profile.ResponseStrengthPercent * 4u / 5u;
            profile.PreFireExtensionLimit = std::min(profile.PreFireExtensionLimit, 1u);
            break;
        case ShipFactionType::RELIC:
            profile.BaseDurationMilliseconds += 65u;
            profile.HeavyResponse = true;
            break;
        case ShipFactionType::FRONTIER:
        case ShipFactionType::XENO:
        default:
            break;
        }

        profile.ResponseStrengthPercent = std::clamp(profile.ResponseStrengthPercent, 60u, 135u);
        profile.BaseDurationMilliseconds = std::clamp(profile.BaseDurationMilliseconds, 150u, 420u);
        return profile;
    }

    uint32_t getForwardDepth(const ShipWeaponAnimationComponent& component, const PixelCoordinate& pixel)
    {
        switch (component.Direction)
        {
        case ShipAttachmentDirection::UP: return pixel.Y >= component.MinimumY ? pixel.Y - component.MinimumY : 0u;
        case ShipAttachmentDirection::DOWN: return component.MaximumY >= pixel.Y ? component.MaximumY - pixel.Y : 0u;
        case ShipAttachmentDirection::LEFT: return pixel.X >= component.MinimumX ? pixel.X - component.MinimumX : 0u;
        case ShipAttachmentDirection::RIGHT: return component.MaximumX >= pixel.X ? component.MaximumX - pixel.X : 0u;
        default: return 0u;
        }
    }

    bool isProtectedStaticPixel(const GeneratedShip& ship, uint32_t x, uint32_t y)
    {
        return ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.EngineExhaustMask.get(x, y) || ship.AttachmentMask.get(x, y);
    }

    bool hasHullPixelInDirection(const GeneratedShip& ship, int32_t startX, int32_t startY, int32_t stepX, int32_t stepY)
    {
        int32_t x = startX + stepX;
        int32_t y = startY + stepY;
        while (x >= 0 && y >= 0 && x < static_cast<int32_t>(ship.HullMask.getWidth()) && y < static_cast<int32_t>(ship.HullMask.getHeight()))
        {
            if (ship.HullMask.get(static_cast<uint32_t>(x), static_cast<uint32_t>(y))) { return true; }
            x += stepX;
            y += stepY;
        }
        return false;
    }

    bool isLikelyStructuralVoid(const GeneratedShip& ship, uint32_t x, uint32_t y)
    {
        if (ship.HullMask.get(x, y) || ship.CockpitMask.get(x, y) || ship.EngineMask.get(x, y) || ship.AttachmentMask.get(x, y)) { return false; }
        const int32_t px = static_cast<int32_t>(x);
        const int32_t py = static_cast<int32_t>(y);
        const bool horizontallyEnclosed = hasHullPixelInDirection(ship, px, py, -1, 0) && hasHullPixelInDirection(ship, px, py, 1, 0);
        const bool verticallyEnclosed = hasHullPixelInDirection(ship, px, py, 0, -1) && hasHullPixelInDirection(ship, px, py, 0, 1);
        return horizontallyEnclosed || verticallyEnclosed;
    }

    std::pair<int32_t, int32_t> getForwardStep(ShipAttachmentDirection direction)
    {
        switch (direction)
        {
        case ShipAttachmentDirection::UP: return { 0, -1 };
        case ShipAttachmentDirection::DOWN: return { 0, 1 };
        case ShipAttachmentDirection::LEFT: return { -1, 0 };
        case ShipAttachmentDirection::RIGHT: return { 1, 0 };
        default: return { 0, -1 };
        }
    }

    std::vector<PixelCoordinate> collectMovablePixels(const GeneratedShip& ship, const ShipWeaponAnimationComponent& component)
    {
        std::vector<PixelCoordinate> pixels;
        if (!component.MovableBarrel || ship.IdleAnimationMetadata.WeaponMovableMask.empty()) { return pixels; }
        for (uint32_t y = component.MinimumY; y <= component.MaximumY && y < ship.IdleAnimationMetadata.WeaponMovableMask.getHeight(); ++y)
        {
            for (uint32_t x = component.MinimumX; x <= component.MaximumX && x < ship.IdleAnimationMetadata.WeaponMovableMask.getWidth(); ++x)
            {
                if (!ship.IdleAnimationMetadata.WeaponMovableMask.get(x, y)) { continue; }
                if (isProtectedStaticPixel(ship, x, y)) { continue; }
                pixels.push_back({ x, y });
            }
        }
        return pixels;
    }

    std::vector<PixelCoordinate> collectFrontEdgePixels(const ShipWeaponAnimationComponent& component, const std::vector<PixelCoordinate>& pixels)
    {
        std::vector<PixelCoordinate> result;
        for (const PixelCoordinate& pixel : pixels)
        {
            if (getForwardDepth(component, pixel) == 0u) { result.push_back(pixel); }
        }
        return result;
    }

    uint32_t getMaximumRecoilCapacity(const ShipWeaponAnimationComponent& component, const std::vector<PixelCoordinate>& pixels)
    {
        uint32_t maximumDepth = 0u;
        for (const PixelCoordinate& pixel : pixels) { maximumDepth = std::max(maximumDepth, getForwardDepth(component, pixel)); }
        return pixels.empty() ? 0u : maximumDepth;
    }

    bool isPreFireExtensionSafe(const GeneratedShip& ship, const Image& underlyingFrame, const FiringWeaponPlan& weapon, uint32_t extension)
    {
        if (extension == 0u || weapon.FrontEdgePixels.empty()) { return true; }
        const auto [stepX, stepY] = getForwardStep(weapon.Component.Direction);
        for (const PixelCoordinate& source : weapon.FrontEdgePixels)
        {
            const int32_t sourceX = static_cast<int32_t>(source.X) + weapon.UnderlyingOffsetX;
            const int32_t sourceY = static_cast<int32_t>(source.Y) + weapon.UnderlyingOffsetY;
            for (uint32_t distance = 1u; distance <= extension; ++distance)
            {
                const int32_t x = sourceX + stepX * static_cast<int32_t>(distance);
                const int32_t y = sourceY + stepY * static_cast<int32_t>(distance);
                if (x < 0 || y < 0 || x >= static_cast<int32_t>(underlyingFrame.getWidth()) || y >= static_cast<int32_t>(underlyingFrame.getHeight())) { return false; }
                const uint32_t px = static_cast<uint32_t>(x);
                const uint32_t py = static_cast<uint32_t>(y);
                if (underlyingFrame.getPixel(px, py).A != 0u) { return false; }
                if (isLikelyStructuralVoid(ship, px, py)) { return false; }
            }
        }
        return true;
    }

    uint32_t findSafePreFireExtension(const GeneratedShip& ship, const Image& underlyingFrame, const FiringWeaponPlan& weapon, uint32_t desiredExtension)
    {
        uint32_t safe = 0u;
        for (uint32_t extension = 1u; extension <= desiredExtension; ++extension)
        {
            if (!isPreFireExtensionSafe(ship, underlyingFrame, weapon, extension)) { break; }
            safe = extension;
        }
        return safe;
    }

    double sampleProfileResponse(const FiringProfile& profile, double value)
    {
        const double t = std::clamp(value, 0.0, 1.0);
        if (profile.Responsive) { return easeOutCubic(t); }
        if (profile.HeavyResponse) { return smoothStep(t * t); }
        return smoothStep(t);
    }

    double samplePreFireResponse(const FiringProfile& profile, double normalizedTime)
    {
        const double t = clampNormalizedTime(normalizedTime);
        if (t <= 0.0 || t >= 0.27) { return 0.0; }
        if (t < 0.17) { return sampleProfileResponse(profile, t / 0.17); }
        return 1.0 - smoothStep((t - 0.17) / 0.10);
    }

    double sampleRecoilResponse(const FiringProfile& profile, double normalizedTime)
    {
        const double t = clampNormalizedTime(normalizedTime);
        if (t <= 0.17 || t >= 1.0) { return 0.0; }
        if (t < 0.28) { return easeOutCubic((t - 0.17) / 0.11); }
        if (t < 0.40)
        {
            const double settle = smoothStep((t - 0.28) / 0.12);
            return 1.0 - 0.12 * settle;
        }
        const double recovery = (t - 0.40) / 0.60;
        return (1.0 - sampleProfileResponse(profile, recovery)) * 0.88;
    }

    uint32_t quantizeTravel(uint32_t maximumTravel, double response)
    {
        if (maximumTravel == 0u) { return 0u; }
        return std::min(maximumTravel, static_cast<uint32_t>(std::floor(std::clamp(response, 0.0, 1.0) * static_cast<double>(maximumTravel) + 0.5)));
    }


    std::vector<uint32_t> resolveTargetIndices(const GeneratedShip& ship, const ShipFiringAnimationTarget& target)
    {
        std::vector<uint32_t> indices;
        const auto& components = ship.IdleAnimationMetadata.WeaponComponents;
        if (target.WeaponComponentIndex >= components.size()) { return indices; }
        const ShipWeaponAnimationComponent& selected = components[target.WeaponComponentIndex];
        if (!target.IncludeSymmetryGroup || selected.SymmetryGroup == 0u)
        {
            indices.push_back(target.WeaponComponentIndex);
            return indices;
        }
        for (uint32_t index = 0u; index < components.size(); ++index)
        {
            if (components[index].SymmetryGroup == selected.SymmetryGroup) { indices.push_back(index); }
        }
        return indices;
    }

    FiringPlan buildPlan(const GeneratedShip& ship, const ShipAnimationPose& underlyingPose, const ShipFiringAnimationTarget& target, const ShipFiringAnimationSettings& settings)
    {
        if (underlyingPose.Frame.getWidth() != ship.FinalImage.getWidth() || underlyingPose.Frame.getHeight() != ship.FinalImage.getHeight())
        {
            throw std::invalid_argument("ShipFiringAnimator underlying pose dimensions must match the GeneratedShip.");
        }
        FiringPlan plan;
        plan.Target = target;
        plan.Diagnostics.TargetWeaponComponentIndex = target.WeaponComponentIndex;
        const auto& components = ship.IdleAnimationMetadata.WeaponComponents;
        if (target.WeaponComponentIndex >= components.size()) { return plan; }

        const ShipWeaponAnimationComponent& selected = components[target.WeaponComponentIndex];
        plan.Profile = getFiringProfile(ship, selected.Type);
        plan.Seed = settings.Seed.value_or(mixGenerationSeed64(ship.DomainSeeds.get(GenerationDomain::WEAPONS) ^ FiringSeedSalt ^ static_cast<uint64_t>(target.WeaponComponentIndex)));
        const GenerationScaleTraits scaleTraits = GenerationScaleTraits::fromDimensions({ ship.FinalImage.getWidth(), ship.FinalImage.getHeight() });
        const uint32_t scaleTravelCapacity = 1u + scaleTraits.AnimationComplexity / 45u;
        const std::vector<uint32_t> indices = resolveTargetIndices(ship, target);

        uint32_t maximumRecoil = 0u;
        uint32_t maximumPreFire = 0u;
        for (uint32_t index : indices)
        {
            const ShipWeaponAnimationComponent& component = components[index];
            FiringWeaponPlan weapon;
            weapon.ComponentIndex = index;
            weapon.Component = component;
            weapon.MovablePixels = collectMovablePixels(ship, component);
            weapon.FrontEdgePixels = collectFrontEdgePixels(component, weapon.MovablePixels);
            if (const ShipAnimationComponentTransform* transform = findAnimationComponentTransform(underlyingPose, ShipAnimationSemanticComponentType::WEAPON, index))
            {
                weapon.UnderlyingOffsetX = transform->OffsetX;
                weapon.UnderlyingOffsetY = transform->OffsetY;
            }

            const uint32_t recoilCapacity = getMaximumRecoilCapacity(component, weapon.MovablePixels);
            const uint32_t profileRecoil = std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(plan.Profile.RecoilLimit) * plan.Profile.ResponseStrengthPercent + 99u) / 100u));
            weapon.MaximumRecoilTravel = std::min({ recoilCapacity, profileRecoil, scaleTravelCapacity });

            if (settings.PreFireMotion && plan.Profile.PreFireExtensionLimit > 0u)
            {
                weapon.MaximumPreFireExtension = findSafePreFireExtension(ship, underlyingPose.Frame, weapon, std::min(plan.Profile.PreFireExtensionLimit, scaleTravelCapacity));
            }

            maximumRecoil = std::max(maximumRecoil, weapon.MaximumRecoilTravel);
            maximumPreFire = std::max(maximumPreFire, weapon.MaximumPreFireExtension);

            ShipFiringWeaponDiagnostic diagnostic;
            diagnostic.WeaponComponentIndex = index;
            diagnostic.Type = component.Type;
            diagnostic.Region = component.Region;
            diagnostic.SymmetryGroup = component.SymmetryGroup;
            diagnostic.MovablePixelCount = static_cast<uint32_t>(weapon.MovablePixels.size());
            diagnostic.MaximumRecoilTravelPixels = weapon.MaximumRecoilTravel;
            diagnostic.MaximumPreFireExtensionPixels = weapon.MaximumPreFireExtension;
            plan.Diagnostics.Weapons.push_back(diagnostic);
            plan.Weapons.push_back(std::move(weapon));
        }

        plan.DurationMilliseconds = plan.Profile.BaseDurationMilliseconds + maximumRecoil * 18u + maximumPreFire * 12u;
        plan.DurationMilliseconds = std::clamp(plan.DurationMilliseconds, 150u, 450u);
        plan.Diagnostics.ValidTarget = !plan.Weapons.empty();
        plan.Diagnostics.TargetSymmetryGroup = selected.SymmetryGroup;
        plan.Diagnostics.PairedOrGrouped = plan.Weapons.size() > 1u;
        plan.Diagnostics.PreFireMotion = maximumPreFire > 0u;
        plan.Diagnostics.ActiveWeaponCount = static_cast<uint32_t>(plan.Weapons.size());
        plan.Diagnostics.MaximumRecoilTravelPixels = maximumRecoil;
        plan.Diagnostics.MaximumPreFireExtensionPixels = maximumPreFire;
        plan.Diagnostics.IndependentPhaseGroupCount = plan.Weapons.empty() ? 0u : 1u;

        plan.SamplingRequirements.Type = ShipAnimationType::FIRE;
        plan.SamplingRequirements.Mode = settings.SamplingMode;
        plan.SamplingRequirements.DurationMilliseconds = plan.DurationMilliseconds;
        plan.SamplingRequirements.ExactFrameCount = settings.ExactFrameCount;
        plan.SamplingRequirements.MinimumFrameCount = settings.MinimumFrameCount;
        plan.SamplingRequirements.MaximumFrameCount = settings.MaximumFrameCount;
        plan.SamplingRequirements.MaximumMechanicalTravelPixels = std::max(maximumRecoil, maximumPreFire);
        plan.SamplingRequirements.ActiveAnimatedComponentCount = static_cast<uint32_t>(plan.Weapons.size());
        plan.SamplingRequirements.IndependentPhaseGroupCount = plan.Diagnostics.IndependentPhaseGroupCount;

        // FIRE is short, but its temporal demand is not identical for every weapon. A simple one-pixel
        // cannon still needs enough samples for the recoil/recovery snap, while deeper recoil and an
        // actual pre-fire mechanism add independently visible transitions. Keep this input explicit
        // rather than forcing every firing event to the same high temporal requirement.
        uint32_t temporalTransitions = plan.Weapons.empty() ? 0u : 2u; // recoil + recovery
        if (maximumPreFire > 0u) { ++temporalTransitions; }
        if (maximumRecoil > 1u && plan.Profile.Responsive) { ++temporalTransitions; }
        if (maximumRecoil >= 3u) { ++temporalTransitions; }
        plan.SamplingRequirements.MaximumTemporalCyclesPerClip = std::min(4u, temporalTransitions);
        plan.SamplingRequirements.ScaleAnimationComplexity = scaleTraits.AnimationComplexity;
        return plan;
    }

    void applyWeaponFrame(Image& frame, const Image& underlyingFrame, const FiringWeaponPlan& weapon, uint32_t recoilTravel, uint32_t preFireExtension)
    {
        for (const PixelCoordinate& pixel : weapon.MovablePixels)
        {
            if (recoilTravel == 0u || getForwardDepth(weapon.Component, pixel) >= recoilTravel) { continue; }
            const int32_t x = static_cast<int32_t>(pixel.X) + weapon.UnderlyingOffsetX;
            const int32_t y = static_cast<int32_t>(pixel.Y) + weapon.UnderlyingOffsetY;
            if (x >= 0 && y >= 0 && x < static_cast<int32_t>(frame.getWidth()) && y < static_cast<int32_t>(frame.getHeight()))
            {
                frame.setPixel(static_cast<uint32_t>(x), static_cast<uint32_t>(y), Color(0u, 0u, 0u, 0u));
            }
        }

        if (preFireExtension == 0u || weapon.FrontEdgePixels.empty()) { return; }
        const auto [stepX, stepY] = getForwardStep(weapon.Component.Direction);
        for (const PixelCoordinate& source : weapon.FrontEdgePixels)
        {
            const int32_t sourceX = static_cast<int32_t>(source.X) + weapon.UnderlyingOffsetX;
            const int32_t sourceY = static_cast<int32_t>(source.Y) + weapon.UnderlyingOffsetY;
            if (sourceX < 0 || sourceY < 0 || sourceX >= static_cast<int32_t>(underlyingFrame.getWidth()) || sourceY >= static_cast<int32_t>(underlyingFrame.getHeight())) { continue; }
            const Color color = underlyingFrame.getPixel(static_cast<uint32_t>(sourceX), static_cast<uint32_t>(sourceY));
            for (uint32_t distance = 1u; distance <= preFireExtension; ++distance)
            {
                const int32_t x = sourceX + stepX * static_cast<int32_t>(distance);
                const int32_t y = sourceY + stepY * static_cast<int32_t>(distance);
                if (x >= 0 && y >= 0 && x < static_cast<int32_t>(frame.getWidth()) && y < static_cast<int32_t>(frame.getHeight())) { frame.setPixel(static_cast<uint32_t>(x), static_cast<uint32_t>(y), color); }
            }
        }
    }

    Image evaluatePlanFrame(const ShipAnimationPose& underlyingPose, const FiringPlan& plan, double normalizedTime)
    {
        Image frame = underlyingPose.Frame;
        if (!plan.Diagnostics.ValidTarget) { return frame; }
        const double t = clampNormalizedTime(normalizedTime);
        if (t <= 0.0 || t >= 1.0) { return frame; }

        const double recoilResponse = sampleRecoilResponse(plan.Profile, t);
        const double preFireResponse = samplePreFireResponse(plan.Profile, t);
        for (const FiringWeaponPlan& weapon : plan.Weapons)
        {
            const uint32_t recoil = quantizeTravel(weapon.MaximumRecoilTravel, recoilResponse);
            const uint32_t preFire = quantizeTravel(weapon.MaximumPreFireExtension, preFireResponse);
            applyWeaponFrame(frame, underlyingPose.Frame, weapon, recoil, preFire);
        }
        return frame;
    }
}

namespace PixelShipGenerator
{
    ShipFiringAnimationPhase getFiringAnimationPhase(double normalizedTime)
    {
        const double t = clampNormalizedTime(normalizedTime);
        if (t <= 0.0 || t >= 1.0) { return ShipFiringAnimationPhase::REST; }
        if (t < 0.17) { return ShipFiringAnimationPhase::PRE_FIRE; }
        if (t < 0.40) { return ShipFiringAnimationPhase::RECOIL; }
        return ShipFiringAnimationPhase::RECOVERY;
    }

    std::vector<ShipFiringAnimationTarget> ShipFiringAnimator::getAvailableTargets(const GeneratedShip& ship) const
    {
        std::vector<ShipFiringAnimationTarget> targets;
        std::set<uint32_t> emittedSymmetryGroups;
        const auto& components = ship.IdleAnimationMetadata.WeaponComponents;
        for (uint32_t index = 0u; index < components.size(); ++index)
        {
            const ShipWeaponAnimationComponent& component = components[index];
            if (!component.MovableBarrel) { continue; }
            if (component.SymmetryGroup != 0u)
            {
                if (!emittedSymmetryGroups.insert(component.SymmetryGroup).second) { continue; }
                targets.push_back({ index, true });
            }
            else
            {
                targets.push_back({ index, false });
            }
        }
        return targets;
    }

    ShipFiringAnimation ShipFiringAnimator::generate(const GeneratedShip& ship, const ShipFiringAnimationTarget& target, const ShipFiringAnimationSettings& settings) const
    {
        ShipAnimationPose neutralPose;
        neutralPose.Frame = ship.FinalImage;
        neutralPose.Layer = ShipAnimationPoseLayer::STATIC_NEUTRAL;
        neutralPose.UnderlyingAnimationType = ShipAnimationType::IDLE;
        return generate(ship, neutralPose, target, settings);
    }

    ShipFiringAnimation ShipFiringAnimator::generate(const GeneratedShip& ship, const ShipAnimationPose& underlyingPose, const ShipFiringAnimationTarget& target, const ShipFiringAnimationSettings& settings) const
    {
        ShipFiringAnimation animation;
        animation.Target = target;
        animation.FrameWidth = ship.FinalImage.getWidth();
        animation.FrameHeight = ship.FinalImage.getHeight();
        const FiringPlan plan = buildPlan(ship, underlyingPose, target, settings);
        animation.Seed = plan.Seed;
        animation.DurationMilliseconds = plan.DurationMilliseconds;
        animation.Diagnostics = plan.Diagnostics;

        if (!plan.Diagnostics.ValidTarget)
        {
            return animation;
        }

        AnimationSamplingPlanner samplingPlanner;
        animation.Sampling = samplingPlanner.plan(plan.SamplingRequirements);
        animation.FrameDurationMilliseconds = animation.Sampling.ActualFrameDurationMilliseconds;
        animation.Frames.reserve(animation.Sampling.ActualFrameCount);
        animation.NormalizedSampleTimes.reserve(animation.Sampling.ActualFrameCount);
        for (uint32_t frameIndex = 0u; frameIndex < animation.Sampling.ActualFrameCount; ++frameIndex)
        {
            const double normalizedTime = static_cast<double>(frameIndex) / static_cast<double>(animation.Sampling.ActualFrameCount);
            animation.NormalizedSampleTimes.push_back(normalizedTime);
            animation.Frames.push_back(evaluatePlanFrame(underlyingPose, plan, normalizedTime));
        }
        return animation;
    }

    Image ShipFiringAnimator::evaluateFrameAtNormalizedTime(const GeneratedShip& ship, const ShipFiringAnimationTarget& target, double normalizedTime, const ShipFiringAnimationSettings& settings) const
    {
        ShipAnimationPose neutralPose;
        neutralPose.Frame = ship.FinalImage;
        neutralPose.Layer = ShipAnimationPoseLayer::STATIC_NEUTRAL;
        neutralPose.UnderlyingAnimationType = ShipAnimationType::IDLE;
        return evaluateFrameAtNormalizedTime(ship, neutralPose, target, normalizedTime, settings);
    }

    Image ShipFiringAnimator::evaluateFrameAtNormalizedTime(const GeneratedShip& ship, const ShipAnimationPose& underlyingPose, const ShipFiringAnimationTarget& target, double normalizedTime, const ShipFiringAnimationSettings& settings) const
    {
        const FiringPlan plan = buildPlan(ship, underlyingPose, target, settings);
        return evaluatePlanFrame(underlyingPose, plan, normalizedTime);
    }
}
