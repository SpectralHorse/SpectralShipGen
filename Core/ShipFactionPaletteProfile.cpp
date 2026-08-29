#include "ShipFactionPaletteProfile.h"

namespace PixelShipGenerator
{
    namespace
    {
        ShipFactionPaletteProfile createFrontierProfile()
        {
            ShipFactionPaletteProfile profile;

            profile.HullHue = { 185u, 215u };
            profile.HullSaturation = { 12u, 28u };
            profile.HullValue = { 42u, 58u };

            profile.Accent.HueOffset = { -35, 25 };
            profile.Accent.Saturation = { 20u, 45u };
            profile.Accent.Value = { 45u, 68u };

            profile.Cockpit.HueOffset = { -5, 25 };
            profile.Cockpit.Saturation = { 50u, 75u };
            profile.Cockpit.Value = { 50u, 72u };

            profile.Light.HueOffset = { -55, -20 };
            profile.Light.Saturation = { 55u, 80u };
            profile.Light.Value = { 70u, 92u };

            profile.Exhaust.HueOffset = { 150, 180 };
            profile.Exhaust.Saturation = { 70u, 95u };
            profile.Exhaust.Value = { 85u, 100u };

            profile.MechanicalSaturation = { 5u, 18u };
            profile.MechanicalValue = { 24u, 40u };

            return profile;
        }

        ShipFactionPaletteProfile createMilitaryProfile()
        {
            ShipFactionPaletteProfile profile;

            profile.HullHue = { 80u, 135u };
            profile.HullSaturation = { 8u, 24u };
            profile.HullValue = { 34u, 50u };

            profile.Accent.HueOffset = { 230, 300 };
            profile.Accent.Saturation = { 55u, 85u };
            profile.Accent.Value = { 65u, 90u };

            profile.Cockpit.HueOffset = { 70, 130 };
            profile.Cockpit.Saturation = { 45u, 70u };
            profile.Cockpit.Value = { 50u, 72u };

            profile.Light.HueOffset = { -110, -60 };
            profile.Light.Saturation = { 70u, 95u };
            profile.Light.Value = { 75u, 100u };

            profile.Exhaust.HueOffset = { -100, -50 };
            profile.Exhaust.Saturation = { 75u, 100u };
            profile.Exhaust.Value = { 88u, 100u };

            profile.MechanicalSaturation = { 4u, 14u };
            profile.MechanicalValue = { 20u, 34u };

            return profile;
        }

        ShipFactionPaletteProfile createAscendantProfile()
        {
            ShipFactionPaletteProfile profile;

            profile.HullHue = { 185u, 235u };
            profile.HullSaturation = { 8u, 20u };
            profile.HullValue = { 70u, 88u };

            profile.Accent.HueOffset = { 20, 90 };
            profile.Accent.Saturation = { 35u, 65u };
            profile.Accent.Value = { 70u, 95u };

            profile.Cockpit.HueOffset = { 0, 55 };
            profile.Cockpit.Saturation = { 35u, 60u };
            profile.Cockpit.Value = { 70u, 90u };

            profile.Light.HueOffset = { 40, 120 };
            profile.Light.Saturation = { 55u, 85u };
            profile.Light.Value = { 90u, 100u };

            profile.Exhaust.HueOffset = { 20, 100 };
            profile.Exhaust.Saturation = { 40u, 75u };
            profile.Exhaust.Value = { 90u, 100u };

            profile.MechanicalSaturation = { 5u, 15u };
            profile.MechanicalValue = { 45u, 60u };

            return profile;
        }

