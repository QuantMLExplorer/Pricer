#pragma once

#include <string>
#include <nlohmann/json.hpp>

#include "pnl/pnl_vector.h"
#include "pnl/pnl_matrix.h"
#include "pnl/pnl_random.h"

#include "BlackScholesModel.hpp"
#include "Option.hpp"

/// Helpers that build the objects of the pricer from a json parameter file and a market file.
/// Each function reads only the json keys it needs.
///
/// Json convention: a vector key ("volatility", "spot", "trend", "payoff coefficients") may
/// contain a single value, which is then used for every asset.
///
/// Market convention: (H + 1) rows x nb_assets columns, one row = one market date
/// tau_j = j * T / H, one column = one asset.

// ---------- Json ----------

/// Opens and parses a json file.
/// @throws std::runtime_error if the file cannot be opened.
nlohmann::json load_json_file(const std::string &file_path);

/// Builds the Black-Scholes model.
/// Reads "option size", "interest rate", "correlation", "volatility", "spot" and "trend".
/// The caller must delete the returned pointer.
BlackScholesModel *construct_model(const nlohmann::json &json_input);

/// Builds the option according to "option type" ("basket", "asian" or "performance").
/// Reads "option type", "option size", "maturity", "fixing dates number",
/// "payoff coefficients" and, except for the performance option, "strike".
/// The caller must delete the returned pointer.
/// @throws std::invalid_argument if the option type is unknown.
Option *construct_option(const nlohmann::json &json_input);

/// Reads "sample number": the number of Monte Carlo samples.
long read_nb_samples(const nlohmann::json &json_input);

/// Reads "fd step": the relative step of the finite differences used for the delta.
double read_fd_step(const nlohmann::json &json_input);

// ---------- Market ----------

/// Reads a market file.
/// The caller must free the returned matrix with pnl_mat_free.
/// @throws std::runtime_error if the file cannot be read.
PnlMat *load_market(const std::string &file_path);

/// Time t of a market row: t = market_line_t * maturity / H, with H = number of market steps.
/// @throws std::invalid_argument if market_line_t is not in [0, H].
double time_of_market_line(double maturity, const PnlMat *market, int market_line_t);

/// Builds the observed past at the time of a market row, as expected by MonteCarlo.
/// Rows = assets. Columns = prices at the fixing dates already passed, plus one extra column
/// with the price at t if t is not a fixing date: nb_assets x (last_fixing_idx + 1) at a fixing
/// date, nb_assets x (last_fixing_idx + 2) otherwise. The LAST column is always the price at t.
/// The caller must free the returned matrix with pnl_mat_free.
/// @throws std::invalid_argument if nb_fixing_dates <= 0, if H is not a multiple of
///         nb_fixing_dates, or if market_line_t is not in [0, H].
PnlMat *construct_past(const PnlMat *market, int nb_fixing_dates, int market_line_t);

// ---------- Random generator ----------

/// Creates a Mersenne Twister generator initialised with the given seed.
/// The caller must free it with pnl_rng_free.
PnlRng *construct_rng(unsigned long seed);