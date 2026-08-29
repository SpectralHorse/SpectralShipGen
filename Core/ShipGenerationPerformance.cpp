#include <PixelShipGenerator/ShipGenerationPerformance.h>

namespace PixelShipGenerator
{
    const char* getShipGenerationPerformanceStageName(ShipGenerationPerformanceStage stage)
    {
        switch (stage)
        {
        case ShipGenerationPerformanceStage::SETUP_PLANNING: return "SETUP_PLANNING";
        case ShipGenerationPerformanceStage::HULL_GENERATION: return "HULL_GENERATION";
        case ShipGenerationPerformanceStage::HULL_VALIDATION: return "HULL_VALIDATION";
        case ShipGenerationPerformanceStage::MACRO_ASYMMETRY_PLANNING: return "MACRO_ASYMMETRY_PLANNING";
        case ShipGenerationPerformanceStage::COCKPIT: return "COCKPIT";
        case ShipGenerationPerformanceStage::ENGINES: return "ENGINES";
        case ShipGenerationPerformanceStage::CENTRAL_CORE: return "CENTRAL_CORE";
        case ShipGenerationPerformanceStage::HULL_LAYERS: return "HULL_LAYERS";
        case ShipGenerationPerformanceStage::MAJOR_FEATURES: return "MAJOR_FEATURES";
        case ShipGenerationPerformanceStage::WEAPONS: return "WEAPONS";
        case ShipGenerationPerformanceStage::ATTACHMENTS: return "ATTACHMENTS";
        case ShipGenerationPerformanceStage::MATERIAL_COMPOSITION: return "MATERIAL_COMPOSITION";
        case ShipGenerationPerformanceStage::LIVERY: return "LIVERY";
        case ShipGenerationPerformanceStage::DETAILS: return "DETAILS";
        case ShipGenerationPerformanceStage::PAINTING_COMPOSITION: return "PAINTING_COMPOSITION";
        default: return "UNKNOWN";
        }
    }
}
