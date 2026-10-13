#include "model/BlackScholesModel.hpp"
#include "utils/time_grid.hpp"

#include <cmath>
#include <stdexcept>

// ---------- Construction / destruction ----------

BlackScholesModel::BlackScholesModel(double interest_rate_, double correlation_,
                                     const PnlVect *volatility_, const PnlVect *spot_,
                                     const PnlVect *trend_)
{
    interest_rate = interest_rate_;
    correlation = correlation_;
    nb_assets = volatility_->size;
    volatility = NULL;
    spot = NULL;
    trend = NULL;
    cholesky_factor = NULL;

    if (spot_->size != nb_assets || trend_->size != nb_assets) {
        throw std::invalid_argument("volatility, spot and trend must have the same size");
    }
    if (nb_assets > 1 && !(correlation > -1.0 / (nb_assets - 1) && correlation < 1.0)) {
        throw std::invalid_argument("correlation must be in ]-1/(D-1), 1[");
    }

    volatility = pnl_vect_copy(volatility_);
    spot = pnl_vect_copy(spot_);
    trend = pnl_vect_copy(trend_);

    // Correlation matrix (1 on the diagonal, rho elsewhere), factorised in place
    cholesky_factor = pnl_mat_create_from_double(nb_assets, nb_assets, correlation);
    pnl_mat_set_diag(cholesky_factor, 1.0, 0);
    if (pnl_mat_chol(cholesky_factor) != 0) {
        free_all();
        throw std::invalid_argument("Cholesky factorisation failed");
    }
}

BlackScholesModel::~BlackScholesModel()
{
    free_all();
}

void BlackScholesModel::free_all()
{
    pnl_vect_free(&volatility);
    pnl_vect_free(&spot);
    pnl_vect_free(&trend);
    pnl_mat_free(&cholesky_factor);
}

double BlackScholesModel::getR() const
{
    return interest_rate;
}

int BlackScholesModel::getD() const
{
    return nb_assets;
}

// ---------- Risk-neutral simulation from time 0 ----------

void BlackScholesModel::asset(PnlMat *path, double maturity, int nb_fixing_dates, PnlRng *rng) const
{
    double time_step = maturity / nb_fixing_dates;
    PnlVect *gaussian = pnl_vect_create(nb_assets);

    // Column 0: initial prices
    for (int i = 0; i < nb_assets; i++) {
        pnl_mat_set(path, i, 0, pnl_vect_get(spot, i));
    }

    // Columns 1..N: one gaussian vector per date, shared by all assets
    for (int j = 1; j <= nb_fixing_dates; j++) {
        pnl_vect_rng_normal_d(gaussian, nb_assets, rng);
        for (int i = 0; i < nb_assets; i++) {
            double volatility_i = pnl_vect_get(volatility, i);
            double previous_price = pnl_mat_get(path, i, j - 1);
            PnlVect cholesky_row = pnl_vect_wrap_mat_row(cholesky_factor, i);
            double correlated_gaussian = pnl_vect_scalar_prod(&cholesky_row, gaussian);
            pnl_mat_set(path, i, j,
                        previous_price * exp((interest_rate - volatility_i * volatility_i / 2) * time_step
                                             + volatility_i * sqrt(time_step) * correlated_gaussian));
        }
    }
    pnl_vect_free(&gaussian);
}

// ---------- Risk-neutral simulation from time t ----------

