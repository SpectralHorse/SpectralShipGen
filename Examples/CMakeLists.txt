# These examples intentionally consume only the public SpectralShipGen target.
# They can be built either as part of the Library checkout or as a standalone
# project against an installed SpectralShipGen package.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.16)
    project(SpectralShipGenPublicExamples LANGUAGES CXX)
    find_package(SpectralShipGen CONFIG REQUIRED)
endif()

set(SPECTRAL_SHIP_GEN_PUBLIC_EXAMPLE_SOURCES
    built_in_generation.cpp
    custom_structural_profile.cpp
    custom_faction_profile.cpp
    palette_configuration.cpp
    fully_custom_generation.cpp
    recipe_round_trip.cpp
    animation.cpp
    deterministic_reroll.cpp
)

add_custom_target(SpectralShipGenExamples)
foreach(EXAMPLE_SOURCE IN LISTS SPECTRAL_SHIP_GEN_PUBLIC_EXAMPLE_SOURCES)
    get_filename_component(EXAMPLE_NAME "${EXAMPLE_SOURCE}" NAME_WE)
    set(EXAMPLE_TARGET "SpectralShipGenExample_${EXAMPLE_NAME}")
    add_executable(${EXAMPLE_TARGET} "${EXAMPLE_SOURCE}")
    target_link_libraries(${EXAMPLE_TARGET} PRIVATE SpectralShipGen::Core)
    target_compile_features(${EXAMPLE_TARGET} PRIVATE cxx_std_17)
    add_dependencies(SpectralShipGenExamples ${EXAMPLE_TARGET})
endforeach()
