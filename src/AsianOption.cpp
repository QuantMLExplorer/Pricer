#include "AsianOption.hpp"

#include <algorithm>
#include <stdexcept>

AsianOption::AsianOption(double maturity_, int nb_fixing_dates_, int nb_assets_,
                         const PnlVect *payoff_coefficients_, double strike_)
    : Option(maturity_, nb_fixing_dates_, nb_assets_)
{
    // Checked before the allocation: a destructor is not called if a constructor throws
    if (payoff_coefficients_->size != nb_assets_) {
        throw std::invalid_argument("payoff_coefficients must have nb_assets components");
    }
    payoff_coefficients = pnl_vect_copy(payoff_coefficients_);
    strike = strike_;
}

AsianOption::~AsianOption()
{
    pnl_vect_free(&payoff_coefficients);
}

double AsianOption::payoff(const PnlMat *path) const
{
    double weighted_average_sum = 0.0;
    for (int i = 0; i < nb_assets; i++) {
        // Average of asset i over the N + 1 fixing dates
        double average_price = 0.0;
        for (int j = 0; j <= nb_fixing_dates; j++) {
            average_price += pnl_mat_get(path, i, j);
        }
        average_price /= (nb_fixing_dates + 1);
        weighted_average_sum += pnl_vect_get(payoff_coefficients, i) * average_price;
    }
    return std::max(weighted_average_sum - strike, 0.0);
}