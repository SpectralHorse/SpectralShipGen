# Library-owned targets. This file is intentionally SFML-independent so it can
# become the build core of the standalone SpectralShipGen Library repository.

add_library(SpectralShipGenCore STATIC
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

target_include_directories(SpectralShipGenCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc/SpectralShipGen
            ${CMAKE_CURRENT_SOURCE_DIR}/Core/Animation
            ${CMAKE_CURRENT_SOURCE_DIR}/Core/Generation
)
target_compile_features(SpectralShipGenCore PUBLIC cxx_std_17)
spectral_ship_gen_enable_sanitizers(SpectralShipGenCore)
set_target_properties(SpectralShipGenCore PROPERTIES EXPORT_NAME Core)
add_library(SpectralShipGen::Core ALIAS SpectralShipGenCore)
install(TARGETS SpectralShipGenCore
    EXPORT SpectralShipGenTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(SPECTRAL_SHIP_GEN_DEVELOPER_TARGET_DEFAULT ON)
else()
    set(SPECTRAL_SHIP_GEN_DEVELOPER_TARGET_DEFAULT OFF)
endif()

option(SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS "Build the reusable diagnostics library" ${SPECTRAL_SHIP_GEN_DEVELOPER_TARGET_DEFAULT})
if(SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS)
add_library(SpectralShipGenDiagnosticsCore STATIC
    Diagnostics/GenerationStatistics.cpp
    Diagnostics/DiagnosticsRunner.cpp
    Diagnostics/DiagnosticsAnalysis.cpp
    Diagnostics/DiagnosticsResultSerializer.cpp
)
target_include_directories(SpectralShipGenDiagnosticsCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(SpectralShipGenDiagnosticsCore PUBLIC SpectralShipGen::Core)
target_compile_features(SpectralShipGenDiagnosticsCore PUBLIC cxx_std_17)
spectral_ship_gen_enable_sanitizers(SpectralShipGenDiagnosticsCore)
set_target_properties(SpectralShipGenDiagnosticsCore PROPERTIES EXPORT_NAME Diagnostics)
add_library(SpectralShipGen::Diagnostics ALIAS SpectralShipGenDiagnosticsCore)
install(TARGETS SpectralShipGenDiagnosticsCore
    EXPORT SpectralShipGenTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
endif()

option(SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS_CLI "Build the SFML-independent diagnostics CLI" ${SPECTRAL_SHIP_GEN_DEVELOPER_TARGET_DEFAULT})
if(SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS_CLI)
    if(NOT TARGET SpectralShipGen::Diagnostics)
        message(FATAL_ERROR "SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS_CLI requires SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS=ON")
    endif()
    add_executable(ShipGeneratorDiagnostics Diagnostics/ShipGeneratorDiagnostics_main.cpp)
    target_link_libraries(ShipGeneratorDiagnostics PRIVATE SpectralShipGen::Core SpectralShipGen::Diagnostics)
    target_compile_features(ShipGeneratorDiagnostics PRIVATE cxx_std_17)
    spectral_ship_gen_enable_sanitizers(ShipGeneratorDiagnostics)
    target_compile_definitions(ShipGeneratorDiagnostics PRIVATE SPECTRAL_SHIP_GEN_BUILD_CONFIGURATION="${CMAKE_BUILD_TYPE}")
endif()

option(SPECTRAL_SHIP_GEN_BUILD_EXAMPLES "Build the runnable public API examples" ${SPECTRAL_SHIP_GEN_DEVELOPER_TARGET_DEFAULT})
if(SPECTRAL_SHIP_GEN_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

option(SPECTRAL_SHIP_GEN_BUILD_CORE_REGRESSION "Build the unified Core regression runner" ${SPECTRAL_SHIP_GEN_DEVELOPER_TARGET_DEFAULT})
if(SPECTRAL_SHIP_GEN_BUILD_CORE_REGRESSION)
    if(NOT TARGET SpectralShipGen::Diagnostics)
        message(FATAL_ERROR "SPECTRAL_SHIP_GEN_BUILD_CORE_REGRESSION requires SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS=ON")
    endif()
    add_executable(SpectralShipGenRegression
        Tests/SpectralShipGenRegression_main.cpp
        Tests/RegressionRunner.cpp
        Tests/CoreRegressionSuites.cpp
        Tests/RegressionRunnerRegression.cpp
        Tests/DiagnosticsRunnerRegression.cpp
        Tests/DiagnosticsDashboardRegression.cpp
        Tests/Task106ReleaseRobustnessRegression.cpp

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
    target_include_directories(SpectralShipGenRegression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Tests
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/SpectralShipGen
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Generation
    )
    target_link_libraries(SpectralShipGenRegression PRIVATE SpectralShipGen::Core SpectralShipGen::Diagnostics)
    spectral_ship_gen_enable_sanitizers(SpectralShipGenRegression)
    target_compile_features(SpectralShipGenRegression PRIVATE cxx_std_17)
endif()

option(SPECTRAL_SHIP_GEN_BUILD_PUBLIC_HEADER_CHECKS "Compile every public header from an isolated translation unit" ${SPECTRAL_SHIP_GEN_DEVELOPER_TARGET_DEFAULT})
if(SPECTRAL_SHIP_GEN_BUILD_PUBLIC_HEADER_CHECKS)
    file(GLOB_RECURSE SPECTRAL_SHIP_GEN_PUBLIC_HEADERS
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/inc"
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/inc/SpectralShipGen/*.h"
    )
    set(SPECTRAL_SHIP_GEN_PUBLIC_HEADER_CHECK_SOURCES)
    foreach(PUBLIC_HEADER IN LISTS SPECTRAL_SHIP_GEN_PUBLIC_HEADERS)
        string(MAKE_C_IDENTIFIER "${PUBLIC_HEADER}" HEADER_IDENTIFIER)
        set(CHECK_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/public_header_checks/${HEADER_IDENTIFIER}.cpp")
        file(WRITE "${CHECK_SOURCE}" "#include <${PUBLIC_HEADER}>\n")
        list(APPEND SPECTRAL_SHIP_GEN_PUBLIC_HEADER_CHECK_SOURCES "${CHECK_SOURCE}")
    endforeach()
    add_library(SpectralShipGenPublicHeaderChecks OBJECT ${SPECTRAL_SHIP_GEN_PUBLIC_HEADER_CHECK_SOURCES})
    target_link_libraries(SpectralShipGenPublicHeaderChecks PRIVATE SpectralShipGen::Core)
    spectral_ship_gen_enable_sanitizers(SpectralShipGenPublicHeaderChecks)
    target_compile_features(SpectralShipGenPublicHeaderChecks PRIVATE cxx_std_17)
endif()

if(BUILD_TESTING AND TARGET SpectralShipGenRegression)
    set(SPECTRAL_SHIP_GEN_CORE_NORMAL_SUITES
        generator idle-animation lateral-movement-animation longitudinal-movement-animation
        firing-animation animation-state-compatibility animation-profile-routing diagnostics
        diagnostics-runner diagnostics-dashboard recipe arbitrary-resolution rectangular-resolution
        spatial-budget static-profile-routing custom-profile-api custom-faction-api faction-profile
        static-faction-profile-routing palette-configuration component-depth-readability
        public-configuration-api runner
    )
    set(SPECTRAL_SHIP_GEN_CORE_LONG_SUITES
        scale-traits complexity-budget statistics engines wings major-features weapons shading
        hull-layers macro-asymmetry domain-reroll cockpit core-treatment style-expansion
        faction-expansion structural-negative-space silhouette-articulation visual-hierarchy
        material-composition livery detail-motif release-robustness
    )
    foreach(SUITE_NAME IN LISTS SPECTRAL_SHIP_GEN_CORE_NORMAL_SUITES)
        add_test(NAME core.${SUITE_NAME} COMMAND SpectralShipGenRegression --suite ${SUITE_NAME})
        set_tests_properties(core.${SUITE_NAME} PROPERTIES LABELS "core;normal")
    endforeach()
    foreach(SUITE_NAME IN LISTS SPECTRAL_SHIP_GEN_CORE_LONG_SUITES)
        add_test(NAME core.${SUITE_NAME} COMMAND SpectralShipGenRegression --suite ${SUITE_NAME})
        set_tests_properties(core.${SUITE_NAME} PROPERTIES LABELS "core;long")
    endforeach()
endif()
