# Library-owned targets. This file is intentionally SFML-independent so it can
# become the build core of the standalone PixelShipGenerator Library repository.

add_library(PixelShipGeneratorCore STATIC
    Core/BuiltInPresetCatalog.cpp
    Core/GeneratedShip.cpp
    Core/GenerationComplexityBudget.cpp
    Core/GenerationDomain.cpp
    Core/GenerationDomainReroll.cpp
    Core/GenerationSpatialBudget.cpp
    Core/GenerationTuningProfile.cpp
    Core/Image.cpp
    Core/PixelMask.cpp
    Core/ShipAttachmentProfile.cpp
    Core/ShipFactionPaletteProfile.cpp
    Core/ShipFactionProfile.cpp
    Core/ShipFactionProfileValidation.cpp
    Core/ShipGenerationProfile.cpp
    Core/ShipGenerationProfileValidation.cpp
    Core/ShipGenerationSeeds.cpp
    Core/ShipGenerationPerformance.cpp
    Core/ShipGenerationRecipe.cpp
    Core/ShipGenerationRecipeJson.cpp
    Core/ShipGenerationRecipeProfileSerialization.cpp
    Core/ShipGenerationRecipeSerializer.cpp
    Core/ShipResolvedGenerationConfiguration.cpp
    Core/ShipMaterialZoneType.cpp
    Core/ShipLiveryType.cpp
    Core/ShipDetailMotifType.cpp
    Core/ShipPaletteGenerationProfile.cpp
    Core/ShipPaletteGenerationProfileValidation.cpp
    Core/ShipPaletteGenerator.cpp
    Core/ShipSurfaceDetailProfile.cpp
    Core/ShipStructuralNegativeSpaceType.cpp
    Core/ShipVisualAnchorType.cpp
    Core/SilhouetteQualityMetrics.cpp
    Core/ShipGenerator.cpp

    Core/Animation/AnimationSamplingPlanner.cpp
    Core/Animation/ShipAnimationPose.cpp
    Core/Animation/ShipAnimationStateCoordinator.cpp
    Core/Animation/ShipFiringAnimator.cpp
    Core/Animation/ShipIdleAnimator.cpp
    Core/Animation/ShipIdleAnimationInternal.cpp
    Core/Animation/ShipIdleAnimationPlanner.cpp
    Core/Animation/ShipIdleAnimationFrameEvaluator.cpp
    Core/Animation/ShipLateralMovementAnimator.cpp
    Core/Animation/ShipLongitudinalMovementAnimator.cpp
    Core/Animation/ShipMovementAnimation.cpp
    Core/Animation/ShipSpritesheetUtils.cpp

    Core/Generation/AttachmentGenerator.cpp
    Core/Generation/CockpitGenerator.cpp
    Core/Generation/CoreTreatmentGenerator.cpp
    Core/Generation/DetailGenerator.cpp
    Core/Generation/EngineGenerator.cpp
    Core/Generation/GenerationMath.cpp
    Core/Generation/HullGenerator.cpp
    Core/Generation/HullLayerGenerator.cpp
    Core/Generation/MacroAsymmetryPlanner.cpp
    Core/Generation/MajorFeatureGenerator.cpp
    Core/Generation/MaterialCompositionGenerator.cpp
    Core/Generation/LiveryGenerator.cpp
    Core/Generation/PixelMaskUtils.cpp
    Core/Generation/ShipGenerationContext.cpp
    Core/Generation/ShipPainter.cpp
    Core/Generation/WeaponCandidateBuilder.cpp
    Core/Generation/WeaponCandidateValidator.cpp
    Core/Generation/WeaponGenerator.cpp
    Core/Generation/WeaponHardpointPlanner.cpp
    Core/Generation/VisualHierarchyPlanner.cpp
)

target_include_directories(PixelShipGeneratorCore
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc/PixelShipGenerator
            ${CMAKE_CURRENT_SOURCE_DIR}/Core/Animation
            ${CMAKE_CURRENT_SOURCE_DIR}/Core/Generation
)
target_compile_features(PixelShipGeneratorCore PUBLIC cxx_std_17)

