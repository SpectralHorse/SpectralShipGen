#include "RegressionRunner.h"
#include "CoreRegressionSuites.h"

#include <iostream>

int main(int argc, char** argv)
{
    const std::vector<SpectralShipGenTests::RegressionSuite> suites = SpectralShipGenTests::createCoreRegressionSuites();
    return SpectralShipGenTests::runRegressionRunner(argc, argv, suites, "SpectralShipGenRegression", std::cout, std::cerr);
}
