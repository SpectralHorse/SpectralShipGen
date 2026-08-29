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
    int runGenerationDiagnosticsRegression();
    int runGenerationRecipeRegression();
    int runGeneratorStatisticsRegression();
    int runEngineGeometryRegression();
    int runWingGeometryRegression();
    int runMajorFeatureRegression();
    int runWeaponGeometryRegression();
    int runPainterShadingRegression();
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
    int runFactionExpansionRegression();
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
    int runGenerationCalibrationRegression();
    int runAttributeRerollStudioRegression();
    int runDiagnosticsAppRegression();

    std::vector<RegressionSuite> createCoreRegressionSuites();
    std::vector<RegressionSuite> createPreviewRegressionSuites();
}
