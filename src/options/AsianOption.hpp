#pragma once

#include "options/Option.hpp"

#include "pnl/pnl_matrix.h"
#include "pnl/pnl_vector.h"

/// Discrete Asian option on a basket:
///   payoff = ( sum_i payoff_coefficients_i * (1 / (N + 1)) * sum_{j=0}^{N} S_{t_j, i}  -  strike )^+
/// The average is taken over the N + 1 fixing dates, the initial date included.
class AsianOption : public Option {

protected:
    PnlVect *payoff_coefficients;   ///< coefficient lambda_i of each asset (size nb_assets)
    double strike;                  ///< strike K

public:
    /// The coefficients are copied, so the caller keeps ownership of its own vector.
    /// @throws std::invalid_argument if payoff_coefficients_ does not have nb_assets_ components,
    ///         or if the base class parameters are invalid.
    AsianOption(double maturity_, int nb_fixing_dates_, int nb_assets_,
                const PnlVect *payoff_coefficients_, double strike_);
    ~AsianOption() override;

    /// Copying is forbidden: the option owns a raw PNL pointer (double free otherwise).
    AsianOption(const AsianOption &) = delete;
    AsianOption &operator=(const AsianOption &) = delete;

    /// @param path   nb_assets x (nb_fixing_dates + 1)
    double payoff(const PnlMat *path) const override;
};