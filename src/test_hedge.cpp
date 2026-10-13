// Profit and loss of the delta hedging along a market trajectory.
// Usage: test_hedge file.json market.txt
// Output: json with the final P&L, the initial price and its standard deviation.
// The run is long: a price and a delta are computed at each of the H + 1 market dates.

#include <iomanip>
#include <iostream>

#include "json_helper.hpp"
#include "pricer_setup.hpp"
#include "Hedging.hpp"

namespace {

/// Seed of the random generator: fixed, so that the results are reproducible
const unsigned long SEED = 1234;

}  // namespace

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "Usage : " << argv[0] << " file.json market.txt" << std::endl;
        return 1;
    }

    try {
        nlohmann::json json_input = load_json_file(argv[1]);

        BlackScholesModel *model = construct_model(json_input);
        Option *option = construct_option(json_input);
        PnlMat *market = load_market(argv[2]);
        PnlRng *rng = construct_rng(SEED);

        MonteCarlo monte_carlo(*model, *option, read_nb_samples(json_input));
        HedgingResults results = compute_pnl(*model, *option, monte_carlo, market,
                                             read_fd_step(json_input), rng);

        nlohmann::json json_output = {
            {"finalPnL", results.final_pnl},
            {"initialPrice", results.initial_price},
            {"initialPriceStdDev", results.initial_price_std_dev}};
        std::cout << std::setw(4) << json_output << std::endl;

        delete model;
        delete option;
        pnl_mat_free(&market);
        pnl_rng_free(&rng);
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}