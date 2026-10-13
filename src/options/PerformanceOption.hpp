#pragma once

#include "options/Option.hpp"

#include "pnl/pnl_matrix.h"
#include "pnl/pnl_vector.h"

/// Performance option on a basket (no strike):
///   payoff = 1 + sum_{j=1}^{N} ( basket(t_j) / basket(t_{j-1}) - 1 )^+
/// with basket(t_j) = sum_i payoff_coefficients_i * S_{t_j, i} and N = nb_fixing_dates.
/// The positive part applies to EACH term of the sum, not to the whole sum.
class PerformanceOption : public Option {

protected:
    PnlVect *payoff_coefficients;   ///< coefficient lambda_i of each asset (size nb_assets)

public:
    /// The coefficients are copied, so the caller keeps ownership of its own vector.
    /// @throws std::invalid_argument if payoff_coefficients_ does not have nb_assets_ components,
    ///         or if the base class parameters are invalid.
    PerformanceOption(double maturity_, int nb_fixing_dates_, int nb_assets_,
                      const PnlVect *payoff_coefficients_);
    ~PerformanceOption() override;

    /// Copying is forbidden: the option owns a raw PNL pointer (double free otherwise).
    PerformanceOption(const PerformanceOption &) = delete;
    PerformanceOption &operator=(const PerformanceOption &) = delete;

    /// The basket value at each fixing date is computed once and reused as the
    /// "previous" value at the next date. It must stay non-zero (division).
    /// @param path   nb_assets x (nb_fixing_dates + 1)
    double payoff(const PnlMat *path) const override;
};