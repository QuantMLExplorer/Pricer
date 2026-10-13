// Checks the simulation of the market under the historical probability (drift = trend):
// the mean of S_T / S_0 of the first asset must be exp(trend * T), not exp(r * T).
// Usage: test_simul_market file.json
// Output: the empirical mean, the expected value, and [OK] or [ECHEC] (within 4 standard errors).

#include <cmath>
#include <iostream>

#include "io/pricer_setup.hpp"

namespace {

/// Seed of the random generator: fixed, so that the results are reproducible
const unsigned long SEED = 1234;

/// Number of simulated markets used to estimate the mean
const long NB_RUNS = 100000;

/// Number of standard errors tolerated between the empirical and the expected mean
const double NB_STANDARD_ERRORS = 4.0;

/// Value of a json key holding either a number or an array: the first component for an array.
double first_value(const nlohmann::json &json_input, const char *key)
{
    const nlohmann::json &node = json_input.at(key);
    return node.is_array() ? node[0].get<double>() : node.get<double>();
}

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

        double maturity = json_input.at("maturity").get<double>();
        int nb_market_steps = json_input.at("hedging dates number").get<int>();
        double trend = first_value(json_input, "trend");
        double volatility = first_value(json_input, "volatility");

        PnlRng *rng = construct_rng(SEED);
        PnlMat *market = pnl_mat_create(nb_market_steps + 1, model->getD());

        // Mean of S_T / S_0 for the first asset over many simulated markets
        double sum_of_ratios = 0.0;
        for (long run = 0; run < NB_RUNS; run++) {
            model->simul_market(market, maturity, nb_market_steps, rng);
            sum_of_ratios += pnl_mat_get(market, nb_market_steps, 0) / pnl_mat_get(market, 0, 0);
        }
        double empirical_mean = sum_of_ratios / NB_RUNS;

        // S_T / S_0 is log-normal: its mean is exp(mu T) and its variance
        // exp(2 mu T) * (exp(sigma^2 T) - 1)
        double expected_mean = exp(trend * maturity);
        double standard_error =
            expected_mean * sqrt(exp(volatility * volatility * maturity) - 1.0) / sqrt((double)NB_RUNS);

        std::cout << "mean S_T/S_0 = " << empirical_mean << ", expected exp(trend T) = " << expected_mean
                  << ", standard error = " << standard_error << std::endl;
        bool passed = std::fabs(empirical_mean - expected_mean) < NB_STANDARD_ERRORS * standard_error;
        std::cout << (passed ? "[OK]" : "[ECHEC]") << std::endl;

        delete model;
        pnl_mat_free(&market);
        pnl_rng_free(&rng);
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}