add_library(PixelShipGeneratorDiagnosticsCore STATIC
    Diagnostics/GenerationStatistics.cpp
    Diagnostics/DiagnosticsRunner.cpp
    Diagnostics/DiagnosticsAnalysis.cpp
    Diagnostics/DiagnosticsResultSerializer.cpp
)
target_include_directories(PixelShipGeneratorDiagnosticsCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(PixelShipGeneratorDiagnosticsCore PUBLIC PixelShipGeneratorCore)
target_compile_features(PixelShipGeneratorDiagnosticsCore PUBLIC cxx_std_17)

option(PIXEL_SHIP_GENERATOR_BUILD_DIAGNOSTICS_CLI "Build the SFML-independent diagnostics CLI" ON)
if(PIXEL_SHIP_GENERATOR_BUILD_DIAGNOSTICS_CLI)
    add_executable(ShipGeneratorDiagnostics Diagnostics/ShipGeneratorDiagnostics_main.cpp)
    target_link_libraries(ShipGeneratorDiagnostics PRIVATE PixelShipGeneratorCore PixelShipGeneratorDiagnosticsCore)
    target_compile_features(ShipGeneratorDiagnostics PRIVATE cxx_std_17)
    target_compile_definitions(ShipGeneratorDiagnostics PRIVATE PIXEL_SHIP_GENERATOR_BUILD_CONFIGURATION="${CMAKE_BUILD_TYPE}")
endif()

option(PIXEL_SHIP_GENERATOR_BUILD_EXAMPLES "Compile the public API example translation units" ON)
if(PIXEL_SHIP_GENERATOR_BUILD_EXAMPLES)
    add_library(PixelShipGeneratorExamples OBJECT
        Examples/custom_profile_generation.cpp
        Examples/public_configuration_api.cpp
        Examples/recipe_serialization.cpp
    )
    target_link_libraries(PixelShipGeneratorExamples PRIVATE PixelShipGeneratorCore)
    target_compile_features(PixelShipGeneratorExamples PRIVATE cxx_std_17)
endif()

option(PIXEL_SHIP_GENERATOR_BUILD_CORE_REGRESSION "Build the unified Core regression runner" ON)
if(PIXEL_SHIP_GENERATOR_BUILD_CORE_REGRESSION)
    add_executable(PixelShipGeneratorRegression
        Tests/PixelShipGeneratorRegression_main.cpp
        Tests/RegressionRunner.cpp
        Tests/CoreRegressionSuites.cpp
        Tests/RegressionRunnerRegression.cpp
        Tests/DiagnosticsRunnerRegression.cpp
        Tests/DiagnosticsDashboardRegression.cpp

        Tests/GenerationComplexityBudgetRegression.cpp
        Tests/GenerationDomainRerollRegression.cpp
        Tests/GenerationScaleTraitsRegression.cpp
        Tests/GenerationSpatialBudgetRegression.cpp
        Tests/MacroAsymmetryRegression.cpp
        Tests/ShipArbitraryResolutionRegression.cpp
        Tests/ShipCockpitGeometryRegression.cpp
        Tests/ShipCoreTreatmentRegression.cpp
        Tests/ShipDetailMotifRegression.cpp
        Tests/ShipEngineGeometryRegression.cpp
        Tests/ShipFactionExpansionRegression.cpp
        Tests/ShipFactionProfileRegression.cpp
        Tests/StaticFactionProfileRoutingRegression.cpp
        Tests/ShipCustomFactionApiRegression.cpp
        Tests/ShipPaletteConfigurationRegression.cpp
        Tests/PublicConfigurationApiRegression.cpp
        Tests/ShipGenerationDiagnosticsRegression.cpp
        Tests/ShipGenerationRecipeRegression.cpp
        Tests/ShipGeneratorRegression.cpp
        Tests/ShipGeneratorStatisticsRegression.cpp
        Tests/ShipHullLayerRegression.cpp
        Tests/ShipIdleAnimationRegression.cpp
        Tests/ShipLateralMovementAnimationRegression.cpp
        Tests/ShipLongitudinalMovementAnimationRegression.cpp
        Tests/ShipFiringAnimationRegression.cpp
        Tests/ShipAnimationStateCompatibilityRegression.cpp
        Tests/ShipAnimationProfileRoutingRegression.cpp
        Tests/ShipLiveryRegression.cpp
        Tests/ShipMajorFeatureRegression.cpp
        Tests/ShipMaterialCompositionRegression.cpp
        Tests/ShipPainterShadingRegression.cpp
        Tests/ShipComponentDepthReadabilityRegression.cpp
        Tests/ShipCustomProfileApiRegression.cpp
        Tests/ShipRectangularResolutionRegression.cpp
        Tests/ShipSilhouetteArticulationRegression.cpp
        Tests/ShipStructuralNegativeSpaceRegression.cpp
        Tests/ShipStyleExpansionRegression.cpp
        Tests/StaticProfileRoutingRegression.cpp
        Tests/ShipVisualHierarchyRegression.cpp
        Tests/ShipWeaponGeometryRegression.cpp
        Tests/ShipWingGeometryRegression.cpp
    )
    target_include_directories(PixelShipGeneratorRegression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Tests
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/PixelShipGenerator
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Generation
    )
    target_link_libraries(PixelShipGeneratorRegression PRIVATE PixelShipGeneratorCore PixelShipGeneratorDiagnosticsCore)
    target_compile_features(PixelShipGeneratorRegression PRIVATE cxx_std_17)
endif()

if(BUILD_TESTING AND TARGET PixelShipGeneratorRegression)
    set(PIXEL_SHIP_GENERATOR_CORE_NORMAL_SUITES
        generator idle-animation lateral-movement-animation longitudinal-movement-animation
        firing-animation animation-state-compatibility animation-profile-routing diagnostics
        diagnostics-runner diagnostics-dashboard recipe arbitrary-resolution rectangular-resolution
        spatial-budget static-profile-routing custom-profile-api custom-faction-api faction-profile
        static-faction-profile-routing palette-configuration component-depth-readability
        public-configuration-api runner
    )
    set(PIXEL_SHIP_GENERATOR_CORE_LONG_SUITES
        scale-traits complexity-budget statistics engines wings major-features weapons shading
        hull-layers macro-asymmetry domain-reroll cockpit core-treatment style-expansion
        faction-expansion structural-negative-space silhouette-articulation visual-hierarchy
        material-composition livery detail-motif
    )
    foreach(SUITE_NAME IN LISTS PIXEL_SHIP_GENERATOR_CORE_NORMAL_SUITES)
        add_test(NAME core.${SUITE_NAME} COMMAND PixelShipGeneratorRegression --suite ${SUITE_NAME})
        set_tests_properties(core.${SUITE_NAME} PROPERTIES LABELS "core;normal")
    endforeach()
    foreach(SUITE_NAME IN LISTS PIXEL_SHIP_GENERATOR_CORE_LONG_SUITES)
        add_test(NAME core.${SUITE_NAME} COMMAND PixelShipGeneratorRegression --suite ${SUITE_NAME})
        set_tests_properties(core.${SUITE_NAME} PROPERTIES LABELS "core;long")
    endforeach()
endif()
