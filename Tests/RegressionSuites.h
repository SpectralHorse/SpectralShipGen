#pragma once

#include "RegressionRunner.h"

#include <vector>

namespace PixelShipGeneratorTests
{
    int runGeneratorRegression();
    int runIdleAnimationRegression();
    int runLateralMovementAnimationRegression();
    int runLongitudinalMovementAnimationRegression();
    int runFiringAnimationRegression();
    int runAnimationStateCompatibilityRegression();
    int runAnimationProfileRoutingRegression();
    int runGenerationDiagnosticsRegression();
    int runGenerationRecipeRegression();
    int runGeneratorStatisticsRegression();
    int runEngineGeometryRegression();
    int runWingGeometryRegression();
    int runMajorFeatureRegression();
    int runWeaponGeometryRegression();
    int runPainterShadingRegression();
    int runComponentDepthReadabilityRegression();
    int runCustomProfileApiRegression();
    int runArbitraryResolutionRegression();
    int runRectangularResolutionRegression();
    int runGenerationScaleTraitsRegression();
    int runGenerationComplexityBudgetRegression();
    int runGenerationSpatialBudgetRegression();
    int runHullLayerRegression();
    int runMacroAsymmetryRegression();
    int runGenerationDomainRerollRegression();
    int runCockpitGeometryRegression();
    int runCoreTreatmentRegression();
    int runStyleExpansionRegression();
    int runStaticProfileRoutingRegression();
    int runFactionExpansionRegression();
    int runFactionProfileRegression();
    int runStaticFactionProfileRoutingRegression();
    int runCustomFactionApiRegression();
    int runPaletteConfigurationRegression();
    int runPublicConfigurationApiRegression();
    int runStructuralNegativeSpaceRegression();
    int runSilhouetteArticulationRegression();
    int runVisualHierarchyRegression();
    int runMaterialCompositionRegression();
    int runLiveryRegression();
    int runDetailMotifRegression();
    int runRegressionRunnerRegression();
    int runDiagnosticsRunnerRegression();
    int runDiagnosticsDashboardRegression();

    int runPreviewPreferencesRegression();
    int runPreviewFavoritesPersistenceRegression();
    int runPreviewFavoritesBrowserRegression();
    int runPreviewUserPresetPersistenceRegression();
    int runPreviewConfigurationBundleRegression();
    int runPreviewSessionRegression();
    int runPreviewWorkspaceRegression();
    int runPreviewInspectionRegression();
    int runPreviewAnimationLabRegression();
    int runGenerationCalibrationRegression();
    int runAttributeRerollStudioRegression();
    int runDiagnosticsAppRegression();
    int runPreviewConfigurationEditorRegression();
    int runPreviewFactionProfileEditorRegression();
    int runPreviewPaletteEditorRegression();

    std::vector<RegressionSuite> createCoreRegressionSuites();
    std::vector<RegressionSuite> createPreviewRegressionSuites();
}
