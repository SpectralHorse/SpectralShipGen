#include "RegressionRunner.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <exception>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>

namespace PixelShipGeneratorTests
{
    namespace
    {
        struct RunnerOptions
        {
            bool Help = false;
            bool List = false;
            bool RunAll = false;
            bool IncludeLong = false;
            std::vector<std::string> SuiteNames;
            std::vector<std::string> Filters;
        };

        struct SuiteExecution
        {
            const RegressionSuite* Suite = nullptr;
            bool Passed = false;
            double Seconds = 0.0;
        };

        std::string normalize(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
            {
                if (character == '_')
                {
                    return '-';
                }
                return static_cast<char>(std::tolower(character));
            });
            return value;
        }

        bool containsNormalized(std::string_view text, const std::string& filter)
        {
            return normalize(std::string(text)).find(filter) != std::string::npos;
        }

        void printUsage(std::ostream& output, const char* runnerName)
        {
            output
                << "Usage: " << runnerName << " [options]\n"
                << "  --list                  List registered suites.\n"
                << "  --suite <name>          Run one suite; may be repeated.\n"
                << "  --filter <text>         Run matching suite/category names; may be repeated.\n"
                << "  --all                   Run all normal suites.\n"
                << "  --include-long          Include long-running suites with default/--all/--filter.\n"
                << "  --help                  Show this help.\n\n"
                << "With no selector, all normal suites run. An explicitly named --suite runs even if long-running.\n";
        }

        bool parseOptions(int argc, const char* const* argv, RunnerOptions& options, std::string& error)
        {
            for (int index = 1; index < argc; ++index)
            {
                const std::string argument = argv[index] != nullptr ? argv[index] : "";
                if (argument == "--help" || argument == "-h")
                {
                    options.Help = true;
                }
                else if (argument == "--list")
                {
                    options.List = true;
                }
                else if (argument == "--all")
                {
                    options.RunAll = true;
                }
                else if (argument == "--include-long")
                {
                    options.IncludeLong = true;
                }
                else if (argument == "--suite" || argument == "--filter")
                {
                    if (index + 1 >= argc)
                    {
                        error = argument + " requires a value.";
                        return false;
                    }
                    const std::string value = argv[++index] != nullptr ? argv[index] : "";
                    if (value.empty())
                    {
                        error = argument + " requires a non-empty value.";
                        return false;
                    }
                    if (argument == "--suite")
                    {
                        options.SuiteNames.push_back(value);
                    }
                    else
                    {
                        options.Filters.push_back(value);
                    }
                }
                else
                {
                    error = "Unknown argument: " + argument;
                    return false;
                }
            }
            return true;
        }

        const RegressionSuite* findSuite(const std::vector<RegressionSuite>& suites, const std::string& requested)
        {
            const std::string normalizedRequested = normalize(requested);
            const auto iterator = std::find_if(suites.begin(), suites.end(), [&](const RegressionSuite& suite)
            {
                return suite.Name != nullptr && normalize(suite.Name) == normalizedRequested;
            });
            return iterator != suites.end() ? &(*iterator) : nullptr;
        }

        bool matchesFilters(const RegressionSuite& suite, const std::vector<std::string>& filters)
        {
            if (filters.empty())
            {
                return false;
            }

            const std::string category = normalize(getRegressionCategoryName(suite.Category));
            for (const std::string& rawFilter : filters)
            {
                const std::string filter = normalize(rawFilter);
                if ((suite.Name != nullptr && containsNormalized(suite.Name, filter)) ||
                    (suite.DisplayName != nullptr && containsNormalized(suite.DisplayName, filter)) ||
                    category.find(filter) != std::string::npos)
                {
                    return true;
                }
            }
            return false;
        }

        bool selectSuites(
            const std::vector<RegressionSuite>& suites,
            const RunnerOptions& options,
            std::vector<const RegressionSuite*>& selected,
            std::string& error)
        {
            std::set<const RegressionSuite*> uniqueSuites;

            for (const std::string& name : options.SuiteNames)
            {
                const RegressionSuite* suite = findSuite(suites, name);
                if (suite == nullptr)
                {
                    error = "Unknown suite: " + name;
                    return false;
                }
                uniqueSuites.insert(suite);
            }

            const bool hasBroadSelection = options.RunAll || !options.Filters.empty() || options.SuiteNames.empty();
            if (hasBroadSelection)
            {
                for (const RegressionSuite& suite : suites)
                {
                    const bool selectedByDefault = options.SuiteNames.empty() && options.Filters.empty();
                    const bool selectedByAll = options.RunAll;
                    const bool selectedByFilter = matchesFilters(suite, options.Filters);
                    if (!(selectedByDefault || selectedByAll || selectedByFilter))
                    {
                        continue;
                    }
                    if (suite.LongRunning && !options.IncludeLong && !selectedByFilter)
                    {
                        continue;
                    }
                    uniqueSuites.insert(&suite);
                }
            }

            for (const RegressionSuite& suite : suites)
            {
                if (uniqueSuites.find(&suite) != uniqueSuites.end())
                {
                    selected.push_back(&suite);
                }
            }

            if (selected.empty())
            {
                error = "No regression suites matched the requested selection.";
                return false;
            }
            return true;
        }