        ShipFactionPaletteProfile createXenoProfile()
        {
            ShipFactionPaletteProfile profile;

            profile.HullHue = { 250u, 340u };
            profile.HullSaturation = { 35u, 65u };
            profile.HullValue = { 40u, 65u };

            profile.Accent.HueOffset = { 80, 160 };
            profile.Accent.Saturation = { 55u, 85u };
            profile.Accent.Value = { 55u, 85u };

            profile.Cockpit.HueOffset = { 120, 220 };
            profile.Cockpit.Saturation = { 45u, 75u };
            profile.Cockpit.Value = { 45u, 75u };

            profile.Light.HueOffset = { 80, 220 };
            profile.Light.Saturation = { 70u, 95u };
            profile.Light.Value = { 85u, 100u };

            profile.Exhaust.HueOffset = { 120, 240 };
            profile.Exhaust.Saturation = { 60u, 90u };
            profile.Exhaust.Value = { 80u, 100u };

            profile.MechanicalSaturation = { 20u, 40u };
            profile.MechanicalValue = { 20u, 40u };

            return profile;
        }

        ShipFactionPaletteProfile createCorporateProfile()
        {
            ShipFactionPaletteProfile profile;

            // Commercial finishes stay near-neutral so a single disciplined
            // identification color reads clearly as a manufactured brand cue.
            profile.HullHue = { 175u, 245u };
            profile.HullSaturation = { 4u, 18u };
            profile.HullValue = { 48u, 82u };

            profile.Accent.HueOffset = { 70, 285 };
            profile.Accent.Saturation = { 70u, 95u };
            profile.Accent.Value = { 68u, 96u };

            profile.Cockpit.HueOffset = { -10, 45 };
            profile.Cockpit.Saturation = { 35u, 62u };
            profile.Cockpit.Value = { 64u, 90u };

            profile.Light.HueOffset = { 65, 160 };
            profile.Light.Saturation = { 58u, 86u };
            profile.Light.Value = { 86u, 100u };

            profile.Exhaust.HueOffset = { 140, 205 };
            profile.Exhaust.Saturation = { 55u, 82u };
            profile.Exhaust.Value = { 90u, 100u };

            profile.MechanicalSaturation = { 2u, 12u };
            profile.MechanicalValue = { 20u, 36u };

            return profile;
        }

        ShipFactionPaletteProfile createRelicProfile()
        {
            ShipFactionPaletteProfile profile;

            // Relic hulls are dark and subdued; sparse emissive roles provide
            // the high-contrast "internally alive" read instead of bright armor.
            profile.HullHue = { 205u, 330u };
            profile.HullSaturation = { 10u, 34u };
            profile.HullValue = { 24u, 46u };

            profile.Accent.HueOffset = { -30, 45 };
            profile.Accent.Saturation = { 18u, 45u };
            profile.Accent.Value = { 32u, 58u };

            profile.Cockpit.HueOffset = { 90, 180 };
            profile.Cockpit.Saturation = { 46u, 76u };
            profile.Cockpit.Value = { 46u, 72u };

            profile.Light.HueOffset = { 95, 205 };
            profile.Light.Saturation = { 76u, 100u };
            profile.Light.Value = { 92u, 100u };

            profile.Exhaust.HueOffset = { 90, 190 };
            profile.Exhaust.Saturation = { 62u, 92u };
            profile.Exhaust.Value = { 78u, 96u };

            profile.MechanicalSaturation = { 8u, 24u };
            profile.MechanicalValue = { 12u, 28u };

            return profile;
        }
    }

    const ShipFactionPaletteProfile& getShipFactionPaletteProfile(ShipFactionType faction)
    {
        static const ShipFactionPaletteProfile FrontierProfile = createFrontierProfile();
        static const ShipFactionPaletteProfile MilitaryProfile = createMilitaryProfile();
        static const ShipFactionPaletteProfile AscendantProfile = createAscendantProfile();
        static const ShipFactionPaletteProfile XenoProfile = createXenoProfile();
        static const ShipFactionPaletteProfile CorporateProfile = createCorporateProfile();
        static const ShipFactionPaletteProfile RelicProfile = createRelicProfile();

        switch (faction)
        {
        case ShipFactionType::FRONTIER: return FrontierProfile;
        case ShipFactionType::MILITARY: return MilitaryProfile;
        case ShipFactionType::ASCENDANT: return AscendantProfile;
        case ShipFactionType::XENO: return XenoProfile;
        case ShipFactionType::CORPORATE: return CorporateProfile;
        case ShipFactionType::RELIC: return RelicProfile;
        default: return FrontierProfile;
        }
    }
}