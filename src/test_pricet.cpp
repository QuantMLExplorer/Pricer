// Price and delta of an option at a time t read on a market trajectory.
// Usage: test_pricet file.json market.txt market_line_t
//   market_line_t : row of the market file giving the current time t = market_line_t * T / H.
// Output: json with t, the price, the deltas and their standard deviations.

#include <cstdlib>
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
    if (argc != 4) {
        std::cerr << "Usage : " << argv[0] << " file.json market.txt market_line_t" << std::endl;
        return 1;
    }

    try {
        nlohmann::json json_input = load_json_file(argv[1]);

        BlackScholesModel *model = construct_model(json_input);
        Option *option = construct_option(json_input);

        PnlMat *market = load_market(argv[2]);
        int market_line_t = std::atoi(argv[3]);
        double t = time_of_market_line(option->getT(), market, market_line_t);
        PnlMat *past = construct_past(market, option->getN(), market_line_t);

        PnlRng *rng = construct_rng(SEED);
        MonteCarlo monte_carlo(*model, *option, read_nb_samples(json_input));

        // Price at t
        double price, confidence_half_width;
        monte_carlo.price(past, t, rng, price, confidence_half_width);

        // Delta at t
        int nb_assets = option->getD();
        PnlVect *delta = pnl_vect_create(nb_assets);
        PnlVect *delta_std_dev = pnl_vect_create(nb_assets);
        monte_carlo.delta(past, t, read_fd_step(json_input), rng, delta, delta_std_dev);

        nlohmann::json json_output = {
            {"t", t},
            {"price", price},
            {"priceStdDev", confidence_half_width / CONFIDENCE_95_QUANTILE},
            {"delta", delta},
            {"deltaStdDev", delta_std_dev}};
        std::cout << std::setw(4) << json_output << std::endl;

        pnl_vect_free(&delta);
        pnl_vect_free(&delta_std_dev);
        delete model;
        delete option;
        pnl_mat_free(&market);
        pnl_mat_free(&past);
        pnl_rng_free(&rng);
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}