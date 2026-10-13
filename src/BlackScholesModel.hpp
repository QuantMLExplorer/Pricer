#pragma once

#include "pnl/pnl_vector.h"
#include "pnl/pnl_matrix.h"
#include "pnl/pnl_random.h"

/// Multi-dimensional Black-Scholes model with constant correlation.
///
/// For asset i:  S_{t,i} = S_{0,i} exp((drift_i - volatility_i^2 / 2) t + volatility_i (L W_t)_i)
/// where the drift is the interest rate under the risk-neutral probability
/// and the trend under the historical probability, and L is the Cholesky
/// factor of the correlation matrix (1 on the diagonal, `correlation` elsewhere).
///
/// Matrix conventions used by the simulation methods:
///   - path   : nb_assets x (nb_fixing_dates + 1), row i = asset, column j = fixing date;
///   - market : (nb_market_steps + 1) x nb_assets, row = market date, column = asset.
class BlackScholesModel {

private:
    double interest_rate;        ///< risk-free rate r
    double correlation;          ///< correlation rho between any two distinct assets
    int nb_assets;               ///< number of assets D
    PnlVect *volatility;         ///< volatility of each asset (size D)
    PnlVect *spot;               ///< initial price of each asset (size D)
    PnlVect *trend;              ///< drift mu of each asset under the historical probability (size D)
    PnlMat *cholesky_factor;     ///< lower triangular L such that L L' = correlation matrix

    /// Frees every PNL object owned by the model (safe on NULL pointers).
    void free_all();

public:
    /// Builds the model. The vectors are copied, so the caller keeps ownership of its own.
    /// @throws std::invalid_argument if the vectors have different sizes or if
    ///         correlation is not in ]-1/(D-1), 1[.
    BlackScholesModel(double interest_rate_, double correlation_, const PnlVect *volatility_,
                      const PnlVect *spot_, const PnlVect *trend_);
    ~BlackScholesModel();

    /// Copying is forbidden: the model owns raw PNL pointers (double free otherwise).
    BlackScholesModel(const BlackScholesModel &) = delete;
    BlackScholesModel &operator=(const BlackScholesModel &) = delete;

    /// @return the risk-free rate r
    double getR() const;
    /// @return the number of assets D
    int getD() const;

    /// Simulates one risk-neutral path from time 0 on the grid t_j = j * maturity / nb_fixing_dates.
    /// @param path              output, nb_assets x (nb_fixing_dates + 1)
    /// @param maturity          maturity T
    /// @param nb_fixing_dates   number of fixing dates N
    /// @param rng               random number generator
    void asset(PnlMat *path, double maturity, int nb_fixing_dates, PnlRng *rng) const;

    /// Simulates one risk-neutral path from time t, given the observed past.
    /// @param path              output, nb_assets x (nb_fixing_dates + 1)
    /// @param t                 current time, in [0, maturity]
    /// @param maturity          maturity T
    /// @param nb_fixing_dates   number of fixing dates N
    /// @param rng               random number generator
    /// @param past              observed prices, nb_assets rows: columns 0..last_fixing_idx hold the
    ///                          fixing dates already passed, and the LAST column is always the price
    ///                          at t (an extra column if t is not a fixing date).
    void asset(PnlMat *path, double t, double maturity, int nb_fixing_dates,
               PnlRng *rng, const PnlMat *past) const;

    /// Copies `path` into `shifted`, multiplying asset i by (1 + h) from time t onwards.
    /// If t is a fixing date, its column is shifted too (it holds s_t).
    /// Used for the finite difference estimation of the delta.
    /// @param i   index of the shifted asset
    /// @param h   relative shift (use -h for the downward shift)
    void shift_asset(PnlMat *shifted, const PnlMat *path, int i, double h,
                     double t, double maturity, int nb_fixing_dates) const;

    /// Simulates a market under the historical probability (drift = trend)
    /// on a grid of nb_market_steps steps of size maturity / nb_market_steps.
    /// @param market   output, resized to (nb_market_steps + 1) x nb_assets
    void simul_market(PnlMat *market, double maturity, int nb_market_steps, PnlRng *rng) const;
};