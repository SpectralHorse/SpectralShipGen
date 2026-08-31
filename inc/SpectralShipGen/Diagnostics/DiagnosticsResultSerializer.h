#pragma once

#include <filesystem>
#include <string>

#include <SpectralShipGen/Diagnostics/DiagnosticsRunner.h>

namespace SpectralShipGenDiagnostics
{
    constexpr uint32_t DiagnosticsResultSchemaVersion = 2u;

    struct DiagnosticsResultLoadResult
    {
        bool Success = false;
        DiagnosticsResult Result;
        std::string Error;
    };

    std::string serializeDiagnosticsResultJson(const DiagnosticsResult& result);
    DiagnosticsResultLoadResult deserializeDiagnosticsResultJson(const std::string& jsonText);
    bool saveDiagnosticsResultJson(const std::filesystem::path& path, const DiagnosticsResult& result, std::string& errorMessage);
    DiagnosticsResultLoadResult loadDiagnosticsResultJson(const std::filesystem::path& path);
}
