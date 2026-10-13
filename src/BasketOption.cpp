#include "BasketOption.hpp"

#include <algorithm>
#include <stdexcept>

BasketOption::BasketOption(double maturity_, int nb_fixing_dates_, int nb_assets_,
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

BasketOption::~BasketOption()
{
    pnl_vect_free(&payoff_coefficients);
}

double BasketOption::payoff(const PnlMat *path) const
{
    // Last column of the path: prices at maturity
    double basket_value = 0.0;
    for (int i = 0; i < nb_assets; i++) {
        basket_value += pnl_vect_get(payoff_coefficients, i) * pnl_mat_get(path, i, nb_fixing_dates);
    }
    return std::max(basket_value - strike, 0.0);
}