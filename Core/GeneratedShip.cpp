#include <PixelShipGenerator/GeneratedShip.h>

namespace PixelShipGenerator
{
    void GeneratedShip::reset(uint32_t width, uint32_t height, const ShipGenerationSeeds& seeds)
    {
        Seed = seeds.Master;
        Seeds = seeds;

        HullMask.reset(width, height, false);
        CockpitMask.reset(width, height, false);
        EngineMask.reset(width, height, false);
        EngineExhaustMask.reset(width, height, false);

        AttachmentMask.reset(width, height, false);
        AttachmentPlacements.clear();

        AccentMask.reset(width, height, false);
        MechanicalDetailMask.reset(width, height, false);
        LightMask.reset(width, height, false);

        IdleAnimationMetadata.reset(width, height);

        FinalImage.reset(width, height, Color());
    }

    void GeneratedShip::clear()
    {
        HullMask.clear(false);
        CockpitMask.clear(false);
        EngineMask.clear(false);
        EngineExhaustMask.clear(false);

        AttachmentMask.clear(false);
        AttachmentPlacements.clear();

        AccentMask.clear(false);
        MechanicalDetailMask.clear(false);
        LightMask.clear(false);

        IdleAnimationMetadata.clear();

        FinalImage.clear(Color());
    }
} // namespace PixelShipGenerator
