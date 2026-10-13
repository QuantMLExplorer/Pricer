#pragma once

#include "Option.hpp"

#include "pnl/pnl_matrix.h"
#include "pnl/pnl_vector.h"

/// Basket option: it only depends on the prices at maturity T.
///   payoff = ( sum_i payoff_coefficients_i * S_{T, i}  -  strike )^+
/// where i runs over the nb_assets assets.
/// The coefficients may be negative and the strike may be <= 0: with the right signs
/// this covers put options.
class BasketOption : public Option {

protected:
    PnlVect *payoff_coefficients;   ///< coefficient lambda_i of each asset (size nb_assets)
    double strike;                  ///< strike K

public:
    /// The coefficients are copied, so the caller keeps ownership of its own vector.
    /// @throws std::invalid_argument if payoff_coefficients_ does not have nb_assets_ components,
    ///         or if the base class parameters are invalid.
    BasketOption(double maturity_, int nb_fixing_dates_, int nb_assets_,
                 const PnlVect *payoff_coefficients_, double strike_);
    ~BasketOption() override;

    /// Copying is forbidden: the option owns a raw PNL pointer (double free otherwise).
    BasketOption(const BasketOption &) = delete;
    BasketOption &operator=(const BasketOption &) = delete;

    /// @param path   nb_assets x (nb_fixing_dates + 1); only the last column (date T) is used
    double payoff(const PnlMat *path) const override;
};