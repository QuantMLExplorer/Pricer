#include "options/PerformanceOption.hpp"

#include <algorithm>
#include <stdexcept>

PerformanceOption::PerformanceOption(double maturity_, int nb_fixing_dates_, int nb_assets_,
                                     const PnlVect *payoff_coefficients_)
    : Option(maturity_, nb_fixing_dates_, nb_assets_)
{
    // Checked before the allocation: a destructor is not called if a constructor throws
    if (payoff_coefficients_->size != nb_assets_) {
        throw std::invalid_argument("payoff_coefficients must have nb_assets components");
    }
    payoff_coefficients = pnl_vect_copy(payoff_coefficients_);
}

PerformanceOption::~PerformanceOption()
{
    pnl_vect_free(&payoff_coefficients);
}

double PerformanceOption::payoff(const PnlMat *path) const
{
    double payoff_value = 1.0;

    // Basket value at the initial date t_0
    double previous_basket_value = 0.0;
    for (int i = 0; i < nb_assets; i++) {
        previous_basket_value += pnl_vect_get(payoff_coefficients, i) * pnl_mat_get(path, i, 0);
    }

    for (int j = 1; j <= nb_fixing_dates; j++) {
        double basket_value = 0.0;
        for (int i = 0; i < nb_assets; i++) {
            basket_value += pnl_vect_get(payoff_coefficients, i) * pnl_mat_get(path, i, j);
        }
        // Positive part of each term: the performance between two consecutive dates
        payoff_value += std::max(basket_value / previous_basket_value - 1.0, 0.0);
        previous_basket_value = basket_value;
    }
    return payoff_value;
}