void BlackScholesModel::asset(PnlMat *path, double t, double maturity, int nb_fixing_dates,
                              PnlRng *rng, const PnlMat *past) const
{
    double time_step = maturity / nb_fixing_dates;
    int last_fixing_idx = time_grid::last_fixing_index(t, maturity, nb_fixing_dates);

    // Fixing dates already passed: copied from the observations
    for (int j = 0; j <= last_fixing_idx; j++) {
        for (int i = 0; i < nb_assets; i++) {
            pnl_mat_set(path, i, j, pnl_mat_get(past, i, j));
        }
    }
    if (last_fixing_idx == nb_fixing_dates) {
        return;   // t = T: the whole path is known
    }

    // Duration of the first simulated step: from t to the next fixing date
    double first_step = (last_fixing_idx + 1) * time_step - t;
    int price_at_t_col = past->n - 1;   // the last column of past is the price at t
    PnlVect *gaussian = pnl_vect_create(nb_assets);

    for (int j = last_fixing_idx + 1; j <= nb_fixing_dates; j++) {
        bool is_first_step = (j == last_fixing_idx + 1);
        double step = is_first_step ? first_step : time_step;
        pnl_vect_rng_normal_d(gaussian, nb_assets, rng);
        for (int i = 0; i < nb_assets; i++) {
            double volatility_i = pnl_vect_get(volatility, i);
            double previous_price = is_first_step ? pnl_mat_get(past, i, price_at_t_col)
                                                  : pnl_mat_get(path, i, j - 1);
            PnlVect cholesky_row = pnl_vect_wrap_mat_row(cholesky_factor, i);
            double correlated_gaussian = pnl_vect_scalar_prod(&cholesky_row, gaussian);
            pnl_mat_set(path, i, j,
                        previous_price * exp((interest_rate - volatility_i * volatility_i / 2) * step
                                             + volatility_i * sqrt(step) * correlated_gaussian));
        }
    }
    pnl_vect_free(&gaussian);
}

// ---------- Shifted path for the delta ----------

void BlackScholesModel::shift_asset(PnlMat *shifted, const PnlMat *path, int i, double h,
                                    double t, double maturity, int nb_fixing_dates) const
{
    double time_step = maturity / nb_fixing_dates;
    int last_fixing_idx = time_grid::last_fixing_index(t, maturity, nb_fixing_dates);
    bool is_fixing_date = fabs(t - last_fixing_idx * time_step) < time_grid::TOLERANCE * time_step;

    // If t is a fixing date, its column holds s_t and must be shifted too;
    // otherwise the shift starts at the next fixing date.
    int first_shifted_col = is_fixing_date ? last_fixing_idx : last_fixing_idx + 1;

    pnl_mat_clone(shifted, path);
    for (int j = first_shifted_col; j <= nb_fixing_dates; j++) {
        pnl_mat_set(shifted, i, j, pnl_mat_get(path, i, j) * (1.0 + h));
    }
}

// ---------- Market simulation (historical probability) ----------

void BlackScholesModel::simul_market(PnlMat *market, double maturity, int nb_market_steps,
                                     PnlRng *rng) const
{
    double time_step = maturity / nb_market_steps;
    pnl_mat_resize(market, nb_market_steps + 1, nb_assets);
    PnlVect *gaussian = pnl_vect_create(nb_assets);

    // Row 0: initial prices
    for (int i = 0; i < nb_assets; i++) {
        pnl_mat_set(market, 0, i, pnl_vect_get(spot, i));
    }

    // Rows 1..H: the drift is the trend, not the interest rate
    for (int j = 1; j <= nb_market_steps; j++) {
        pnl_vect_rng_normal_d(gaussian, nb_assets, rng);
        for (int i = 0; i < nb_assets; i++) {
            double volatility_i = pnl_vect_get(volatility, i);
            double trend_i = pnl_vect_get(trend, i);
            double previous_price = pnl_mat_get(market, j - 1, i);
            PnlVect cholesky_row = pnl_vect_wrap_mat_row(cholesky_factor, i);
            double correlated_gaussian = pnl_vect_scalar_prod(&cholesky_row, gaussian);
            pnl_mat_set(market, j, i,
                        previous_price * exp((trend_i - volatility_i * volatility_i / 2) * time_step
                                             + volatility_i * sqrt(time_step) * correlated_gaussian));
        }
    }
    pnl_vect_free(&gaussian);
}