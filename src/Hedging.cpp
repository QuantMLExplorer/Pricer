#include "Hedging.hpp"

#include <cmath>

#include "pricer_setup.hpp"   // construct_past

namespace {

/// Quantile of the standard normal distribution for a two-sided 95% confidence interval
/// (same value as in MonteCarlo.cpp): converts a half width into a standard deviation.
const double CONFIDENCE_95_QUANTILE = 1.96;

}  

HedgingResults compute_pnl(const BlackScholesModel &model, const Option &option,
                           const MonteCarlo &monte_carlo, const PnlMat *market,
                           double finite_difference_step, PnlRng *rng)
{
    int nb_assets = option.getD();
    int nb_fixing_dates = option.getN();
    double maturity = option.getT();
    int nb_market_steps = market->m - 1;   // H

    // Growth factor of the risk-free part between two consecutive market dates
    double growth_factor = exp(model.getR() * maturity / nb_market_steps);

    PnlVect *delta = pnl_vect_create(nb_assets);
    PnlVect *delta_std_dev = pnl_vect_create(nb_assets);   // required by delta(), not used here
    PnlVect *previous_delta = pnl_vect_create(nb_assets);
    PnlVect *delta_change = pnl_vect_create(nb_assets);

    double risk_free_amount = 0.0;   // V_j: part invested at the risk-free rate
    HedgingResults results;

    for (int j = 0; j <= nb_market_steps; j++) {
        double current_time = j * maturity / nb_market_steps;
        PnlMat *past = construct_past(market, nb_fixing_dates, j);

        double price, confidence_half_width;
        monte_carlo.price(past, current_time, rng, price, confidence_half_width);
        monte_carlo.delta(past, current_time, finite_difference_step, rng, delta, delta_std_dev);

        // Asset prices at the current time: row j of the market (a view, never freed)
        PnlVect asset_prices = pnl_vect_wrap_mat_row(market, j);

        if (j == 0) {
            // V_0 = p_0 - delta_0 . S_0
            risk_free_amount = price - pnl_vect_scalar_prod(delta, &asset_prices);
            results.initial_price = price;
            results.initial_price_std_dev = confidence_half_width / CONFIDENCE_95_QUANTILE;
        } else {
            // V_j = V_{j-1} * growth - (delta_j - delta_{j-1}) . S_{tau_j}
            pnl_vect_clone(delta_change, delta);
            pnl_vect_minus_vect(delta_change, previous_delta);
            risk_free_amount = risk_free_amount * growth_factor
                               - pnl_vect_scalar_prod(delta_change, &asset_prices);
        }
        pnl_vect_clone(previous_delta, delta);

        // Last date: past is the whole realised path, so it gives the payoff
        if (j == nb_market_steps) {
            results.final_pnl = risk_free_amount
                                + pnl_vect_scalar_prod(delta, &asset_prices)
                                - option.payoff(past);
        }
        pnl_mat_free(&past);
    }

    pnl_vect_free(&delta);
    pnl_vect_free(&delta_std_dev);
    pnl_vect_free(&previous_delta);
    pnl_vect_free(&delta_change);
    return results;
}