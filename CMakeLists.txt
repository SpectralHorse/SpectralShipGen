cmake_minimum_required(VERSION 3.16)

set(SPECTRAL_SHIP_GEN_PACKAGE_VERSION "0.0.0" CACHE STRING
    "Development package version placeholder; Task 109 will establish the release version contract")
project(SpectralShipGen VERSION ${SPECTRAL_SHIP_GEN_PACKAGE_VERSION} LANGUAGES CXX)

if(MSVC)
    add_compile_options(/wd26812)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

option(SPECTRAL_SHIP_GEN_ENABLE_SANITIZERS "Enable ASan+UBSan for SpectralShipGen project targets" OFF)
if(SPECTRAL_SHIP_GEN_ENABLE_SANITIZERS)
    if(MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "SPECTRAL_SHIP_GEN_ENABLE_SANITIZERS requires GCC or Clang on a non-MSVC toolchain")
    endif()
endif()

function(spectral_ship_gen_enable_sanitizers target_name)
    if(NOT SPECTRAL_SHIP_GEN_ENABLE_SANITIZERS)
        return()
    endif()

    target_compile_options(${target_name} PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )

    get_target_property(_spectral_ship_gen_target_type ${target_name} TYPE)
    if(NOT _spectral_ship_gen_target_type STREQUAL "STATIC_LIBRARY"
       AND NOT _spectral_ship_gen_target_type STREQUAL "OBJECT_LIBRARY"
       AND NOT _spectral_ship_gen_target_type STREQUAL "INTERFACE_LIBRARY")
        target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

include(cmake/SpectralShipGenLibraryTargets.cmake)

set(SPECTRAL_SHIP_GEN_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/SpectralShipGen")
configure_package_config_file(
    cmake/SpectralShipGenConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/SpectralShipGenConfig.cmake"
    INSTALL_DESTINATION "${SPECTRAL_SHIP_GEN_INSTALL_CMAKEDIR}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SpectralShipGenConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(EXPORT SpectralShipGenTargets
    FILE SpectralShipGenTargets.cmake
    NAMESPACE SpectralShipGen::
    DESTINATION "${SPECTRAL_SHIP_GEN_INSTALL_CMAKEDIR}"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SpectralShipGenConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SpectralShipGenConfigVersion.cmake"
    DESTINATION "${SPECTRAL_SHIP_GEN_INSTALL_CMAKEDIR}"
)
install(DIRECTORY inc/SpectralShipGen DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES README.md DESTINATION "${CMAKE_INSTALL_DOCDIR}")
