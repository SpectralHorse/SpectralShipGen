#include "RegressionSuites.h"

namespace PixelShipGeneratorTests
{
    std::vector<RegressionSuite> createCoreRegressionSuites()
    {
        return {
            { "generator", "Generator Determinism", RegressionCategory::DETERMINISM, runGeneratorRegression, false },
            { "idle-animation", "Idle Animation", RegressionCategory::ANIMATION, runIdleAnimationRegression, false },
            { "lateral-movement-animation", "Lateral Movement Animation", RegressionCategory::ANIMATION, runLateralMovementAnimationRegression, false },
            { "longitudinal-movement-animation", "Longitudinal Movement Animation", RegressionCategory::ANIMATION, runLongitudinalMovementAnimationRegression, false },
            { "firing-animation", "Weapon Firing Animation", RegressionCategory::ANIMATION, runFiringAnimationRegression, false },
            { "animation-state-compatibility", "Animation State Compatibility", RegressionCategory::ANIMATION, runAnimationStateCompatibilityRegression, false },
            { "animation-profile-routing", "Animation Profile Routing", RegressionCategory::ANIMATION, runAnimationProfileRoutingRegression, false },
            { "diagnostics", "Generation Diagnostics", RegressionCategory::DIAGNOSTICS, runGenerationDiagnosticsRegression, false },
            { "diagnostics-runner", "Reusable Diagnostics Runner", RegressionCategory::DIAGNOSTICS, runDiagnosticsRunnerRegression, false },
            { "diagnostics-dashboard", "Diagnostics Dashboard Analysis", RegressionCategory::DIAGNOSTICS, runDiagnosticsDashboardRegression, false },
            { "recipe", "Recipe Serialization", RegressionCategory::PERSISTENCE, runGenerationRecipeRegression, false },
            { "statistics", "Generator Statistics", RegressionCategory::DIAGNOSTICS, runGeneratorStatisticsRegression, true },
            { "engines", "Engine Geometry", RegressionCategory::GEOMETRY, runEngineGeometryRegression, true },
            { "wings", "Wing Geometry", RegressionCategory::GEOMETRY, runWingGeometryRegression, true },
            { "major-features", "Major Features", RegressionCategory::GEOMETRY, runMajorFeatureRegression, true },
            { "weapons", "Weapon Geometry", RegressionCategory::GEOMETRY, runWeaponGeometryRegression, true },
            { "shading", "Pixel-art Shading", RegressionCategory::VISUAL_SEMANTICS, runPainterShadingRegression, true },
            { "component-depth-readability", "Component Depth Readability", RegressionCategory::VISUAL_SEMANTICS, runComponentDepthReadabilityRegression, false },
            { "custom-profile-api", "Custom ShipGenerationProfile API", RegressionCategory::DETERMINISM, runCustomProfileApiRegression, false },
            { "arbitrary-resolution", "Arbitrary Resolution", RegressionCategory::GEOMETRY, runArbitraryResolutionRegression, false },
            { "rectangular-resolution", "Rectangular Resolution", RegressionCategory::GEOMETRY, runRectangularResolutionRegression, false },
            { "scale-traits", "Scale Traits", RegressionCategory::GEOMETRY, runGenerationScaleTraitsRegression, true },
            { "complexity-budget", "Complexity Budget", RegressionCategory::VISUAL_SEMANTICS, runGenerationComplexityBudgetRegression, true },
            { "spatial-budget", "Spatial Budget", RegressionCategory::VISUAL_SEMANTICS, runGenerationSpatialBudgetRegression, false },
            { "hull-layers", "Hull Layers", RegressionCategory::VISUAL_SEMANTICS, runHullLayerRegression, true },
            { "macro-asymmetry", "Macro Asymmetry", RegressionCategory::GEOMETRY, runMacroAsymmetryRegression, true },
            { "domain-reroll", "Generation Domain Reroll", RegressionCategory::DETERMINISM, runGenerationDomainRerollRegression, true },
            { "cockpit", "Cockpit Geometry", RegressionCategory::GEOMETRY, runCockpitGeometryRegression, true },
            { "core-treatment", "Core Treatment", RegressionCategory::VISUAL_SEMANTICS, runCoreTreatmentRegression, true },
            { "style-expansion", "Style Expansion", RegressionCategory::VISUAL_SEMANTICS, runStyleExpansionRegression, true },
            { "static-profile-routing", "Static Profile Routing", RegressionCategory::DETERMINISM, runStaticProfileRoutingRegression, false },
            { "faction-expansion", "Faction Expansion", RegressionCategory::VISUAL_SEMANTICS, runFactionExpansionRegression, true },
            { "faction-profile", "Unified ShipFactionProfile", RegressionCategory::DETERMINISM, runFactionProfileRegression, false },
            { "static-faction-profile-routing", "Static Faction Profile Routing", RegressionCategory::DETERMINISM, runStaticFactionProfileRoutingRegression, false },
            { "structural-negative-space", "Structural Negative Space", RegressionCategory::GEOMETRY, runStructuralNegativeSpaceRegression, true },
            { "silhouette-articulation", "Silhouette Articulation", RegressionCategory::GEOMETRY, runSilhouetteArticulationRegression, true },
            { "visual-hierarchy", "Visual Hierarchy", RegressionCategory::VISUAL_SEMANTICS, runVisualHierarchyRegression, true },
            { "material-composition", "Material Composition", RegressionCategory::VISUAL_SEMANTICS, runMaterialCompositionRegression, true },
            { "livery", "Procedural Livery", RegressionCategory::VISUAL_SEMANTICS, runLiveryRegression, true },
            { "detail-motif", "Detail Motif Grammar", RegressionCategory::VISUAL_SEMANTICS, runDetailMotifRegression, true },
            { "runner", "Regression Runner", RegressionCategory::INFRASTRUCTURE, runRegressionRunnerRegression, false }
        };
    }
}
