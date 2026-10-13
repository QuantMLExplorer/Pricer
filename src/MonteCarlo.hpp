#pragma once

#include "pnl/pnl_vector.h"
#include "pnl/pnl_matrix.h"
#include "pnl/pnl_random.h"

#include "BlackScholesModel.hpp"
#include "Option.hpp"

/// Monte Carlo pricer: price and delta of an option in a Black-Scholes model.
///
/// The class does not own the model nor the option, which must outlive it.
/// All the confidence intervals are at the 95% level.
class MonteCarlo {

private:
    const BlackScholesModel &model;   ///< model used to simulate the assets
    const Option &option;             ///< option to price
    long nb_samples;                  ///< number of Monte Carlo samples M

public:
    /// @throws std::invalid_argument if nb_samples_ <= 0 or if model and option
    ///         do not have the same number of assets.
    MonteCarlo(const BlackScholesModel &model_, const Option &option_, long nb_samples_);

    /// References cannot be rebound: copying is forbidden.
    MonteCarlo(const MonteCarlo &) = delete;
    MonteCarlo &operator=(const MonteCarlo &) = delete;

    /// Price at time 0.
    /// @param price                    output, price of the option
    /// @param confidence_half_width    output, half width of the 95% confidence interval
    ///                                 (the interval is [price - half_width, price + half_width])
    void price(PnlRng *rng, double &price, double &confidence_half_width) const;

    /// Price at time t, knowing the observed past.
    /// @param past   observed prices, nb_assets x (last_fixing_idx + 1) if t is a fixing date,
    ///               nb_assets x (last_fixing_idx + 2) otherwise. The last column is the price at t.
    /// @param t      current time, in [0, maturity]
    /// @throws std::invalid_argument if t is out of range or past has the wrong size.
    void price(const PnlMat *past, double t, PnlRng *rng,
               double &price, double &confidence_half_width) const;

    /// Delta at time t: derivative of the price with respect to each asset price at t,
    /// estimated by centred finite differences with relative step h.
    /// Both shifted paths come from the same simulation
    /// @param past            same convention as in price
    /// @param h               finite difference step, > 0
    /// @param delta           output, resized to nb_assets
    /// @param delta_std_dev   output, resized to nb_assets: standard deviation of each estimator
    void delta(const PnlMat *past, double t, double h, PnlRng *rng,
               PnlVect *delta, PnlVect *delta_std_dev) const;
};