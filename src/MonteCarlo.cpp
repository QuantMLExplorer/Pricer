#include "MonteCarlo.hpp"

#include <cmath>
#include <stdexcept>

#include "time_grid.hpp"

namespace {

/// Quantile of the standard normal distribution for a two-sided 95% confidence interval.
const double CONFIDENCE_95_QUANTILE = 1.96;

/// Turns the sum and the sum of squares of the payoffs into a discounted price
/// and the half width of its 95% confidence interval.
void discounted_estimate(double sum, double sum_of_squares, long nb_samples, double discount,
                         double &price, double &confidence_half_width)
{
    double nb = (double)nb_samples;
    double mean = sum / nb;
    double variance = sum_of_squares / nb - mean * mean;
    if (variance < 0.0) {
        variance = 0.0;   // rounding errors: avoids the square root of a negative number
    }
    price = discount * mean;
    confidence_half_width = CONFIDENCE_95_QUANTILE * discount * sqrt(variance) / sqrt(nb);
}

/// Checks that past has nb_assets rows and the expected number of columns at time t.
void check_past_size(const PnlMat *past, double t, double maturity, int nb_fixing_dates, int nb_assets)
{
    int last_fixing_idx = time_grid::last_fixing_index(t, maturity, nb_fixing_dates);
    bool is_fixing_date = time_grid::is_fixing_date(t, maturity, nb_fixing_dates);
    int expected_columns = is_fixing_date ? last_fixing_idx + 1 : last_fixing_idx + 2;
    if (past->m != nb_assets || past->n != expected_columns) {
        throw std::invalid_argument(
            "past must be nb_assets x (last_fixing_idx + 1) at a fixing date, "
            "nb_assets x (last_fixing_idx + 2) otherwise");
    }
}

}  // namespace

// ---------- Construction ----------

MonteCarlo::MonteCarlo(const BlackScholesModel &model_, const Option &option_, long nb_samples_)
    : model(model_), option(option_), nb_samples(nb_samples_)
{
    if (nb_samples_ <= 0) {
        throw std::invalid_argument("the number of samples must be > 0");
    }
    if (model_.getD() != option_.getD()) {
        throw std::invalid_argument("model and option must have the same number of assets");
    }
}

// ---------- Price at time 0 ----------

void MonteCarlo::price(PnlRng *rng, double &price, double &confidence_half_width) const
{
    double maturity = option.getT();
    int nb_fixing_dates = option.getN();
    int nb_assets = option.getD();

    PnlMat *path = pnl_mat_create(nb_assets, nb_fixing_dates + 1);
    double sum_of_payoffs = 0.0;
    double sum_of_squared_payoffs = 0.0;

    for (long sample = 0; sample < nb_samples; sample++) {
        model.asset(path, maturity, nb_fixing_dates, rng);
        double payoff = option.payoff(path);
        sum_of_payoffs += payoff;
        sum_of_squared_payoffs += payoff * payoff;
    }

    double discount = exp(-model.getR() * maturity);
    discounted_estimate(sum_of_payoffs, sum_of_squared_payoffs, nb_samples, discount,
                        price, confidence_half_width);
    pnl_mat_free(&path);
}

// ---------- Price at time t ----------

void MonteCarlo::price(const PnlMat *past, double t, PnlRng *rng,
                       double &price, double &confidence_half_width) const
{
    double maturity = option.getT();
    int nb_fixing_dates = option.getN();
    int nb_assets = option.getD();

    if (t < 0.0 || t > maturity) {
        throw std::invalid_argument("t must be in [0, maturity]");
    }
    check_past_size(past, t, maturity, nb_fixing_dates, nb_assets);

    PnlMat *path = pnl_mat_create(nb_assets, nb_fixing_dates + 1);
    double sum_of_payoffs = 0.0;
    double sum_of_squared_payoffs = 0.0;

    for (long sample = 0; sample < nb_samples; sample++) {
        model.asset(path, t, maturity, nb_fixing_dates, rng, past);
        double payoff = option.payoff(path);
        sum_of_payoffs += payoff;
        sum_of_squared_payoffs += payoff * payoff;
    }

    double discount = exp(-model.getR() * (maturity - t));   // e^{-r(T-t)}, not e^{-rT}
    discounted_estimate(sum_of_payoffs, sum_of_squared_payoffs, nb_samples, discount,
                        price, confidence_half_width);
    pnl_mat_free(&path);
}

// ---------- Delta at time t ----------

void MonteCarlo::delta(const PnlMat *past, double t, double h, PnlRng *rng,
                       PnlVect *delta, PnlVect *delta_std_dev) const
{
    double maturity = option.getT();
    int nb_fixing_dates = option.getN();
    int nb_assets = option.getD();

    if (t < 0.0 || t > maturity) {
        throw std::invalid_argument("t must be in [0, maturity]");
    }
    if (h <= 0.0) {
        throw std::invalid_argument("h must be > 0");
    }
    check_past_size(past, t, maturity, nb_fixing_dates, nb_assets);

    PnlMat *path = pnl_mat_create(nb_assets, nb_fixing_dates + 1);
    PnlMat *path_up = pnl_mat_create(nb_assets, nb_fixing_dates + 1);
    PnlMat *path_down = pnl_mat_create(nb_assets, nb_fixing_dates + 1);

    // For each asset i: sum and sum of squares of payoff(path_up) - payoff(path_down)
    PnlVect *sum_of_differences = pnl_vect_create_from_double(nb_assets, 0.0);
    PnlVect *sum_of_squared_differences = pnl_vect_create_from_double(nb_assets, 0.0);

    for (long sample = 0; sample < nb_samples; sample++) {
        // A single simulation per sample: both shifted paths share the same randomness
        model.asset(path, t, maturity, nb_fixing_dates, rng, past);
        for (int i = 0; i < nb_assets; i++) {
            model.shift_asset(path_up, path, i, h, t, maturity, nb_fixing_dates);
            model.shift_asset(path_down, path, i, -h, t, maturity, nb_fixing_dates);
            double difference = option.payoff(path_up) - option.payoff(path_down);
            pnl_vect_set(sum_of_differences, i, pnl_vect_get(sum_of_differences, i) + difference);
            pnl_vect_set(sum_of_squared_differences, i,
                         pnl_vect_get(sum_of_squared_differences, i) + difference * difference);
        }
    }

    pnl_vect_resize(delta, nb_assets);
    pnl_vect_resize(delta_std_dev, nb_assets);

    int price_at_t_col = past->n - 1;   // the last column of past is the price at t
    double discount = exp(-model.getR() * (maturity - t));
    double nb = (double)nb_samples;

    for (int i = 0; i < nb_assets; i++) {
        // delta_i = discount / (2 h s_{t,i}) * mean of (payoff_up - payoff_down)
        double coefficient = discount / (2.0 * h * pnl_mat_get(past, i, price_at_t_col));
        double mean = pnl_vect_get(sum_of_differences, i) / nb;
        double variance = pnl_vect_get(sum_of_squared_differences, i) / nb - mean * mean;
        if (variance < 0.0) {
            variance = 0.0;
        }
        pnl_vect_set(delta, i, coefficient * mean);
        pnl_vect_set(delta_std_dev, i, coefficient * sqrt(variance / nb));
    }

    pnl_vect_free(&sum_of_differences);
    pnl_vect_free(&sum_of_squared_differences);
    pnl_mat_free(&path);
    pnl_mat_free(&path_up);
    pnl_mat_free(&path_down);
}