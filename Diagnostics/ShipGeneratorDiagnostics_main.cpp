#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <SpectralShipGen/Diagnostics/DiagnosticsRunner.h>
#include <SpectralShipGen/Diagnostics/GenerationStatistics.h>
#include <SpectralShipGen/BuiltInPresetCatalog.h>

namespace
{
    constexpr std::array<uint32_t, 7u> SupportedResolutions = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };

    struct CommandLineOptions
    {
        SpectralShipGenDiagnostics::DiagnosticGenerationConfiguration BaseConfiguration;
        bool AllResolutions = false;
        bool AllStyles = false;
        bool AllFactions = false;
        std::optional<std::string> CsvPath;
        bool DetailedPerformance = false;
    };


    bool parseStyle(const std::string& value, SpectralShipGen::ShipStyle& style)
    {
        return SpectralShipGen::tryGetBuiltInStructuralPreset(value, style);
    }

    bool parseFaction(const std::string& value, SpectralShipGen::ShipFactionType& faction)
    {
        return SpectralShipGen::tryGetBuiltInFactionPreset(value, faction);
    }
    void printUsage(std::ostream& output)
    {
        output << "ShipGeneratorDiagnostics\n\n";
        output << "Usage:\n";
        output << "  ShipGeneratorDiagnostics [options]\n\n";
        output << "Options:\n";
        output << "  --samples <count>                 Samples per configuration. Default: 1000. Zero is allowed.\n";
        output << "  --seed <uint64>                   Deterministic diagnostic root seed.\n";
        output << "  --resolution <24|32|44|64|96|128|160>  Set both width and height to a square preset.\n";
        output << "  --width <pixels>                   Set diagnostic generation width.\n";
        output << "  --height <pixels>                  Set diagnostic generation height.\n";
        output << "  --style <SLEEK|FIGHTER|HEAVY|INDUSTRIAL|SPEARHEAD|DELTA>\n";
        output << "  --faction <FRONTIER|MILITARY|ASCENDANT|XENO|CORPORATE|RELIC>\n";
        output << "  --attachments <on|off>            Enable or disable attachment generation.\n";
        output << "  --detail-density <0-100>\n";
        output << "  --asymmetric-detail-chance <0-100>\n";
        output << "  --all-resolutions                 Sweep all seven standard diagnostic resolutions.\n";
        output << "  --all-styles                      Sweep every current ShipStyle enum value.\n";
        output << "  --all-factions                    Sweep every current ShipFactionType enum value.\n";
        output << "  --csv <path>                      Write extended summary CSV from DiagnosticsResult.\n";
        output << "  --performance                    Enable detailed per-stage timing instrumentation.\n";
        output << "  --help                            Show this help.\n";
    }

    bool parseUInt64(const std::string& text, uint64_t& value)
    {
        if (text.empty() || text.front() == '-')
        {
            return false;
        }

        try
        {
            std::size_t parsedCharacters = 0u;
            const unsigned long long parsedValue = std::stoull(text, &parsedCharacters, 10);

            if (parsedCharacters != text.size())
            {
                return false;
            }

            value = static_cast<uint64_t>(parsedValue);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool parseUInt32(const std::string& text, uint32_t& value)
    {
        uint64_t parsedValue = 0u;

        if (!parseUInt64(text, parsedValue) || parsedValue > std::numeric_limits<uint32_t>::max())
        {
            return false;
        }

        value = static_cast<uint32_t>(parsedValue);
        return true;
    }

    bool isSupportedResolution(uint32_t resolution)
    {
        return std::find(SupportedResolutions.begin(), SupportedResolutions.end(), resolution) != SupportedResolutions.end();
    }

    bool parseCommandLine(int argc, char** argv, CommandLineOptions& options, std::string& error)
    {
        for (int index = 1; index < argc; ++index)
        {
            const std::string argument = argv[index];

            if (argument == "--help")
            {
                printUsage(std::cout);
                return false;
            }

            if (argument == "--all-resolutions")
            {
                options.AllResolutions = true;
                continue;
            }

            if (argument == "--all-styles")
            {
                options.AllStyles = true;
                continue;
            }

            if (argument == "--all-factions")
            {
                options.AllFactions = true;
                continue;
            }

            if (argument == "--performance")
            {
                options.DetailedPerformance = true;
                continue;
            }

            if (index + 1 >= argc)
            {
                error = "Missing value for argument: " + argument;
                return false;
            }

            const std::string value = argv[++index];

            if (argument == "--samples")
            {
                if (!parseUInt64(value, options.BaseConfiguration.Samples))
                {
                    error = "Invalid sample count: " + value;
                    return false;
                }
            }
            else if (argument == "--seed")
            {
                if (!parseUInt64(value, options.BaseConfiguration.DiagnosticSeed))
                {
                    error = "Invalid diagnostic seed: " + value;
                    return false;
                }
            }
            else if (argument == "--resolution")
            {
                uint32_t resolution = 0u;

                if (!parseUInt32(value, resolution) || !isSupportedResolution(resolution))
                {
                    error = "Unsupported resolution: " + value;
                    return false;
                }

                options.BaseConfiguration.Width = resolution;
                options.BaseConfiguration.Height = resolution;
            }
            else if (argument == "--width")
            {
                if (!parseUInt32(value, options.BaseConfiguration.Width) || options.BaseConfiguration.Width < 16u)
                {
                    error = "Invalid width: " + value;
                    return false;
                }
            }
            else if (argument == "--height")
            {
                if (!parseUInt32(value, options.BaseConfiguration.Height) || options.BaseConfiguration.Height < 16u)
                {
                    error = "Invalid height: " + value;
                    return false;
                }
            }
            else if (argument == "--style")
            {
                if (!parseStyle(value, options.BaseConfiguration.Style))
                {
                    error = "Unknown style: " + value;
                    return false;
                }
            }
            else if (argument == "--faction")
            {
                if (!parseFaction(value, options.BaseConfiguration.Faction))
                {
                    error = "Unknown faction: " + value;
                    return false;
                }
            }
            else if (argument == "--attachments")
            {
                if (value == "on")
                {
                    options.BaseConfiguration.AttachmentsEnabled = true;
                }
                else if (value == "off")
                {
                    options.BaseConfiguration.AttachmentsEnabled = false;
                }
                else
                {
                    error = "Invalid --attachments value: " + value + ". Expected on or off.";
                    return false;
                }
            }
            else if (argument == "--detail-density")
            {
                if (!parseUInt32(value, options.BaseConfiguration.DetailDensity) || options.BaseConfiguration.DetailDensity > 100u)
                {
                    error = "Invalid detail density: " + value;
                    return false;
                }
            }
            else if (argument == "--asymmetric-detail-chance")
            {
                if (!parseUInt32(value, options.BaseConfiguration.AsymmetricDetailChance) || options.BaseConfiguration.AsymmetricDetailChance > 100u)
                {
                    error = "Invalid asymmetric detail chance: " + value;
                    return false;
                }
            }
            else if (argument == "--csv")
            {
                options.CsvPath = value;
            }
            else
            {
                error = "Unknown argument: " + argument;
                return false;
            }
        }

        return true;
    }

    std::vector<uint32_t> getSelectedResolutions(const CommandLineOptions& options)
    {
        if (options.AllResolutions)
        {
            return std::vector<uint32_t>(SupportedResolutions.begin(), SupportedResolutions.end());
        }

        return { options.BaseConfiguration.Width };
    }

    std::vector<SpectralShipGen::ShipStyle> getSelectedStyles(const CommandLineOptions& options)
    {
        if (!options.AllStyles)
        {
            return { options.BaseConfiguration.Style };
        }

        std::vector<SpectralShipGen::ShipStyle> styles;
        for (const auto& entry : SpectralShipGen::getBuiltInStructuralPresetCatalog()) { styles.push_back(entry.Preset); }
        return styles;
    }

    std::vector<SpectralShipGen::ShipFactionType> getSelectedFactions(const CommandLineOptions& options)
    {
        if (!options.AllFactions)
        {
            return { options.BaseConfiguration.Faction };
        }

        std::vector<SpectralShipGen::ShipFactionType> factions;
        for (const auto& entry : SpectralShipGen::getBuiltInFactionPresetCatalog()) { factions.push_back(entry.Preset); }
        return factions;
    }
}

int main(int argc, char** argv)
{
    CommandLineOptions options;
    std::string error;

    if (!parseCommandLine(argc, argv, options, error))
    {
        if (!error.empty())
        {
            std::cerr << "Error: " << error << "\n\n";
            printUsage(std::cerr);
            return 1;
        }

        return 0;
    }

    const std::vector<uint32_t> resolutions = getSelectedResolutions(options);
    const std::vector<SpectralShipGen::ShipStyle> styles = getSelectedStyles(options);
    const std::vector<SpectralShipGen::ShipFactionType> factions = getSelectedFactions(options);

    SpectralShipGenDiagnostics::DiagnosticsRunConfiguration runConfiguration;
    runConfiguration.Dimensions.clear();
    if (options.AllResolutions)
    {
        for (const uint32_t resolution : resolutions) { runConfiguration.Dimensions.push_back({ resolution, resolution }); }
    }
    else
    {
        runConfiguration.Dimensions.push_back({ options.BaseConfiguration.Width, options.BaseConfiguration.Height });
    }
    runConfiguration.Styles = styles;
    runConfiguration.Factions = factions;
    runConfiguration.SamplesPerConfiguration = options.BaseConfiguration.Samples;
    runConfiguration.DiagnosticSeed = options.BaseConfiguration.DiagnosticSeed;
    runConfiguration.DetailDensity = options.BaseConfiguration.DetailDensity;
    runConfiguration.AsymmetricDetailChance = options.BaseConfiguration.AsymmetricDetailChance;
    runConfiguration.AttachmentsEnabled = options.BaseConfiguration.AttachmentsEnabled;
    runConfiguration.DetailedPerformanceInstrumentation = options.DetailedPerformance;
#ifdef SPECTRAL_SHIP_GEN_BUILD_CONFIGURATION
    runConfiguration.BuildConfiguration = SPECTRAL_SHIP_GEN_BUILD_CONFIGURATION;
#endif

    uint64_t lastPrinted = 0u;
    const SpectralShipGenDiagnostics::DiagnosticsResult result = SpectralShipGenDiagnostics::DiagnosticsRunner().run(runConfiguration,
        [&](const SpectralShipGenDiagnostics::DiagnosticsProgress& progress)
        {
            const uint64_t interval = std::max<uint64_t>(1u, progress.TotalWorkItems / 20u);
            if (progress.CompletedWorkItems != progress.TotalWorkItems && progress.CompletedWorkItems - lastPrinted < interval) { return; }
            lastPrinted = progress.CompletedWorkItems;
            std::cout << "\rProgress " << progress.CompletedWorkItems << '/' << progress.TotalWorkItems << " (" << static_cast<uint32_t>(progress.ProgressPercent) << "%)";
            if (progress.EstimatedRemainingAvailable) { std::cout << " ETA ~" << (progress.EstimatedRemainingNanoseconds / 1000000000.0) << " s"; }
            std::cout << std::flush;
        });
    if (result.ScheduledWorkItems != 0u) { std::cout << '\n'; }

    for (const auto& configurationResult : result.ConfigurationResults)
    {
        SpectralShipGenDiagnostics::printGenerationStatistics(std::cout, configurationResult.Configuration, configurationResult.Statistics);
    }
    SpectralShipGenDiagnostics::printDiagnosticsResultSummary(std::cout, result);

    if (options.CsvPath.has_value())
    {
        std::ofstream csvFile(*options.CsvPath, std::ios::out | std::ios::trunc);
        if (!csvFile)
        {
            std::cerr << "Error: failed to open CSV path: " << *options.CsvPath << '\n';
            return 1;
        }
        SpectralShipGenDiagnostics::writeDiagnosticsResultCsv(csvFile, result);
        std::cout << "\nCSV summary written: " << *options.CsvPath << '\n';
    }

    if (result.Cancelled || !result.Completed)
    {
        return 2;
    }

    return 0;
}
