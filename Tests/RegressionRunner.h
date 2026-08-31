#pragma once

#include <iosfwd>
#include <string_view>
#include <vector>

namespace SpectralShipGenTests
{
    enum class RegressionCategory
    {
        DETERMINISM,
        GEOMETRY,
        VISUAL_SEMANTICS,
        ANIMATION,
        PERSISTENCE,
        DIAGNOSTICS,
        TOOLING,
        INFRASTRUCTURE
    };

    struct RegressionSuite
    {
        const char* Name = nullptr;
        const char* DisplayName = nullptr;
        RegressionCategory Category = RegressionCategory::INFRASTRUCTURE;
        int (*Run)() = nullptr;
        bool LongRunning = false;
    };

    const char* getRegressionCategoryName(RegressionCategory category);

    int runRegressionRunner(
        int argc,
        const char* const* argv,
        const std::vector<RegressionSuite>& suites,
        const char* runnerName,
        std::ostream& output,
        std::ostream& errors);
}
