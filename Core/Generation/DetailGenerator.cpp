#include "DetailGenerator.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <utility>
#include <vector>

#include "GenerationMath.h"
#include "MacroAsymmetryPlanner.h"
#include "PixelMaskUtils.h"
#include "ShipGenerationSeeds.h"
#include "ShipVisualAnchorType.h"

namespace PixelShipGenerator
{
    namespace
    {
        constexpr uint32_t AccentDetailComplexityCost = 2u;
        constexpr uint32_t MechanicalDetailComplexityCost = 2u;
        constexpr uint32_t LightDetailComplexityCost = 1u;
        constexpr uint32_t SupplementalDetailComplexityCost = 2u;
        constexpr uint64_t DetailMotifSalt = 0xA91E4C67D32BF805ull;

        uint32_t scalePercent(uint32_t value, uint32_t percent)
        {
            return static_cast<uint32_t>((static_cast<uint64_t>(value) * percent + 50u) / 100u);
        }

        uint32_t randomUInt(std::mt19937_64& randomGenerator, uint32_t minimum, uint32_t maximum)
        {
            if (maximum <= minimum) { return minimum; }
            return std::uniform_int_distribution<uint32_t>(minimum, maximum)(randomGenerator);
        }

        uint32_t getMotifScaleChancePercent(const GenerationScaleTraits& traits)
        {
            switch (traits.Tier)
            {
            case GenerationScaleTier::TINY: return 52u;
            case GenerationScaleTier::SMALL: return 82u;
            case GenerationScaleTier::MEDIUM: return 100u;
            case GenerationScaleTier::LARGE: return 108u;
            default: return 100u;
            }
        }

        uint32_t getBaseMotifOccurrences(const GenerationScaleTraits& traits)
        {
            switch (traits.Tier)
            {
            case GenerationScaleTier::TINY: return 1u;
            case GenerationScaleTier::SMALL: return 2u;
            case GenerationScaleTier::MEDIUM: return 3u;
            case GenerationScaleTier::LARGE: return 4u;
            default: return 1u;
            }
        }

        bool acceptSpatialMotif(ShipGenerationContext& context, const GenerationSpatialBudget::RegionSet& regions, uint32_t cost, std::mt19937_64& randomGenerator)
        {
            uint32_t preference = context.SpatialBudget.getDetailPreferencePercent(regions, cost);
            if (context.MacroAsymmetry.Fulfilled && context.MacroAsymmetry.BalanceStrategy == MacroAsymmetryBalanceStrategy::OPPOSITE_SUBTLE_DETAIL)
            {
                const GenerationSpatialRegion opposite = MacroAsymmetryPlanner::getOppositeRegion(context);
                if (opposite != context.MacroAsymmetry.TargetRegion && opposite != GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END && regions[static_cast<std::size_t>(opposite)])
                {
                    preference = std::min(100u, preference + 18u);
                }
            }
            if (preference == 0u || randomUInt(randomGenerator, 0u, 99u) >= preference)
            {
                context.SpatialBudget.recordRejection(regions);
                return false;
            }
            context.SpatialBudget.consume(regions, cost, false);
            return true;
        }

        bool acceptSpatialDetail(ShipGenerationContext& context, const GenerationSpatialBudget::RegionSet& regions, uint32_t cost)
        {
            uint32_t preference = context.SpatialBudget.getDetailPreferencePercent(regions, cost);
            if (context.MacroAsymmetry.Fulfilled && context.MacroAsymmetry.BalanceStrategy == MacroAsymmetryBalanceStrategy::OPPOSITE_SUBTLE_DETAIL)
            {
                const GenerationSpatialRegion opposite = MacroAsymmetryPlanner::getOppositeRegion(context);
                if (opposite != context.MacroAsymmetry.TargetRegion && opposite != GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END && regions[static_cast<std::size_t>(opposite)])
                {
                    preference = std::min(100u, preference + 18u);
                }
            }
            if (preference == 0u || context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) >= preference)
            {
                context.SpatialBudget.recordRejection(regions);
                return false;
            }
            context.SpatialBudget.consume(regions, cost, false);
            return true;
        }