        void printSuiteList(const std::vector<RegressionSuite>& suites, std::ostream& output)
        {
            output << "Registered regression suites: " << suites.size() << '\n';
            for (const RegressionSuite& suite : suites)
            {
                output << "  " << std::left << std::setw(32) << (suite.Name != nullptr ? suite.Name : "")
                       << "  " << std::setw(18) << getRegressionCategoryName(suite.Category)
                       << (suite.LongRunning ? "  LONG" : "  NORMAL")
                       << "  " << (suite.DisplayName != nullptr ? suite.DisplayName : "") << '\n';
            }
        }
    }

    const char* getRegressionCategoryName(RegressionCategory category)
    {
        switch (category)
        {
        case RegressionCategory::DETERMINISM: return "DETERMINISM";
        case RegressionCategory::GEOMETRY: return "GEOMETRY";
        case RegressionCategory::VISUAL_SEMANTICS: return "VISUAL_SEMANTICS";
        case RegressionCategory::ANIMATION: return "ANIMATION";
        case RegressionCategory::PERSISTENCE: return "PERSISTENCE";
        case RegressionCategory::DIAGNOSTICS: return "DIAGNOSTICS";
        case RegressionCategory::TOOLING: return "TOOLING";
        case RegressionCategory::INFRASTRUCTURE: return "INFRASTRUCTURE";
        }
        return "UNKNOWN";
    }

    int runRegressionRunner(
        int argc,
        const char* const* argv,
        const std::vector<RegressionSuite>& suites,
        const char* runnerName,
        std::ostream& output,
        std::ostream& errors)
    {
        RunnerOptions options;
        std::string error;
        if (!parseOptions(argc, argv, options, error))
        {
            errors << "Regression runner error: " << error << '\n';
            printUsage(errors, runnerName);
            return 2;
        }

        if (options.Help)
        {
            printUsage(output, runnerName);
            return 0;
        }

        if (options.List)
        {
            printSuiteList(suites, output);
            return 0;
        }

        std::vector<const RegressionSuite*> selected;
        if (!selectSuites(suites, options, selected, error))
        {
            errors << "Regression runner error: " << error << '\n';
            return 2;
        }

        const auto totalStart = std::chrono::steady_clock::now();
        std::vector<SuiteExecution> executions;
        executions.reserve(selected.size());
        std::size_t passedCount = 0u;

        output << "Running " << selected.size() << " regression suite" << (selected.size() == 1u ? "" : "s") << "...\n";
        for (const RegressionSuite* suite : selected)
        {
            const auto suiteStart = std::chrono::steady_clock::now();
            int result = 1;
            try
            {
                if (suite != nullptr && suite->Run != nullptr)
                {
                    result = suite->Run();
                }
            }
            catch (const std::exception& exception)
            {
                errors << "Unhandled exception in " << suite->Name << ": " << exception.what() << '\n';
                result = 1;
            }
            catch (...)
            {
                errors << "Unhandled non-standard exception in " << suite->Name << ".\n";
                result = 1;
            }

            const auto suiteEnd = std::chrono::steady_clock::now();
            const double seconds = std::chrono::duration<double>(suiteEnd - suiteStart).count();
            const bool passed = result == 0;
            passedCount += passed ? 1u : 0u;
            executions.push_back({ suite, passed, seconds });

            output << (passed ? "[PASS] " : "[FAIL] ")
                   << std::left << std::setw(36) << suite->DisplayName
                   << std::right << std::fixed << std::setprecision(2) << seconds << " s\n";
        }

        const auto totalEnd = std::chrono::steady_clock::now();
        const double totalSeconds = std::chrono::duration<double>(totalEnd - totalStart).count();
        const std::size_t failedCount = executions.size() - passedCount;

        output << "\nSummary\n"
               << "  Suites: " << executions.size() << '\n'
               << "  Passed: " << passedCount << '\n'
               << "  Failed: " << failedCount << '\n'
               << "  Time:   " << std::fixed << std::setprecision(2) << totalSeconds << " s\n";

        if (failedCount != 0u)
        {
            output << "  Failed suites:";
            for (const SuiteExecution& execution : executions)
            {
                if (!execution.Passed)
                {
                    output << ' ' << execution.Suite->Name;
                }
            }
            output << '\n';
        }

        return failedCount == 0u ? 0 : 1;
    }
}
