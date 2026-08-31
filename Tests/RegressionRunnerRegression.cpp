#include "RegressionRunner.h"
#include "CoreRegressionSuites.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{
    int passSuite()
    {
        return 0;
    }

    int failSuite()
    {
        return 1;
    }

    int invoke(
        const std::vector<std::string>& arguments,
        const std::vector<PixelShipGeneratorTests::RegressionSuite>& suites,
        std::string& output,
        std::string& errors)
    {
        std::vector<const char*> argv;
        argv.reserve(arguments.size());
        for (const std::string& argument : arguments)
        {
            argv.push_back(argument.c_str());
        }

        std::ostringstream outputStream;
        std::ostringstream errorStream;
        const int result = PixelShipGeneratorTests::runRegressionRunner(
            static_cast<int>(argv.size()), argv.data(), suites, "RegressionRunnerFixture", outputStream, errorStream);
        output = outputStream.str();
        errors = errorStream.str();
        return result;
    }
}

int PixelShipGeneratorTests::runRegressionRunnerRegression()
{
    const std::vector<RegressionSuite> suites = {
        { "alpha", "Alpha Geometry", RegressionCategory::GEOMETRY, passSuite, false },
        { "beta-long", "Beta Long", RegressionCategory::DETERMINISM, passSuite, true },
        { "gamma", "Gamma Tooling", RegressionCategory::TOOLING, failSuite, false }
    };

    std::string output;
    std::string errors;

    if (invoke({ "runner", "--list" }, suites, output, errors) != 0 || output.find("alpha") == std::string::npos || output.find("beta-long") == std::string::npos || output.find("LONG") == std::string::npos)
    {
        return 1;
    }

    if (invoke({ "runner", "--suite", "alpha" }, suites, output, errors) != 0 || output.find("Suites: 1") == std::string::npos || output.find("[PASS]") == std::string::npos || output.find(" s") == std::string::npos)
    {
        return 1;
    }

    if (invoke({ "runner", "--suite", "missing" }, suites, output, errors) == 0 || errors.find("Unknown suite") == std::string::npos)
    {
        return 1;
    }

    if (invoke({ "runner", "--filter", "geometry" }, suites, output, errors) != 0 || output.find("Suites: 1") == std::string::npos)
    {
        return 1;
    }

    if (invoke({ "runner", "--filter", "beta" }, suites, output, errors) != 0 || output.find("Suites: 1") == std::string::npos || output.find("Beta Long") == std::string::npos)
    {
        return 1;
    }

    if (invoke({ "runner", "--suite", "alpha", "--suite", "beta-long" }, suites, output, errors) != 0 || output.find("Suites: 2") == std::string::npos)
    {
        return 1;
    }

    if (invoke({ "runner", "--all" }, suites, output, errors) == 0 || output.find("Suites: 2") == std::string::npos || output.find("beta-long") != std::string::npos)
    {
        return 1;
    }

    if (invoke({ "runner", "--all", "--include-long" }, suites, output, errors) == 0 || output.find("Suites: 3") == std::string::npos || output.find("Beta Long") == std::string::npos)
    {
        return 1;
    }

    if (invoke({ "runner", "--suite", "gamma" }, suites, output, errors) == 0 || output.find("Failed: 1") == std::string::npos || output.find("[FAIL]") == std::string::npos)
    {
        return 1;
    }

    return 0;
}
