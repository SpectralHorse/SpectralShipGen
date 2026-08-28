#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "GenerationStatistics.h"
#include "ShipGenerationRecipeSerializer.h"

namespace
{
    constexpr std::array<uint32_t, 7u> SupportedResolutions = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };

    struct CommandLineOptions
    {
        PixelShipGeneratorDiagnostics::DiagnosticGenerationConfiguration BaseConfiguration;
        bool AllResolutions = false;
        bool AllStyles = false;
        bool AllFactions = false;
        std::optional<std::string> CsvPath;
    };

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
        output << "  --all-resolutions                 Sweep all seven PreviewApp resolutions.\n";
        output << "  --all-styles                      Sweep every current ShipStyle enum value.\n";
        output << "  --all-factions                    Sweep every current ShipFactionType enum value.\n";
        output << "  --csv <path>                      Write one summary row per configuration.\n";
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
                if (!PixelShipGeneratorPreview::shipStyleFromRecipeString(value, options.BaseConfiguration.Style))
                {
                    error = "Unknown style: " + value;
                    return false;
                }
            }
            else if (argument == "--faction")
            {
                if (!PixelShipGeneratorPreview::shipFactionFromRecipeString(value, options.BaseConfiguration.Faction))
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

    std::vector<PixelShipGenerator::ShipStyle> getSelectedStyles(const CommandLineOptions& options)
    {
        if (!options.AllStyles)
        {
            return { options.BaseConfiguration.Style };
        }

        std::vector<PixelShipGenerator::ShipStyle> styles;

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END); ++index)
        {
            styles.push_back(static_cast<PixelShipGenerator::ShipStyle>(index));
        }

        return styles;
    }

    std::vector<PixelShipGenerator::ShipFactionType> getSelectedFactions(const CommandLineOptions& options)
    {
        if (!options.AllFactions)
        {
            return { options.BaseConfiguration.Faction };
        }

        std::vector<PixelShipGenerator::ShipFactionType> factions;

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END); ++index)
        {
            factions.push_back(static_cast<PixelShipGenerator::ShipFactionType>(index));
        }

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

    std::ofstream csvFile;

    if (options.CsvPath.has_value())
    {
        csvFile.open(*options.CsvPath, std::ios::out | std::ios::trunc);

        if (!csvFile)
        {
            std::cerr << "Error: failed to open CSV path: " << *options.CsvPath << '\n';
            return 1;
        }

        PixelShipGeneratorDiagnostics::writeGenerationStatisticsCsvHeader(csvFile);
    }

    const std::vector<uint32_t> resolutions = getSelectedResolutions(options);
    const std::vector<PixelShipGenerator::ShipStyle> styles = getSelectedStyles(options);
    const std::vector<PixelShipGenerator::ShipFactionType> factions = getSelectedFactions(options);
    const uint64_t dimensionConfigurationCount = options.AllResolutions ? resolutions.size() : 1u;
    const uint64_t configurationCount = dimensionConfigurationCount * styles.size() * factions.size();
    uint64_t configurationIndex = 0u;

    for (uint64_t dimensionIndex = 0u; dimensionIndex < dimensionConfigurationCount; ++dimensionIndex)
    {
        PixelShipGeneratorDiagnostics::DiagnosticGenerationConfiguration dimensionConfiguration = options.BaseConfiguration;
        if (options.AllResolutions)
        {
            dimensionConfiguration.Width = resolutions[dimensionIndex];
            dimensionConfiguration.Height = resolutions[dimensionIndex];
        }

        for (const PixelShipGenerator::ShipStyle style : styles)
        {
            for (const PixelShipGenerator::ShipFactionType faction : factions)
            {
                ++configurationIndex;
                PixelShipGeneratorDiagnostics::DiagnosticGenerationConfiguration configuration = dimensionConfiguration;
                configuration.Style = style;
                configuration.Faction = faction;
                std::cout << "\nRunning configuration " << configurationIndex << '/' << configurationCount << ": " << configuration.Width << 'x' << configuration.Height << ' ' << PixelShipGeneratorPreview::shipStyleToRecipeString(style) << ' ' << PixelShipGeneratorPreview::shipFactionToRecipeString(faction) << " ...\n";
                const PixelShipGeneratorDiagnostics::GenerationStatistics statistics = PixelShipGeneratorDiagnostics::collectGenerationStatistics(configuration);
                PixelShipGeneratorDiagnostics::printGenerationStatistics(std::cout, configuration, statistics);

                if (csvFile)
                {
                    PixelShipGeneratorDiagnostics::writeGenerationStatisticsCsvRow(csvFile, configuration, statistics);
                }
            }
        }
    }

    if (csvFile)
    {
        std::cout << "\nCSV summary written: " << *options.CsvPath << '\n';
    }

    return 0;
}
