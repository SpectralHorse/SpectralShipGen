#pragma once

#include <SpectralShipGen/Color.h>

namespace SpectralShipGen
{
    struct ShipPalette
    {
        Color Transparent = Color(0, 0, 0, 0);
        Color Outline = Color(18, 22, 28, 255);

        // Hull
        Color HullDeepShadow = Color(38, 50, 62, 255);
        Color HullShadow = Color(55, 72, 88, 255);
        Color HullBase = Color(88, 122, 132, 255);
        Color HullHighlight = Color(142, 164, 178, 255);
        Color HullSecondary = Color(80, 104, 122, 255);
        Color HullEdgeHighlight = Color(174, 192, 202, 255);

        // Cockpit
        Color CockpitDark = Color(18, 50, 68, 255);
        Color CockpitBase = Color(40, 112, 148, 255);
        Color CockpitHighlight = Color(116, 206, 228, 255);
        Color CockpitGlint = Color(196, 244, 255, 255);

        // Engine
        Color EngineDark = Color(38, 42, 48, 255);
        Color EngineBase = Color(82, 90, 98, 255);
        Color EngineHighlight = Color(154, 164, 170, 255);
        Color EngineHotCore = Color(255, 190, 72, 255);

        // Exhaust
        Color ExhaustBase = Color(255, 104, 36, 255);
        Color ExhaustHighlight = Color(255, 218, 92, 255);
        Color ExhaustHotCore = Color(255, 244, 180, 255);

        // Panels, accents, vents and small details
        Color HullAccentDark = Color(64, 76, 86, 255);
        Color HullAccent = Color(114, 128, 138, 255);
        Color HullAccentHighlight = Color(174, 186, 192, 255);

        Color MechanicalDark = Color(24, 30, 36, 255);
        Color MechanicalBase = Color(56, 66, 74, 255);

        Color LightBase = Color(52, 186, 158, 255);
        Color LightHighlight = Color(164, 255, 222, 255);
    };
} //namespace SpectralShipGen
