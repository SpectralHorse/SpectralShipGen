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
