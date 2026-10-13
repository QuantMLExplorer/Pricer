#pragma once

#include "pnl/pnl_matrix.h"
#include "pnl/pnl_random.h"

#include "model/BlackScholesModel.hpp"
#include "options/Option.hpp"
#include "pricing/MonteCarlo.hpp"

/// Results of the delta hedging of an option along a market trajectory.
struct HedgingResults {
    double final_pnl;              ///< hedging error: P&L = V_H + delta_H . S_T - payoff
    double initial_price;          ///< price p_0 of the option at time 0
    double initial_price_std_dev;  ///< standard deviation of the Monte Carlo estimator of p_0
};

/// Profit and loss of the delta hedging of an option along a market trajectory.
///
/// At each market date tau_j = j * T / H (j = 0..H), the price p_j and the delta delta_j
/// are computed with the past observed up to tau_j, and the portfolio is rebalanced.
/// The part invested at the risk-free rate evolves as:
///   V_0 = p_0 - delta_0 . S_0
///   V_j = V_{j-1} * exp(r T / H) - (delta_j - delta_{j-1}) . S_{tau_j}     for j = 1..H
/// and the hedging error is:
///   P&L = V_H + delta_H . S_T - payoff
///
/// The fixing dates must be a subset of the market dates (H is a multiple of N).
///
/// @param model         model used to price and to read the interest rate
/// @param option        option to hedge
/// @param monte_carlo   pricer used at each market date (built from the same model and option)
/// @param market        (H + 1) x nb_assets: one row = one market date, one column = one asset
/// @param finite_difference_step   relative step used to estimate the deltas
/// @param rng           random number generator
/// @throws std::invalid_argument if H is not a multiple of the number of fixing dates
HedgingResults compute_pnl(const BlackScholesModel &model, const Option &option,
                           const MonteCarlo &monte_carlo, const PnlMat *market,
                           double finite_difference_step, PnlRng *rng);