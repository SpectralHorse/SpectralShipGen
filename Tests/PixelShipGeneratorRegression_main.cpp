#include "RegressionRunner.h"
#include "CoreRegressionSuites.h"

#include <iostream>

int main(int argc, char** argv)
{
    const std::vector<PixelShipGeneratorTests::RegressionSuite> suites = PixelShipGeneratorTests::createCoreRegressionSuites();
    return PixelShipGeneratorTests::runRegressionRunner(argc, argv, suites, "PixelShipGeneratorRegression", std::cout, std::cerr);
}