        GenerationSpatialBudget::RegionSet collectRectangleRegions(const ShipGenerationContext& context, uint32_t startX, uint32_t startY, uint32_t width, uint32_t height, bool mirrored)
        {
            GenerationSpatialBudget::RegionSet regions = {};
            const uint32_t imageWidth = context.Ship.HullMask.getWidth();
            for (uint32_t y = startY; y < startY + height; ++y)
            {
                for (uint32_t x = startX; x < startX + width; ++x)
                {
                    context.SpatialBudget.addRegion(regions, context.SpatialBudget.getRegionAt(x, y));
                    if (mirrored) { context.SpatialBudget.addRegion(regions, context.SpatialBudget.getRegionAt(imageWidth - 1u - x, y)); }
                }
            }
            return regions;
        }
    }
    void DetailGenerator::generate(ShipGenerationContext& context) const
    {
        GeneratedShip& ship = context.Ship;
        const ShipGenerationConfiguration& settings = context.Settings;
        const ShipGenerationProfile& profile = context.Profile;
        ship.AccentMask.clear(false);
        ship.MechanicalDetailMask.clear(false);
        ship.LightMask.clear(false);
        context.DetailMotifs.reset(ship.HullMask.getWidth(), ship.HullMask.getHeight());

        ResolvedSurfaceDetailProfile detailProfile = resolveSurfaceDetailProfile(settings, profile, context.FactionProfile.SurfaceDetails);

        if (context.DebugInfo != nullptr)
        {
            context.DebugInfo->SurfaceDetailProfile = detailProfile;
            context.DebugInfo->HasSurfaceDetailProfile = true;
        }

        if (settings.RandomStreamMode != GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS)
        {
            std::mt19937_64 motifRandomGenerator(mixGenerationSeed64(context.DomainSeeds.get(GenerationDomain::DETAILS) ^ DetailMotifSalt));
            planDetailMotifs(context, detailProfile, motifRandomGenerator);
            generatePlannedMotifs(context, motifRandomGenerator);
        }

        const uint32_t freeformPercent = getFreeformDetailPercent(context);
        detailProfile.DetailDensityPercent = composeSurfaceDetailPercent(detailProfile.DetailDensityPercent, freeformPercent);

        generateAccentDetails(context, detailProfile);
        generateMechanicalDetails(context, detailProfile);
        generateLightDetails(context, detailProfile);
        generateSupplementalSurfaceDetails(context, detailProfile);
        context.updateDetailMotifDebugInfo();
    }

    void DetailGenerator::planDetailMotifs(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile, std::mt19937_64& randomGenerator) const
    {
        DetailMotifPlan& plan = context.DetailMotifs;
        if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::DETAIL, 1u)) { return; }

        uint32_t chance = scalePercent(context.Profile.DetailMotifChance, getMotifScaleChancePercent(context.ScaleTraits));
        if (context.VisualHierarchy.InfluenceEnabled && context.VisualHierarchy.PrimaryAnchor == ShipVisualAnchorType::SILHOUETTE)
        {
            chance = scalePercent(chance, 82u);
        }
        chance = std::min(100u, chance);
        if (randomUInt(randomGenerator, 0u, 99u) >= chance) { return; }

        plan.PrimaryMotif = selectDetailMotif(context, profile, randomGenerator, ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END);
        if (!plan.hasPrimary()) { return; }
        plan.PrimaryPreferredRegion = getPreferredMotifRegion(context, plan.PrimaryMotif, false, randomGenerator);

        const uint32_t repeatPercent = scalePercent(context.Profile.DetailMotifRepeatPercent, profile.MotifRepeatPercent);
        plan.PrimaryTargetOccurrences = std::clamp(scalePercent(getBaseMotifOccurrences(context.ScaleTraits), repeatPercent), 1u, 5u);
        if (context.ScaleTraits.Tier == GenerationScaleTier::TINY) { plan.PrimaryTargetOccurrences = 1u; }

        plan.MirroredChance = 100u - std::min(100u, profile.AsymmetricDetailChance);
        if (context.MacroAsymmetry.Fulfilled) { plan.MirroredChance = plan.MirroredChance > 24u ? plan.MirroredChance - 24u : 0u; }
        plan.MirroredChance = std::min(100u, plan.MirroredChance + context.Profile.DetailMotifMirroringBonusPercent);

        if (context.ScaleTraits.Tier == GenerationScaleTier::TINY) { return; }
        uint32_t secondaryChance = context.Profile.SecondaryDetailMotifChance;
        if (context.ScaleTraits.Tier == GenerationScaleTier::SMALL) { secondaryChance = scalePercent(secondaryChance, 70u); }
        else if (context.ScaleTraits.Tier == GenerationScaleTier::LARGE) { secondaryChance = std::min(100u, scalePercent(secondaryChance, 112u)); }
        if (randomUInt(randomGenerator, 0u, 99u) >= secondaryChance) { return; }

        plan.SecondaryMotif = selectDetailMotif(context, profile, randomGenerator, plan.PrimaryMotif);
        if (!plan.hasSecondary()) { return; }
        plan.SecondaryPreferredRegion = getPreferredMotifRegion(context, plan.SecondaryMotif, true, randomGenerator);
        plan.SecondaryTargetOccurrences = std::min(2u, std::max(1u, plan.PrimaryTargetOccurrences / 2u));
    }

    void DetailGenerator::generatePlannedMotifs(ShipGenerationContext& context, std::mt19937_64& randomGenerator) const
    {
        DetailMotifPlan& plan = context.DetailMotifs;
        if (!plan.hasPrimary()) { return; }

        for (uint32_t occurrence = 0u; occurrence < plan.PrimaryTargetOccurrences; ++occurrence)
        {
            if (tryGenerateMotifOccurrence(context, plan.PrimaryMotif, plan.PrimaryPreferredRegion, false, randomGenerator)) { ++plan.PrimaryOccurrences; }
        }
        for (uint32_t occurrence = 0u; occurrence < plan.SecondaryTargetOccurrences; ++occurrence)
        {
            if (tryGenerateMotifOccurrence(context, plan.SecondaryMotif, plan.SecondaryPreferredRegion, true, randomGenerator)) { ++plan.SecondaryOccurrences; }
        }
    }

    ShipDetailMotifType DetailGenerator::selectDetailMotif(const ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile, std::mt19937_64& randomGenerator, ShipDetailMotifType excluded) const
    {
        uint64_t totalWeight = 0u;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END); ++index)
        {
            const ShipDetailMotifType type = static_cast<ShipDetailMotifType>(index);
            if (type == excluded) { continue; }
            if (context.ScaleTraits.Tier == GenerationScaleTier::TINY && (type == ShipDetailMotifType::TRIPLE_VENT_BANK || type == ShipDetailMotifType::THREE_NODE_LIGHTS || type == ShipDetailMotifType::RECESSED_SLOT)) { continue; }
            totalWeight += profile.MotifWeights.getWeight(type);
        }
        if (totalWeight == 0u) { return ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END; }

        uint64_t roll = std::uniform_int_distribution<uint64_t>(0u, totalWeight - 1u)(randomGenerator);
        for (uint32_t index = 0u; index < static_cast<uint32_t>(ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END); ++index)
        {
            const ShipDetailMotifType type = static_cast<ShipDetailMotifType>(index);
            if (type == excluded) { continue; }
            if (context.ScaleTraits.Tier == GenerationScaleTier::TINY && (type == ShipDetailMotifType::TRIPLE_VENT_BANK || type == ShipDetailMotifType::THREE_NODE_LIGHTS || type == ShipDetailMotifType::RECESSED_SLOT)) { continue; }
            const uint64_t weight = profile.MotifWeights.getWeight(type);
            if (roll < weight) { return type; }
            roll -= weight;
        }
        return ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END;
    }

    GenerationSpatialRegion DetailGenerator::getPreferredMotifRegion(const ShipGenerationContext& context, ShipDetailMotifType type, bool secondary, std::mt19937_64& randomGenerator) const
    {
        if (context.Profile.DetailMotifPlacementBias == ShipDetailMotifPlacementBias::AXIAL)
        {
            if (type == ShipDetailMotifType::PAIRED_VENTS || type == ShipDetailMotifType::TRIPLE_VENT_BANK || type == ShipDetailMotifType::RECESSED_SLOT) { return GenerationSpatialRegion::REAR_FUSELAGE; }
            return secondary ? GenerationSpatialRegion::FRONT_FUSELAGE : GenerationSpatialRegion::MID_FUSELAGE;
        }
        if (context.Profile.DetailMotifPlacementBias == ShipDetailMotifPlacementBias::WING_SURFACE && context.WingRegions.hasWings())
        {
            if (type == ShipDetailMotifType::PAIRED_VENTS || type == ShipDetailMotifType::TRIPLE_VENT_BANK || type == ShipDetailMotifType::RECESSED_SLOT) { return GenerationSpatialRegion::LEFT_WING_ROOT; }
            return randomUInt(randomGenerator, 0u, 99u) < 58u ? GenerationSpatialRegion::LEFT_OUTER_WING : GenerationSpatialRegion::LEFT_WING_ROOT;
        }

        if (context.VisualHierarchy.InfluenceEnabled)
        {
            switch (context.VisualHierarchy.PrimaryAnchor)
            {
            case ShipVisualAnchorType::COCKPIT: return GenerationSpatialRegion::FRONT_FUSELAGE;
            case ShipVisualAnchorType::ENGINES: return GenerationSpatialRegion::REAR_FUSELAGE;
            case ShipVisualAnchorType::WINGS: if (context.WingRegions.hasWings()) { return type == ShipDetailMotifType::PAIRED_LIGHTS || type == ShipDetailMotifType::THREE_NODE_LIGHTS ? GenerationSpatialRegion::LEFT_OUTER_WING : GenerationSpatialRegion::LEFT_WING_ROOT; } break;
            case ShipVisualAnchorType::WEAPONS: return context.WingRegions.hasWings() ? GenerationSpatialRegion::LEFT_WING_ROOT : GenerationSpatialRegion::FRONT_FUSELAGE;
            case ShipVisualAnchorType::CENTRAL_CORE:
            case ShipVisualAnchorType::HULL_LAYERS: return GenerationSpatialRegion::MID_FUSELAGE;
            case ShipVisualAnchorType::NEGATIVE_SPACE: return context.WingRegions.hasWings() ? GenerationSpatialRegion::LEFT_WING_ROOT : GenerationSpatialRegion::REAR_FUSELAGE;
            default: break;
            }
        }

        switch (type)
        {
        case ShipDetailMotifType::PAIRED_VENTS:
        case ShipDetailMotifType::TRIPLE_VENT_BANK:
        case ShipDetailMotifType::RECESSED_SLOT: return GenerationSpatialRegion::REAR_FUSELAGE;
        case ShipDetailMotifType::PAIRED_LIGHTS:
        case ShipDetailMotifType::THREE_NODE_LIGHTS: return context.WingRegions.hasWings() ? GenerationSpatialRegion::LEFT_OUTER_WING : GenerationSpatialRegion::FRONT_FUSELAGE;
        case ShipDetailMotifType::PARALLEL_SEAMS:
        case ShipDetailMotifType::REPEATED_DASHES: return context.WingRegions.hasWings() && randomUInt(randomGenerator, 0u, 99u) < 42u ? GenerationSpatialRegion::LEFT_WING_ROOT : GenerationSpatialRegion::MID_FUSELAGE;
        default: return GenerationSpatialRegion::MID_FUSELAGE;
        }
    }

    bool DetailGenerator::tryGenerateMotifOccurrence(ShipGenerationContext& context, ShipDetailMotifType type, GenerationSpatialRegion preferredRegion, bool secondary, std::mt19937_64& randomGenerator) const
    {
        if (type == ShipDetailMotifType::SHIP_DETAIL_MOTIF_TYPE_END) { return false; }
        const uint32_t cost = getMotifComplexityCost(type);
        if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::DETAIL, cost)) { return false; }

        for (uint32_t attempt = 0u; attempt < MaximumDetailPlacementAttempts; ++attempt)
        {
            uint32_t x = 0u;
            uint32_t y = 0u;
            if (!trySelectMotifAnchor(context, type, preferredRegion, randomGenerator, x, y)) { ++context.DetailMotifs.RejectedPlacements; continue; }

            const bool mirrored = randomUInt(randomGenerator, 0u, 99u) < context.DetailMotifs.MirroredChance;
            if (mirrored) { x = std::min(x, context.Ship.HullMask.getWidth() - 1u - x); }
            std::vector<std::pair<uint32_t, uint32_t>> pixels;
            if (!buildMotifPixels(context, type, x, y, mirrored, randomGenerator, pixels) || pixels.empty()) { ++context.DetailMotifs.RejectedPlacements; continue; }
            if (!areSurfaceDetailPixelsAvailable(context, pixels) || !areMotifPixelsMaterialCoherent(context, pixels)) { ++context.DetailMotifs.RejectedPlacements; continue; }

            const auto regions = context.SpatialBudget.collectRegions(pixels);
            if (!acceptSpatialMotif(context, regions, cost, randomGenerator)) { ++context.DetailMotifs.RejectedPlacements; continue; }

            PixelMask* target = nullptr;
            if (type == ShipDetailMotifType::PAIRED_LIGHTS || type == ShipDetailMotifType::THREE_NODE_LIGHTS) { target = &context.Ship.LightMask; }
            else if (type == ShipDetailMotifType::REPEATED_DASHES) { target = &context.Ship.AccentMask; }
            else { target = &context.Ship.MechanicalDetailMask; }
            commitSurfaceDetailPixels(*target, pixels);
            commitSurfaceDetailPixels(secondary ? context.DetailMotifs.SecondaryMask : context.DetailMotifs.PrimaryMask, pixels);
            context.ComplexityBudget.tryConsume(GenerationComplexityCategory::DETAIL, cost);
            return true;
        }
        return false;
    }

    bool DetailGenerator::trySelectMotifAnchor(ShipGenerationContext& context, ShipDetailMotifType type, GenerationSpatialRegion preferredRegion, std::mt19937_64& randomGenerator, uint32_t& x, uint32_t& y) const
    {
        std::vector<std::pair<uint32_t, uint32_t>> preferred;
        std::vector<std::pair<uint32_t, uint32_t>> fallback;
        const uint32_t width = context.Ship.HullMask.getWidth();
        const uint32_t height = context.Ship.HullMask.getHeight();
        for (uint32_t py = 0u; py < height; ++py)
        {
            for (uint32_t px = 0u; px < width; ++px)
            {
                if (!isSurfaceDetailPixelAvailable(context, px, py, true)) { continue; }
                const GenerationSpatialRegion region = context.SpatialBudget.getRegionAt(px, py);
                if (preferredRegion != GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END && region != preferredRegion && region != context.SpatialBudget.getMirroredRegion(preferredRegion)) { continue; }
                fallback.emplace_back(px, py);
                if (isPreferredMotifMaterial(context, type, px, py)) { preferred.emplace_back(px, py); }
            }
        }
        const auto& candidates = preferred.empty() ? fallback : preferred;
        if (candidates.empty()) { return false; }
        const auto& selected = candidates[randomUInt(randomGenerator, 0u, static_cast<uint32_t>(candidates.size() - 1u))];
        x = selected.first;
        y = selected.second;
        return true;
    }

    bool DetailGenerator::buildMotifPixels(ShipGenerationContext& context, ShipDetailMotifType type, uint32_t x, uint32_t y, bool mirrored, std::mt19937_64& randomGenerator, std::vector<std::pair<uint32_t, uint32_t>>& pixels) const
    {
        bool horizontal = randomUInt(randomGenerator, 0u, 99u) < 68u;
        if (context.Profile.DetailMotifOrientationBias == ShipDetailMotifOrientationBias::LONGITUDINAL &&
            (type == ShipDetailMotifType::PARALLEL_SEAMS || type == ShipDetailMotifType::REPEATED_DASHES || type == ShipDetailMotifType::PAIRED_LIGHTS || type == ShipDetailMotifType::THREE_NODE_LIGHTS))
        {
            horizontal = false;
        }
        if (context.Profile.DetailMotifOrientationBias == ShipDetailMotifOrientationBias::LATERAL) { horizontal = true; }

        const auto add = [&](int32_t px, int32_t py) { return addSurfaceDetailCandidatePixel(context, pixels, px, py, mirrored); };
        if (type == ShipDetailMotifType::PAIRED_VENTS || type == ShipDetailMotifType::TRIPLE_VENT_BANK || type == ShipDetailMotifType::PARALLEL_SEAMS)
        {
            const uint32_t lineCount = type == ShipDetailMotifType::TRIPLE_VENT_BANK ? 3u : 2u;
            const uint32_t dimension = horizontal ? context.Ship.HullMask.getWidth() : context.Ship.HullMask.getHeight();
            const uint32_t minimumLength = type == ShipDetailMotifType::PARALLEL_SEAMS ? 3u : 2u;
            const uint32_t maximumLength = std::max(minimumLength, GenerationMath::scalePixelsFrom64(type == ShipDetailMotifType::PARALLEL_SEAMS ? 6u : 4u, dimension));
            const uint32_t length = randomUInt(randomGenerator, minimumLength, maximumLength);
            const uint32_t spacing = randomUInt(randomGenerator, 2u, context.ScaleTraits.Tier == GenerationScaleTier::LARGE ? 3u : 2u);
            for (uint32_t line = 0u; line < lineCount; ++line)
            {
                for (uint32_t offset = 0u; offset < length; ++offset)
                {
                    const int32_t px = static_cast<int32_t>(x + (horizontal ? offset : line * spacing));
                    const int32_t py = static_cast<int32_t>(y + (horizontal ? line * spacing : offset));
                    if (!add(px, py)) { return false; }
                }
            }
            return true;
        }
        if (type == ShipDetailMotifType::PAIRED_LIGHTS || type == ShipDetailMotifType::THREE_NODE_LIGHTS)
        {
            const uint32_t count = type == ShipDetailMotifType::THREE_NODE_LIGHTS ? 3u : 2u;
            const uint32_t spacing = randomUInt(randomGenerator, 2u, 3u);
            for (uint32_t node = 0u; node < count; ++node)
            {
                if (!add(static_cast<int32_t>(x + (horizontal ? node * spacing : 0u)), static_cast<int32_t>(y + (horizontal ? 0u : node * spacing)))) { return false; }
            }
            return true;
        }
        if (type == ShipDetailMotifType::REPEATED_DASHES)
        {
            const uint32_t count = context.ScaleTraits.Tier == GenerationScaleTier::LARGE ? randomUInt(randomGenerator, 3u, 4u) : 3u;
            const uint32_t dashLength = context.ScaleTraits.Tier == GenerationScaleTier::TINY ? 1u : randomUInt(randomGenerator, 1u, 2u);
            const uint32_t spacing = dashLength + randomUInt(randomGenerator, 1u, 2u);
            for (uint32_t dash = 0u; dash < count; ++dash)
            {
                for (uint32_t offset = 0u; offset < dashLength; ++offset)
                {
                    if (!add(static_cast<int32_t>(x + (horizontal ? dash * spacing + offset : 0u)), static_cast<int32_t>(y + (horizontal ? 0u : dash * spacing + offset)))) { return false; }
                }
            }
            return true;
        }
        if (type == ShipDetailMotifType::RECESSED_SLOT)
        {
            uint32_t slotWidth = std::max(3u, GenerationMath::scalePixelsFrom64(4u, context.Ship.HullMask.getWidth()));
            uint32_t slotHeight = std::max(2u, GenerationMath::scalePixelsFrom64(2u, context.Ship.HullMask.getHeight()));
            if (context.Profile.DetailMotifOrientationBias == ShipDetailMotifOrientationBias::LONGITUDINAL) { std::swap(slotWidth, slotHeight); }
            for (uint32_t oy = 0u; oy < slotHeight; ++oy)
            {
                for (uint32_t ox = 0u; ox < slotWidth; ++ox)
                {
                    if (ox != 0u && oy != 0u && ox + 1u != slotWidth && oy + 1u != slotHeight) { continue; }
                    if (!add(static_cast<int32_t>(x + ox), static_cast<int32_t>(y + oy))) { return false; }
                }
            }
            return true;
        }
        return false;
    }

    bool DetailGenerator::areMotifPixelsMaterialCoherent(const ShipGenerationContext& context, const std::vector<std::pair<uint32_t, uint32_t>>& pixels) const
    {
        if (pixels.empty()) { return false; }
        const auto materialClass = [&](uint32_t x, uint32_t y)
            {
                if (context.MaterialComposition.MechanicalMask.get(x, y)) { return 2u; }
                if (context.MaterialComposition.SecondaryHullMask.get(x, y)) { return 1u; }
                return 0u;
            };
        const uint32_t expected = materialClass(pixels.front().first, pixels.front().second);
        for (const auto& pixel : pixels)
        {
            if (materialClass(pixel.first, pixel.second) != expected) { return false; }
        }
        return true;
    }

    bool DetailGenerator::isPreferredMotifMaterial(const ShipGenerationContext& context, ShipDetailMotifType type, uint32_t x, uint32_t y) const
    {
        const bool mechanical = context.MaterialComposition.MechanicalMask.get(x, y) || context.CoreTreatment.RecessedMask.get(x, y);
        const bool secondary = context.MaterialComposition.SecondaryHullMask.get(x, y);
        switch (type)
        {
        case ShipDetailMotifType::PAIRED_VENTS:
        case ShipDetailMotifType::TRIPLE_VENT_BANK:
        case ShipDetailMotifType::RECESSED_SLOT: return mechanical;
        case ShipDetailMotifType::PAIRED_LIGHTS:
        case ShipDetailMotifType::THREE_NODE_LIGHTS: return secondary || context.CoreTreatment.BoundaryMask.get(x, y);
        case ShipDetailMotifType::PARALLEL_SEAMS: return secondary;
        case ShipDetailMotifType::REPEATED_DASHES: return !mechanical;
        default: return true;
        }
    }

    uint32_t DetailGenerator::getMotifComplexityCost(ShipDetailMotifType type) const
    {
        return type == ShipDetailMotifType::PAIRED_LIGHTS || type == ShipDetailMotifType::THREE_NODE_LIGHTS ? 1u : 2u;
    }

    uint32_t DetailGenerator::getFreeformDetailPercent(const ShipGenerationContext& context) const
    {
        if (!context.DetailMotifs.hasPrimary()) { return 100u; }
        uint32_t percent = 65u;
        switch (context.ScaleTraits.Tier)
        {
        case GenerationScaleTier::TINY: percent = 76u; break;
        case GenerationScaleTier::SMALL: percent = 68u; break;
        case GenerationScaleTier::MEDIUM: percent = 62u; break;
        case GenerationScaleTier::LARGE: percent = 58u; break;
        default: break;
        }
        if (context.DetailMotifs.hasSecondary()) { percent = scalePercent(percent, 88u); }
        if (!context.Livery.empty()) { percent = scalePercent(percent, 86u); }
        return std::max(40u, percent);
    }

    void DetailGenerator::generateAccentDetails(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile) const
    {
        GeneratedShip& ship = context.Ship;
        const ShipGenerationConfiguration& settings = context.Settings;
        const uint32_t width = ship.HullMask.getWidth();
        const uint32_t height = ship.HullMask.getHeight();

        if (width == 0u || height == 0u)
        {
            return;
        }

        const uint32_t minimumDimension = std::min(width, height);
        const uint32_t baseEffectiveDensity = std::min(100u, composeSurfaceDetailPercent(settings.DetailDensity, profile.DetailDensityPercent));
        const uint32_t effectiveDensity = composeSurfaceDetailPercent(baseEffectiveDensity, 50u + context.ScaleTraits.DetailComplexity / 2u);
        const uint32_t basePatternCount = GenerationMath::scalePixelsFrom64(6u, minimumDimension);
        const uint32_t targetPatternCount = composeSurfaceDetailPercent(basePatternCount, effectiveDensity);
        const uint64_t totalWeight = static_cast<uint64_t>(profile.AccentPanelWeight) + profile.AccentStripeWeight + profile.AccentArmorWeight;

        if (targetPatternCount == 0u || totalWeight == 0u)
        {
            return;
        }

        for (uint32_t patternIndex = 0u; patternIndex < targetPatternCount; ++patternIndex)
        {
            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::DETAIL, AccentDetailComplexityCost))
            {
                break;
            }

            for (uint32_t attempt = 0u; attempt < MaximumDetailPlacementAttempts; ++attempt)
            {
                const bool mirrored = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) >= profile.AsymmetricDetailChance;
                uint32_t x = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, width - 1u);
                uint32_t y = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, height - 1u);

                if (context.WingRegions.hasWings() && context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < 30u)
                {
                    trySelectDetailAnchorFromMask(context, context.WingRegions.WingMask, x, y);
                }

                if (mirrored)
                {
                    x = std::min(x, width - 1u - x);
                }

                uint64_t roll = context.getGenerationRandomUInt64(GenerationDomain::DETAILS, 0u, totalWeight - 1u);

                if (roll < profile.AccentPanelWeight)
                {
                    const uint32_t panelWidth = context.getGenerationRandomUInt(GenerationDomain::DETAILS, GenerationMath::scalePixelsFrom64(2u, width), GenerationMath::scalePixelsFrom64(4u, width));
                    const uint32_t panelHeight = context.getGenerationRandomUInt(GenerationDomain::DETAILS, GenerationMath::scalePixelsFrom64(2u, height), GenerationMath::scalePixelsFrom64(3u, height));
                    const bool generated = mirrored ? tryAddSymmetricDetailRectangle(context, ship.AccentMask, x, y, panelWidth, panelHeight) : tryAddDetailRectangle(context, ship.AccentMask, x, y, panelWidth, panelHeight);

                    if (generated)
                    {
                        context.ComplexityBudget.tryConsume(GenerationComplexityCategory::DETAIL, AccentDetailComplexityCost);
                        if (context.DebugInfo != nullptr)
                        {
                            ++context.DebugInfo->AccentPatternCount;
                        }

                        break;
                    }

                    continue;
                }

                roll -= profile.AccentPanelWeight;

                if (roll < profile.AccentStripeWeight)
                {
                    const bool horizontal = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < 70u;
                    const uint32_t length = horizontal ? context.getGenerationRandomUInt(GenerationDomain::DETAILS, GenerationMath::scalePixelsFrom64(3u, width), GenerationMath::scalePixelsFrom64(7u, width)) : context.getGenerationRandomUInt(GenerationDomain::DETAILS, GenerationMath::scalePixelsFrom64(3u, height), GenerationMath::scalePixelsFrom64(7u, height));
                    const uint32_t stripeWidth = horizontal ? length : 1u;
                    const uint32_t stripeHeight = horizontal ? 1u : length;
                    const bool generated = mirrored ? tryAddSymmetricDetailRectangle(context, ship.AccentMask, x, y, stripeWidth, stripeHeight) : tryAddDetailRectangle(context, ship.AccentMask, x, y, stripeWidth, stripeHeight);

                    if (generated)
                    {
                        context.ComplexityBudget.tryConsume(GenerationComplexityCategory::DETAIL, AccentDetailComplexityCost);
                        if (context.DebugInfo != nullptr)
                        {
                            ++context.DebugInfo->AccentPatternCount;
                        }

                        break;
                    }

                    continue;
                }

                const uint32_t armorWidth = context.getGenerationRandomUInt(GenerationDomain::DETAILS, GenerationMath::scalePixelsFrom64(3u, width), GenerationMath::scalePixelsFrom64(5u, width));
                const uint32_t armorHeight = context.getGenerationRandomUInt(GenerationDomain::DETAILS, GenerationMath::scalePixelsFrom64(2u, height), GenerationMath::scalePixelsFrom64(4u, height));
                const uint32_t variant = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 2u);
                std::vector<std::pair<uint32_t, uint32_t>> candidatePixels;
                bool candidateCreated = true;

                for (uint32_t offsetY = 0u; offsetY < armorHeight && candidateCreated; ++offsetY)
                {
                    for (uint32_t offsetX = 0u; offsetX < armorWidth; ++offsetX)
                    {
                        bool includePixel = true;

                        if (variant == 0u && offsetY == 0u && offsetX == armorWidth - 1u)
                        {
                            includePixel = false;
                        }
                        else if (variant == 1u && offsetY == armorHeight - 1u && offsetX == armorWidth - 1u)
                        {
                            includePixel = false;
                        }
                        else if (variant == 2u && offsetY == 0u && (offsetX == 0u || offsetX == armorWidth - 1u))
                        {
                            includePixel = false;
                        }

                        if (!includePixel)
                        {
                            continue;
                        }

                        if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + offsetX), static_cast<int32_t>(y + offsetY), mirrored))
                        {
                            candidateCreated = false;
                            break;
                        }
                    }
                }

                if (!candidateCreated || candidatePixels.empty())
                {
                    continue;
                }

                if (!areSurfaceDetailPixelsAvailable(context, candidatePixels))
                {
                    continue;
                }

                const auto spatialRegions = context.SpatialBudget.collectRegions(candidatePixels);
                if (!acceptSpatialDetail(context, spatialRegions, AccentDetailComplexityCost)) { continue; }
                commitSurfaceDetailPixels(ship.AccentMask, candidatePixels);
                context.ComplexityBudget.tryConsume(GenerationComplexityCategory::DETAIL, AccentDetailComplexityCost);

                if (context.DebugInfo != nullptr)
                {
                    ++context.DebugInfo->AccentPatternCount;
                }

                break;
            }
        }
    }

    void DetailGenerator::generateMechanicalDetails(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile) const
    {
        GeneratedShip& ship = context.Ship;
        const ShipGenerationConfiguration& settings = context.Settings;
        const uint32_t width = ship.HullMask.getWidth();
        const uint32_t height = ship.HullMask.getHeight();

        if (width == 0u || height == 0u)
        {
            return;
        }

        const uint32_t minimumDimension = std::min(width, height);
        const uint32_t baseEffectiveDensity = std::min(100u, composeSurfaceDetailPercent(settings.DetailDensity, profile.DetailDensityPercent));
        const uint32_t effectiveDensity = composeSurfaceDetailPercent(baseEffectiveDensity, 50u + context.ScaleTraits.DetailComplexity / 2u);
        const uint32_t basePatternCount = GenerationMath::scalePixelsFrom64(4u, minimumDimension);
        uint32_t targetPatternCount = composeSurfaceDetailPercent(basePatternCount, effectiveDensity);
        targetPatternCount = composeSurfaceDetailPercent(targetPatternCount, profile.MechanicalPatternCountPercent);

        if (targetPatternCount == 0u)
        {
            return;
        }

        for (uint32_t patternIndex = 0u; patternIndex < targetPatternCount; ++patternIndex)
        {
            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::DETAIL, MechanicalDetailComplexityCost))
            {
                break;
            }

            for (uint32_t attempt = 0u; attempt < MaximumDetailPlacementAttempts; ++attempt)
            {
                const bool mirrored = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) >= profile.AsymmetricDetailChance;
                const bool horizontal = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < profile.HorizontalVentChance;
                uint32_t x = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, width - 1u);
                uint32_t y = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, height - 1u);

                if (PixelMaskUtils::getMaskPixelCount(context.CoreTreatment.RecessedMask) > 0u && context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < 38u)
                {
                    trySelectDetailAnchorFromMask(context, context.CoreTreatment.RecessedMask, x, y);
                }
                else if (context.WingRegions.hasWings() && context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < 40u)
                {
                    trySelectDetailAnchorFromMask(context, context.WingRegions.WingRootMask, x, y);
                }

                const uint32_t slitCount = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 2u, 3u);
                const uint32_t slitLength = horizontal ? context.getGenerationRandomUInt(GenerationDomain::DETAILS, 2u, GenerationMath::scalePixelsFrom64(4u, width)) : context.getGenerationRandomUInt(GenerationDomain::DETAILS, 2u, GenerationMath::scalePixelsFrom64(4u, height));

                if (mirrored)
                {
                    x = std::min(x, width - 1u - x);
                }

                std::vector<std::pair<uint32_t, uint32_t>> candidatePixels;
                bool generated = true;

                for (uint32_t slit = 0u; slit < slitCount && generated; ++slit)
                {
                    for (uint32_t offset = 0u; offset < slitLength; ++offset)
                    {
                        const int32_t pixelX = horizontal ? static_cast<int32_t>(x + offset) : static_cast<int32_t>(x + slit * 2u);
                        const int32_t pixelY = horizontal ? static_cast<int32_t>(y + slit * 2u) : static_cast<int32_t>(y + offset);

                        if (!addSurfaceDetailCandidatePixel(context, candidatePixels, pixelX, pixelY, mirrored))
                        {
                            generated = false;
                            break;
                        }
                    }
                }

                if (!generated || candidatePixels.empty())
                {
                    continue;
                }

                if (!areSurfaceDetailPixelsAvailable(context, candidatePixels))
                {
                    continue;
                }

                const auto spatialRegions = context.SpatialBudget.collectRegions(candidatePixels);
                if (!acceptSpatialDetail(context, spatialRegions, MechanicalDetailComplexityCost)) { continue; }
                commitSurfaceDetailPixels(ship.MechanicalDetailMask, candidatePixels);
                context.ComplexityBudget.tryConsume(GenerationComplexityCategory::DETAIL, MechanicalDetailComplexityCost);

                if (context.DebugInfo != nullptr)
                {
                    ++context.DebugInfo->MechanicalPatternCount;
                }

                break;
            }
        }
    }

    void DetailGenerator::generateLightDetails(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile) const
    {
        GeneratedShip& ship = context.Ship;
        const ShipGenerationConfiguration& settings = context.Settings;
        const uint32_t width = ship.HullMask.getWidth();
        const uint32_t height = ship.HullMask.getHeight();

        if (width == 0u || height == 0u)
        {
            return;
        }

        const uint32_t minimumDimension = std::min(width, height);
        const uint32_t baseEffectiveDensity = std::min(100u, composeSurfaceDetailPercent(settings.DetailDensity, profile.DetailDensityPercent));
        const uint32_t effectiveDensity = composeSurfaceDetailPercent(baseEffectiveDensity, 50u + context.ScaleTraits.DetailComplexity / 2u);
        const uint32_t basePatternCount = GenerationMath::scalePixelsFrom64(3u, minimumDimension);
        uint32_t targetPatternCount = composeSurfaceDetailPercent(basePatternCount, effectiveDensity);
        targetPatternCount = composeSurfaceDetailPercent(targetPatternCount, profile.LightPatternCountPercent);

        for (uint32_t patternIndex = 0u; patternIndex < targetPatternCount; ++patternIndex)
        {
            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::DETAIL, LightDetailComplexityCost))
            {
                break;
            }

            for (uint32_t attempt = 0u; attempt < MaximumDetailPlacementAttempts; ++attempt)
            {
                const bool mirrored = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) >= profile.AsymmetricDetailChance;
                uint32_t x = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, width - 1u);
                uint32_t y = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, height - 1u);

                if (PixelMaskUtils::getMaskPixelCount(context.CoreTreatment.BoundaryMask) > 0u && context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < 32u)
                {
                    trySelectDetailAnchorFromMask(context, context.CoreTreatment.BoundaryMask, x, y);
                }
                else if (context.WingRegions.hasWings() && context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < 55u)
                {
                    trySelectDetailAnchorFromMask(context, context.WingRegions.OuterWingMask, x, y);
                }

                const uint32_t lightHeight = context.ScaleTraits.DetailComplexity >= 80u && context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < 15u ? 2u : 1u;

                if (mirrored)
                {
                    x = std::min(x, width - 1u - x);
                }

                const bool generated = mirrored ? tryAddSymmetricDetailRectangle(context, ship.LightMask, x, y, 1u, lightHeight, LightDetailComplexityCost, true)
                    : tryAddDetailRectangle(context, ship.LightMask, x, y, 1u, lightHeight, LightDetailComplexityCost, true);

                if (generated)
                {
                    context.ComplexityBudget.tryConsume(GenerationComplexityCategory::DETAIL, LightDetailComplexityCost);
                    if (context.DebugInfo != nullptr)
                    {
                        ++context.DebugInfo->LightPatternCount;
                    }

                    break;
                }
            }
        }
    }

    void DetailGenerator::generateSupplementalSurfaceDetails(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile) const
    {
        GeneratedShip& ship = context.Ship;
        const ShipGenerationConfiguration& settings = context.Settings;
        const uint32_t width = ship.HullMask.getWidth();
        const uint32_t height = ship.HullMask.getHeight();

        if (width == 0u || height == 0u)
        {
            return;
        }

        const uint32_t minimumDimension = std::min(width, height);
        const uint32_t baseEffectiveDensity = std::min(100u, composeSurfaceDetailPercent(settings.DetailDensity, profile.DetailDensityPercent));
        const uint32_t effectiveDensity = composeSurfaceDetailPercent(baseEffectiveDensity, 50u + context.ScaleTraits.DetailComplexity / 2u);
        const uint32_t basePatternCount = GenerationMath::scalePixelsFrom64(3u, minimumDimension);
        const uint32_t targetPatternCount = composeSurfaceDetailPercent(basePatternCount, effectiveDensity);

        if (targetPatternCount == 0u)
        {
            return;
        }

        for (uint32_t patternIndex = 0u; patternIndex < targetPatternCount; ++patternIndex)
        {
            if (!context.ComplexityBudget.canAfford(GenerationComplexityCategory::DETAIL, SupplementalDetailComplexityCost))
            {
                break;
            }

            for (uint32_t attempt = 0u; attempt < MaximumDetailPlacementAttempts; ++attempt)
            {
                const SupplementalSurfaceDetailType type = getSupplementalSurfaceDetailType(context, profile);

                if (type == SupplementalSurfaceDetailType::SUPPLEMENTAL_SURFACE_DETAIL_TYPE_END)
                {
                    return;
                }

                const bool mirrored = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) >= profile.AsymmetricDetailChance;
                uint32_t x = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, width - 1u);
                uint32_t y = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, height - 1u);

                if (type == SupplementalSurfaceDetailType::LUMINOUS_CHANNEL
                    && context.FactionProfile.SurfaceDetails.LuminousChannelCoreRegionBiasChance > 0u
                    && PixelMaskUtils::getMaskPixelCount(context.CoreTreatment.CoreRegionMask) > 0u
                    && context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < context.FactionProfile.SurfaceDetails.LuminousChannelCoreRegionBiasChance)
                {
                    trySelectDetailAnchorFromMask(context, context.CoreTreatment.CoreRegionMask, x, y);
                }
                else if (type == SupplementalSurfaceDetailType::IDENTIFICATION_MARKING && context.WingRegions.hasWings()
                    && context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < 45u)
                {
                    trySelectDetailAnchorFromMask(context, context.WingRegions.WingRootMask, x, y);
                }

                if (mirrored)
                {
                    x = std::min(x, width - 1u - x);
                }

                bool generated = false;

                switch (type)
                {
                case SupplementalSurfaceDetailType::PANEL_SEAM: generated = generatePanelSeamDetail(context, x, y, mirrored); break;
                case SupplementalSurfaceDetailType::GEOMETRIC_MARKING: generated = generateGeometricMarkingDetail(context, x, y, mirrored); break;
                case SupplementalSurfaceDetailType::MECHANICAL_EXPOSURE: generated = generateMechanicalExposureDetail(context, x, y, mirrored); break;
                case SupplementalSurfaceDetailType::REPEATING_MOTIF: generated = generateRepeatingMotifDetail(context, x, y, mirrored); break;
                case SupplementalSurfaceDetailType::IDENTIFICATION_MARKING: generated = generateIdentificationMarkingDetail(context, x, y, mirrored); break;
                case SupplementalSurfaceDetailType::LUMINOUS_CHANNEL: generated = generateLuminousChannelDetail(context, x, y, mirrored); break;
                default: break;
                }

                if (generated)
                {
                    context.ComplexityBudget.tryConsume(GenerationComplexityCategory::DETAIL, SupplementalDetailComplexityCost);
                    if (context.DebugInfo != nullptr && type != SupplementalSurfaceDetailType::SUPPLEMENTAL_SURFACE_DETAIL_TYPE_END)
                    {
                        ++context.DebugInfo->SupplementalSurfaceDetailCounts[static_cast<std::size_t>(type)];
                    }

                    break;
                }
            }
        }
    }

    bool DetailGenerator::generatePanelSeamDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const
    {
        GeneratedShip& ship = context.Ship;
        const uint32_t width = ship.HullMask.getWidth();
        const uint32_t height = ship.HullMask.getHeight();
        const bool horizontal = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < 60u;
        const uint32_t length = horizontal ? context.getGenerationRandomUInt(GenerationDomain::DETAILS, GenerationMath::scalePixelsFrom64(3u, width), GenerationMath::scalePixelsFrom64(6u, width))
            : context.getGenerationRandomUInt(GenerationDomain::DETAILS, GenerationMath::scalePixelsFrom64(3u, height), GenerationMath::scalePixelsFrom64(6u, height));
        const uint32_t detailWidth = horizontal ? length : 1u;
        const uint32_t detailHeight = horizontal ? 1u : length;

        if (mirrored)
        {
            return tryAddSymmetricDetailRectangle(context, ship.MechanicalDetailMask, x, y, detailWidth, detailHeight);
        }

        return tryAddDetailRectangle(context, ship.MechanicalDetailMask, x, y, detailWidth, detailHeight);
    }

    bool DetailGenerator::generateGeometricMarkingDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const
    {
        GeneratedShip& ship = context.Ship;
        const bool highResolution = context.ScaleTraits.DetailComplexity >= 80u;
        const uint32_t variant = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 3u);
        std::vector<std::pair<uint32_t, uint32_t>> candidatePixels;

        if (variant == 0u)
        {
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x), static_cast<int32_t>(y), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 1u), static_cast<int32_t>(y + 1u), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 2u), static_cast<int32_t>(y), mirrored)) { return false; }

            if (highResolution && !addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 3u), static_cast<int32_t>(y + 1u), mirrored)) { return false; }
        }
        else if (variant == 1u)
        {
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 1u), static_cast<int32_t>(y), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x), static_cast<int32_t>(y + 1u), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 1u), static_cast<int32_t>(y + 1u), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 2u), static_cast<int32_t>(y + 1u), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 1u), static_cast<int32_t>(y + 2u), mirrored)) { return false; }
        }
        else if (variant == 2u)
        {
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x), static_cast<int32_t>(y), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 1u), static_cast<int32_t>(y + 1u), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 2u), static_cast<int32_t>(y + 2u), mirrored)) { return false; }

            if (highResolution && !addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 3u), static_cast<int32_t>(y + 3u), mirrored)) { return false; }
        }
        else
        {
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x), static_cast<int32_t>(y), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 2u), static_cast<int32_t>(y), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 1u), static_cast<int32_t>(y + 1u), mirrored)) { return false; }

            if (highResolution)
            {
                if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x), static_cast<int32_t>(y + 2u), mirrored)) { return false; }
                if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 2u), static_cast<int32_t>(y + 2u), mirrored)) { return false; }
            }
        }

        if (!areSurfaceDetailPixelsAvailable(context, candidatePixels))
        {
            return false;
        }

        const auto spatialRegions = context.SpatialBudget.collectRegions(candidatePixels);
        if (!acceptSpatialDetail(context, spatialRegions, SupplementalDetailComplexityCost)) { return false; }
        commitSurfaceDetailPixels(ship.AccentMask, candidatePixels);
        return true;
    }

    bool DetailGenerator::generateMechanicalExposureDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const
    {
        GeneratedShip& ship = context.Ship;
        const uint32_t width = ship.HullMask.getWidth();
        const uint32_t height = ship.HullMask.getHeight();
        const uint32_t patchWidth = context.getGenerationRandomUInt(GenerationDomain::DETAILS, GenerationMath::scalePixelsFrom64(2u, width), GenerationMath::scalePixelsFrom64(3u, width));
        const uint32_t patchHeight = context.getGenerationRandomUInt(GenerationDomain::DETAILS, GenerationMath::scalePixelsFrom64(2u, height), GenerationMath::scalePixelsFrom64(3u, height));
        const uint32_t variant = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 2u);
        std::vector<std::pair<uint32_t, uint32_t>> candidatePixels;

        for (uint32_t offsetY = 0u; offsetY < patchHeight; ++offsetY)
        {
            for (uint32_t offsetX = 0u; offsetX < patchWidth; ++offsetX)
            {
                bool includePixel = false;

                if (variant == 0u)
                {
                    includePixel = offsetX == 0u || offsetY == 0u || offsetX == patchWidth - 1u || offsetY == patchHeight - 1u;
                }
                else if (variant == 1u)
                {
                    includePixel = (offsetX + offsetY) % 2u == 0u;
                }
                else
                {
                    includePixel = true;

                    if (offsetX == patchWidth - 1u && offsetY == 0u) { includePixel = false; }
                    if (offsetX == 0u && offsetY == patchHeight - 1u) { includePixel = false; }
                }

                if (!includePixel)
                {
                    continue;
                }

                if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + offsetX), static_cast<int32_t>(y + offsetY), mirrored))
                {
                    return false;
                }
            }
        }

        if (candidatePixels.empty())
        {
            return false;
        }

        if (!areSurfaceDetailPixelsAvailable(context, candidatePixels))
        {
            return false;
        }

        const auto spatialRegions = context.SpatialBudget.collectRegions(candidatePixels);
        if (!acceptSpatialDetail(context, spatialRegions, SupplementalDetailComplexityCost)) { return false; }
        commitSurfaceDetailPixels(ship.MechanicalDetailMask, candidatePixels);
        return true;
    }

    bool DetailGenerator::generateRepeatingMotifDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const
    {
        GeneratedShip& ship = context.Ship;
        const bool horizontal = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < 70u;
        const uint32_t variant = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 2u);
        const uint32_t repeatCount = context.ScaleTraits.DetailComplexity >= 80u ? context.getGenerationRandomUInt(GenerationDomain::DETAILS, 2u, 4u) : context.getGenerationRandomUInt(GenerationDomain::DETAILS, 2u, 3u);
        const uint32_t spacing = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 2u, 3u);
        std::vector<std::pair<uint32_t, uint32_t>> candidatePixels;

        for (uint32_t repeat = 0u; repeat < repeatCount; ++repeat)
        {
            const uint32_t repeatOffset = repeat * spacing;
            const int32_t originX = static_cast<int32_t>(x + (horizontal ? repeatOffset : 0u));
            const int32_t originY = static_cast<int32_t>(y + (horizontal ? 0u : repeatOffset));

            if (variant == 0u)
            {
                if (!addSurfaceDetailCandidatePixel(context, candidatePixels, originX, originY, mirrored)) { return false; }
                if (!addSurfaceDetailCandidatePixel(context, candidatePixels, originX + (horizontal ? 0 : 1), originY + (horizontal ? 1 : 0), mirrored)) { return false; }
            }
            else if (variant == 1u)
            {
                if (!addSurfaceDetailCandidatePixel(context, candidatePixels, originX, originY, mirrored)) { return false; }
                if (!addSurfaceDetailCandidatePixel(context, candidatePixels, originX + (horizontal ? 1 : 0), originY + (horizontal ? 0 : 1), mirrored)) { return false; }
            }
            else
            {
                if (!addSurfaceDetailCandidatePixel(context, candidatePixels, originX, originY, mirrored)) { return false; }

                if (horizontal)
                {
                    if (!addSurfaceDetailCandidatePixel(context, candidatePixels, originX + 1, originY + 1, mirrored)) { return false; }
                    if (!addSurfaceDetailCandidatePixel(context, candidatePixels, originX + 2, originY, mirrored)) { return false; }
                }
                else
                {
                    if (!addSurfaceDetailCandidatePixel(context, candidatePixels, originX + 1, originY + 1, mirrored)) { return false; }
                    if (!addSurfaceDetailCandidatePixel(context, candidatePixels, originX, originY + 2, mirrored)) { return false; }
                }
            }
        }

        if (!areSurfaceDetailPixelsAvailable(context, candidatePixels))
        {
            return false;
        }

        const auto spatialRegions = context.SpatialBudget.collectRegions(candidatePixels);
        if (!acceptSpatialDetail(context, spatialRegions, SupplementalDetailComplexityCost)) { return false; }
        commitSurfaceDetailPixels(ship.AccentMask, candidatePixels);
        return true;
    }

    bool DetailGenerator::generateIdentificationMarkingDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const
    {
        GeneratedShip& ship = context.Ship;
        const bool detailed = context.ScaleTraits.DetailComplexity >= 55u;
        const uint32_t variant = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 2u);
        std::vector<std::pair<uint32_t, uint32_t>> candidatePixels;

        // Small hard-edged motifs read as serial/brand marks rather than damage.
        if (variant == 0u)
        {
            for (uint32_t offset = 0u; offset < (detailed ? 4u : 3u); ++offset)
            {
                if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + offset), static_cast<int32_t>(y), mirrored)) { return false; }
            }
            if (detailed && !addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 1u), static_cast<int32_t>(y + 1u), mirrored)) { return false; }
        }
        else if (variant == 1u)
        {
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x), static_cast<int32_t>(y), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 1u), static_cast<int32_t>(y), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x), static_cast<int32_t>(y + 1u), mirrored)) { return false; }
            if (detailed && !addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 2u), static_cast<int32_t>(y + 1u), mirrored)) { return false; }
        }
        else
        {
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x), static_cast<int32_t>(y), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 1u), static_cast<int32_t>(y + 1u), mirrored)) { return false; }
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 2u), static_cast<int32_t>(y), mirrored)) { return false; }
            if (detailed && !addSurfaceDetailCandidatePixel(context, candidatePixels, static_cast<int32_t>(x + 3u), static_cast<int32_t>(y), mirrored)) { return false; }
        }

        if (!areSurfaceDetailPixelsAvailable(context, candidatePixels)) { return false; }
        const auto spatialRegions = context.SpatialBudget.collectRegions(candidatePixels);
        if (!acceptSpatialDetail(context, spatialRegions, SupplementalDetailComplexityCost)) { return false; }
        commitSurfaceDetailPixels(ship.AccentMask, candidatePixels);
        return true;
    }

    bool DetailGenerator::generateLuminousChannelDetail(ShipGenerationContext& context, uint32_t x, uint32_t y, bool mirrored) const
    {
        GeneratedShip& ship = context.Ship;
        const bool vertical = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, 99u) < 72u;
        const uint32_t dimension = vertical ? ship.HullMask.getHeight() : ship.HullMask.getWidth();
        const uint32_t minimumLength = std::max(2u, GenerationMath::scalePixelsFrom64(3u, dimension));
        const uint32_t maximumLength = std::max(minimumLength, GenerationMath::scalePixelsFrom64(context.ScaleTraits.DetailComplexity >= 70u ? 7u : 5u, dimension));
        const uint32_t length = context.getGenerationRandomUInt(GenerationDomain::DETAILS, minimumLength, maximumLength);
        std::vector<std::pair<uint32_t, uint32_t>> candidatePixels;

        for (uint32_t offset = 0u; offset < length; ++offset)
        {
            const int32_t pixelX = static_cast<int32_t>(x + (vertical ? 0u : offset));
            const int32_t pixelY = static_cast<int32_t>(y + (vertical ? offset : 0u));
            if (!addSurfaceDetailCandidatePixel(context, candidatePixels, pixelX, pixelY, mirrored)) { return false; }
        }

        if (!areSurfaceDetailPixelsAvailable(context, candidatePixels)) { return false; }
        const auto spatialRegions = context.SpatialBudget.collectRegions(candidatePixels);
        if (!acceptSpatialDetail(context, spatialRegions, SupplementalDetailComplexityCost)) { return false; }
        commitSurfaceDetailPixels(ship.LightMask, candidatePixels);
        return true;
    }

    SupplementalSurfaceDetailType DetailGenerator::getSupplementalSurfaceDetailType(ShipGenerationContext& context, const ResolvedSurfaceDetailProfile& profile) const
    {
        uint64_t totalWeight = 0u;

        for (uint32_t index = 0u; index < static_cast<uint32_t>(SupplementalSurfaceDetailType::SUPPLEMENTAL_SURFACE_DETAIL_TYPE_END); ++index)
        {
            const SupplementalSurfaceDetailType type = static_cast<SupplementalSurfaceDetailType>(index);
            totalWeight += profile.SupplementalWeights.getWeight(type);
        }

        if (totalWeight == 0u)
        {
            return SupplementalSurfaceDetailType::SUPPLEMENTAL_SURFACE_DETAIL_TYPE_END;
        }

        uint64_t roll = context.getGenerationRandomUInt64(GenerationDomain::DETAILS, 0u, totalWeight - 1u);

        for (uint32_t index = 0u; index < static_cast<uint32_t>(SupplementalSurfaceDetailType::SUPPLEMENTAL_SURFACE_DETAIL_TYPE_END); ++index)
        {
            const SupplementalSurfaceDetailType type = static_cast<SupplementalSurfaceDetailType>(index);
            const uint64_t weight = profile.SupplementalWeights.getWeight(type);

            if (roll < weight)
            {
                return type;
            }

            roll -= weight;
        }

        return SupplementalSurfaceDetailType::SUPPLEMENTAL_SURFACE_DETAIL_TYPE_END;
    }

    bool DetailGenerator::tryAddSymmetricDetailRectangle(ShipGenerationContext& context, PixelMask& targetMask, uint32_t startX, uint32_t startY, uint32_t width, uint32_t height, uint32_t spatialCost, bool allowHullLayerBoundary) const
    {
        GeneratedShip& ship = context.Ship;
        if (width == 0u)
        {
            return false;
        }

        if (startX + width > ship.HullMask.getWidth())
        {
            return false;
        }

        const uint32_t mirroredStartX = ship.HullMask.getWidth() - startX - width;

        if (!isDetailRectangleAvailable(context, startX, startY, width, height, allowHullLayerBoundary))
        {
            return false;
        }

        if (!isDetailRectangleAvailable(context, mirroredStartX, startY, width, height, allowHullLayerBoundary))
        {
            return false;
        }

        const auto spatialRegions = collectRectangleRegions(context, startX, startY, width, height, true);
        if (!acceptSpatialDetail(context, spatialRegions, spatialCost)) { return false; }

        PixelMaskUtils::addMaskRectangle(targetMask, startX, startY, width, height);
        PixelMaskUtils::addMaskRectangle(targetMask, mirroredStartX, startY, width, height);

        return true;
    }

    bool DetailGenerator::tryAddDetailRectangle(ShipGenerationContext& context, PixelMask& targetMask, uint32_t startX, uint32_t startY, uint32_t width, uint32_t height, uint32_t spatialCost, bool allowHullLayerBoundary) const
    {
        if (!isDetailRectangleAvailable(context, startX, startY, width, height, allowHullLayerBoundary))
        {
            return false;
        }

        const auto spatialRegions = collectRectangleRegions(context, startX, startY, width, height, false);
        if (!acceptSpatialDetail(context, spatialRegions, spatialCost)) { return false; }

        PixelMaskUtils::addMaskRectangle(targetMask, startX, startY, width, height);

        return true;
    }

    bool DetailGenerator::isDetailRectangleAvailable(const ShipGenerationContext& context, uint32_t startX, uint32_t startY, uint32_t width, uint32_t height, bool allowHullLayerBoundary) const
    {
        const GeneratedShip& ship = context.Ship;
        if (width == 0u || height == 0u)
        {
            return false;
        }

        if (startX >= ship.HullMask.getWidth() || startY >= ship.HullMask.getHeight())
        {
            return false;
        }

        if (width > ship.HullMask.getWidth() - startX || height > ship.HullMask.getHeight() - startY)
        {
            return false;
        }

        const uint32_t materialClass = context.MaterialComposition.MechanicalMask.get(startX, startY) ? 2u : (context.MaterialComposition.SecondaryHullMask.get(startX, startY) ? 1u : 0u);
        for (uint32_t y = startY; y < startY + height; ++y)
        {
            for (uint32_t x = startX; x < startX + width; ++x)
            {
                if (!isSurfaceDetailPixelAvailable(context, x, y, allowHullLayerBoundary))
                {
                    return false;
                }
                const uint32_t pixelMaterialClass = context.MaterialComposition.MechanicalMask.get(x, y) ? 2u : (context.MaterialComposition.SecondaryHullMask.get(x, y) ? 1u : 0u);
                if (pixelMaterialClass != materialClass) { return false; }
            }
        }

        return true;
    }

    ResolvedSurfaceDetailProfile DetailGenerator::resolveSurfaceDetailProfile(const ShipGenerationConfiguration& settings, const ShipGenerationProfile& styleProfile, const ShipFactionSurfaceDetailProfile& factionProfile) const
    {
        ResolvedSurfaceDetailProfile result;

        result.DetailDensityPercent = composeSurfaceDetailPercent(styleProfile.DetailDensityPercent, factionProfile.DetailDensityPercent);
        result.MechanicalPatternCountPercent = composeSurfaceDetailPercent(styleProfile.MechanicalPatternCountPercent, factionProfile.MechanicalPatternCountPercent);
        result.LightPatternCountPercent = factionProfile.LightPatternCountPercent;
        result.AccentPanelWeight = composeSurfaceDetailPercent(styleProfile.AccentPanelWeight, factionProfile.AccentPanelWeightPercent);
        result.AccentStripeWeight = composeSurfaceDetailPercent(styleProfile.AccentStripeWeight, factionProfile.AccentStripeWeightPercent);
        result.AccentArmorWeight = composeSurfaceDetailPercent(styleProfile.AccentArmorWeight, factionProfile.AccentArmorWeightPercent);
        result.HorizontalVentChance = composeSurfaceDetailChance(styleProfile.HorizontalVentChance, factionProfile.HorizontalVentChancePercent);
        result.SupplementalWeights.PanelSeam = composeSurfaceDetailPercent(styleProfile.SupplementalDetailWeights.PanelSeam, factionProfile.SupplementalWeightMultipliersPercent.PanelSeam);
        result.SupplementalWeights.GeometricMarking = composeSurfaceDetailPercent(styleProfile.SupplementalDetailWeights.GeometricMarking, factionProfile.SupplementalWeightMultipliersPercent.GeometricMarking);
        result.SupplementalWeights.MechanicalExposure = composeSurfaceDetailPercent(styleProfile.SupplementalDetailWeights.MechanicalExposure, factionProfile.SupplementalWeightMultipliersPercent.MechanicalExposure);
        result.SupplementalWeights.RepeatingMotif = composeSurfaceDetailPercent(styleProfile.SupplementalDetailWeights.RepeatingMotif, factionProfile.SupplementalWeightMultipliersPercent.RepeatingMotif);
        result.SupplementalWeights.IdentificationMarking = composeSurfaceDetailPercent(styleProfile.SupplementalDetailWeights.GeometricMarking, factionProfile.SupplementalWeightMultipliersPercent.IdentificationMarking);
        result.SupplementalWeights.LuminousChannel = composeSurfaceDetailPercent(styleProfile.SupplementalDetailWeights.RepeatingMotif, factionProfile.SupplementalWeightMultipliersPercent.LuminousChannel);
        result.MotifWeights.PairedVents = composeSurfaceDetailPercent(styleProfile.DetailMotifWeights.PairedVents, factionProfile.MotifWeightMultipliersPercent.PairedVents);
        result.MotifWeights.TripleVentBank = composeSurfaceDetailPercent(styleProfile.DetailMotifWeights.TripleVentBank, factionProfile.MotifWeightMultipliersPercent.TripleVentBank);
        result.MotifWeights.PairedLights = composeSurfaceDetailPercent(styleProfile.DetailMotifWeights.PairedLights, factionProfile.MotifWeightMultipliersPercent.PairedLights);
        result.MotifWeights.ThreeNodeLights = composeSurfaceDetailPercent(styleProfile.DetailMotifWeights.ThreeNodeLights, factionProfile.MotifWeightMultipliersPercent.ThreeNodeLights);
        result.MotifWeights.ParallelSeams = composeSurfaceDetailPercent(styleProfile.DetailMotifWeights.ParallelSeams, factionProfile.MotifWeightMultipliersPercent.ParallelSeams);
        result.MotifWeights.RepeatedDashes = composeSurfaceDetailPercent(styleProfile.DetailMotifWeights.RepeatedDashes, factionProfile.MotifWeightMultipliersPercent.RepeatedDashes);
        result.MotifWeights.RecessedSlot = composeSurfaceDetailPercent(styleProfile.DetailMotifWeights.RecessedSlot, factionProfile.MotifWeightMultipliersPercent.RecessedSlot);
        result.MotifRepeatPercent = factionProfile.MotifRepeatPercent;
        result.AsymmetricDetailChance = static_cast<uint32_t>(std::clamp(static_cast<int32_t>(settings.AsymmetricDetailChance) + factionProfile.AsymmetricDetailChanceOffset, 0, 100));

        return result;
    }

    bool DetailGenerator::trySelectDetailAnchorFromMask(ShipGenerationContext& context, const PixelMask& mask, uint32_t& x, uint32_t& y) const
    {
        const uint32_t pixelCount = PixelMaskUtils::getMaskPixelCount(mask);

        if (pixelCount == 0u)
        {
            return false;
        }

        uint32_t selectedIndex = context.getGenerationRandomUInt(GenerationDomain::DETAILS, 0u, pixelCount - 1u);

        for (uint32_t candidateY = 0u; candidateY < mask.getHeight(); ++candidateY)
        {
            for (uint32_t candidateX = 0u; candidateX < mask.getWidth(); ++candidateX)
            {
                if (!mask.get(candidateX, candidateY))
                {
                    continue;
                }

                if (selectedIndex == 0u)
                {
                    x = candidateX;
                    y = candidateY;
                    return true;
                }

                --selectedIndex;
            }
        }

        return false;
    }

    uint32_t DetailGenerator::composeSurfaceDetailPercent(uint32_t baseValue, uint32_t multiplierPercent) const
    {
        const uint64_t result = (static_cast<uint64_t>(baseValue) * multiplierPercent + 50u) / 100u;
        return static_cast<uint32_t>(std::min<uint64_t>(result, std::numeric_limits<uint32_t>::max()));
    }

    uint32_t DetailGenerator::composeSurfaceDetailChance(uint32_t baseChance, uint32_t multiplierPercent) const
    {
        return std::min(100u, composeSurfaceDetailPercent(baseChance, multiplierPercent));
    }

    bool DetailGenerator::isSurfaceDetailPixelAvailable(const ShipGenerationContext& context, uint32_t x, uint32_t y, bool allowHullLayerBoundary) const
    {
        const GeneratedShip& ship = context.Ship;
        if (!ship.HullMask.get(x, y))
        {
            return false;
        }
        if (ship.CockpitMask.get(x, y))
        {
            return false;
        }
        if (ship.EngineMask.get(x, y))
        {
            return false;
        }
        if (ship.AttachmentMask.get(x, y))
        {
            return false;
        }
        if (context.Livery.PrimaryMarkingMask.get(x, y) || context.Livery.SecondaryMarkingMask.get(x, y))
        {
            return false;
        }
        if (!allowHullLayerBoundary && (context.HullLayers.BoundaryMask.get(x, y) || context.CoreTreatment.BoundaryMask.get(x, y)))
        {
            return false;
        }
        if (context.CoreTreatment.LuminousMask.get(x, y))
        {
            return false;
        }
        if (context.MajorFeatures.OccupiedMask.get(x, y))
        {
            return false;
        }
        if (context.Weapons.OccupiedMask.get(x, y))
        {
            return false;
        }
        if (ship.AccentMask.get(x, y))
        {
            return false;
        }
        if (ship.MechanicalDetailMask.get(x, y))
        {
            return false;
        }
        if (ship.LightMask.get(x, y))
        {
            return false;
        }

        return true;
    }

    bool DetailGenerator::areSurfaceDetailPixelsAvailable(const ShipGenerationContext& context, const std::vector<std::pair<uint32_t, uint32_t>>& pixels) const
    {
        for (const auto& pixel : pixels)
        {
            if (!isSurfaceDetailPixelAvailable(context, pixel.first, pixel.second)) { return false; }
        }

        return areMotifPixelsMaterialCoherent(context, pixels);
    }

    void DetailGenerator::commitSurfaceDetailPixels(PixelMask& mask, const std::vector<std::pair<uint32_t, uint32_t>>& pixels) const
    {
        for (const auto& pixel : pixels)
        {
            mask.set(pixel.first, pixel.second, true);
        }
    }

    bool DetailGenerator::addSurfaceDetailCandidatePixel(const ShipGenerationContext& context, std::vector<std::pair<uint32_t, uint32_t>>& pixels, int32_t x, int32_t y, bool mirrored) const
    {
        const GeneratedShip& ship = context.Ship;
        const uint32_t width = ship.HullMask.getWidth();
        const uint32_t height = ship.HullMask.getHeight();

        if (x < 0 || y < 0 || x >= static_cast<int32_t>(width) || y >= static_cast<int32_t>(height))
        {
            return false;
        }

        const uint32_t pixelX = static_cast<uint32_t>(x);
        const uint32_t pixelY = static_cast<uint32_t>(y);
        const std::pair<uint32_t, uint32_t> pixel(pixelX, pixelY);

        if (std::find(pixels.begin(), pixels.end(), pixel) == pixels.end())
        {
            pixels.push_back(pixel);
        }

        if (!mirrored)
        {
            return true;
        }

        const std::pair<uint32_t, uint32_t> mirroredPixel(width - 1u - pixelX, pixelY);

        if (std::find(pixels.begin(), pixels.end(), mirroredPixel) == pixels.end())
        {
            pixels.push_back(mirroredPixel);
        }

        return true;
    }
}
