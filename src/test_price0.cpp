// Price of an option at time 0.
// Usage: test_price0 file.json
// Output: json with the price and the standard deviation of the Monte Carlo estimator.

#include <iomanip>
#include <iostream>

#include "json_helper.hpp"
#include "pricer_setup.hpp"
#include "MonteCarlo.hpp"

namespace {

/// Quantile of the standard normal distribution for a two-sided 95% confidence interval
const double CONFIDENCE_95_QUANTILE = 1.96;

/// Seed of the random generator: fixed, so that the results are reproducible
const unsigned long SEED = 1234;

}  // namespace

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage : " << argv[0] << " file.json" << std::endl;
        return 1;
    }

    try {
        nlohmann::json json_input = load_json_file(argv[1]);

        BlackScholesModel *model = construct_model(json_input);
        Option *option = construct_option(json_input);
        PnlRng *rng = construct_rng(SEED);

        MonteCarlo monte_carlo(*model, *option, read_nb_samples(json_input));
        double price, confidence_half_width;
        monte_carlo.price(rng, price, confidence_half_width);

        nlohmann::json json_output = {
            {"price", price},
            {"priceStdDev", confidence_half_width / CONFIDENCE_95_QUANTILE}};
        std::cout << std::setw(4) << json_output << std::endl;

        delete model;
        delete option;
        pnl_rng_free(&rng);
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}