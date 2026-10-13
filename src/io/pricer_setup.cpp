#include "io/pricer_setup.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

#include "io/json_helper.hpp"
#include "options/BasketOption.hpp"
#include "options/AsianOption.hpp"
#include "options/PerformanceOption.hpp"

namespace {

/// Reads the vector stored under `key`. If it has a single component, extends it to nb_assets.
/// The caller must free the returned vector.
PnlVect *read_vector(const nlohmann::json &json_input, const char *key, int nb_assets)
{
    PnlVect *vector = NULL;
    json_input.at(key).get_to(vector);
    if (vector->size == 1 && nb_assets > 1) {
        pnl_vect_resize_from_scalar(vector, nb_assets, GET(vector, 0));
    }
    return vector;
}

}  // namespace

// ---------- Json ----------

nlohmann::json load_json_file(const std::string &file_path)
{
    std::ifstream file(file_path);
    if (!file) {
        throw std::runtime_error("cannot open " + file_path);
    }
    return nlohmann::json::parse(file);
}

BlackScholesModel *construct_model(const nlohmann::json &json_input)
{
    int nb_assets = json_input.at("option size").get<int>();
    double interest_rate = json_input.at("interest rate").get<double>();
    double correlation = json_input.at("correlation").get<double>();

    PnlVect *volatility = read_vector(json_input, "volatility", nb_assets);
    PnlVect *spot = read_vector(json_input, "spot", nb_assets);
    PnlVect *trend = read_vector(json_input, "trend", nb_assets);
    BlackScholesModel *model =
        new BlackScholesModel(interest_rate, correlation, volatility, spot, trend);

    // The model stores its own copies (pnl_vect_copy in its constructor),
    // so these temporary vectors can be freed now.
    pnl_vect_free(&volatility);
    pnl_vect_free(&spot);
    pnl_vect_free(&trend);
    return model;
}

Option *construct_option(const nlohmann::json &json_input)
{
    std::string option_type = json_input.at("option type").get<std::string>();
    int nb_assets = json_input.at("option size").get<int>();
    double maturity = json_input.at("maturity").get<double>();
    int nb_fixing_dates = json_input.at("fixing dates number").get<int>();

    PnlVect *payoff_coefficients = read_vector(json_input, "payoff coefficients", nb_assets);

    Option *option = NULL;
    if (option_type == "basket") {
        double strike = json_input.at("strike").get<double>();
        option = new BasketOption(maturity, nb_fixing_dates, nb_assets, payoff_coefficients, strike);
    } else if (option_type == "asian") {
        double strike = json_input.at("strike").get<double>();
        option = new AsianOption(maturity, nb_fixing_dates, nb_assets, payoff_coefficients, strike);
    } else if (option_type == "performance") {
        // No strike for the performance option
        option = new PerformanceOption(maturity, nb_fixing_dates, nb_assets, payoff_coefficients);
    }

    // The option stores its own copy (pnl_vect_copy in its constructor),
    // so this temporary vector can be freed now.
    pnl_vect_free(&payoff_coefficients);

    if (option == NULL) {
        throw std::invalid_argument("unknown option type: " + option_type);
    }
    return option;
}

long read_nb_samples(const nlohmann::json &json_input)
{
    return json_input.at("sample number").get<long>();
}

double read_fd_step(const nlohmann::json &json_input)
{
    return json_input.at("fd step").get<double>();
}

// ---------- Market ----------

PnlMat *load_market(const std::string &file_path)
{
    PnlMat *market = pnl_mat_create_from_file(file_path.c_str());
    if (market == NULL) {
        throw std::runtime_error("cannot read market file " + file_path);
    }
    return market;
}

double time_of_market_line(double maturity, const PnlMat *market, int market_line_t)
{
    int nb_market_steps = market->m - 1;   // H
    if (market_line_t < 0 || market_line_t > nb_market_steps) {
        throw std::invalid_argument("market_line_t must be in [0, H]");
    }
    return market_line_t * maturity / nb_market_steps;
}

PnlMat *construct_past(const PnlMat *market, int nb_fixing_dates, int market_line_t)
{
    int nb_assets = market->n;
    int nb_market_steps = market->m - 1;   // H

    if (nb_fixing_dates <= 0) {
        throw std::invalid_argument("nb_fixing_dates must be > 0");
    }
    if (nb_market_steps % nb_fixing_dates != 0) {
        throw std::invalid_argument("the number of market steps must be a multiple of nb_fixing_dates");
    }
    if (market_line_t < 0 || market_line_t > nb_market_steps) {
        throw std::invalid_argument("market_line_t must be in [0, H]");
    }

    // Number of market rows between two consecutive fixing dates
    int market_rows_per_fixing = nb_market_steps / nb_fixing_dates;

    // Index of the last fixing date <= t (integer division: exact, no tolerance needed)
    int last_fixing_idx = market_line_t / market_rows_per_fixing;

    // t is a fixing date when its market row falls exactly on a fixing row
    bool is_fixing_date = (market_line_t % market_rows_per_fixing == 0);

    int nb_columns = is_fixing_date ? last_fixing_idx + 1 : last_fixing_idx + 2;
    PnlMat *past = pnl_mat_create(nb_assets, nb_columns);

    // Fixing dates already passed
    for (int j = 0; j <= last_fixing_idx; j++) {
        int market_line = j * market_rows_per_fixing;
        for (int i = 0; i < nb_assets; i++) {
            pnl_mat_set(past, i, j, pnl_mat_get(market, market_line, i));
        }
    }

    // Price at t, when t is between two fixing dates
    if (!is_fixing_date) {
        for (int i = 0; i < nb_assets; i++) {
            pnl_mat_set(past, i, last_fixing_idx + 1, pnl_mat_get(market, market_line_t, i));
        }
    }
    return past;
}

// ---------- Random generator ----------

PnlRng *construct_rng(unsigned long seed)
{
    PnlRng *rng = pnl_rng_create(PNL_RNG_MERSENNE);
    pnl_rng_sseed(rng, seed);
    return rng;
}