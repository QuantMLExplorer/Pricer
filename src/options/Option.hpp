#pragma once

#include "pnl/pnl_matrix.h"

/// Abstract base class of the options: payoff phi(S_{t_0}, ..., S_{t_N}) of a path
/// observed on the regular grid of fixing dates t_j = j * maturity / nb_fixing_dates.
///
/// Path convention: a path is a nb_assets x (nb_fixing_dates + 1) matrix,
/// row i = asset, column j = fixing date (column 0 is the initial price).
class Option {

protected:
    double maturity;        ///< maturity T
    int nb_fixing_dates;    ///< number of fixing dates N (the path has N + 1 columns)
    int nb_assets;          ///< number of underlying assets D

public:
    /// @throws std::invalid_argument if maturity <= 0, nb_fixing_dates < 1 or nb_assets < 1.
    Option(double maturity_, int nb_fixing_dates_, int nb_assets_);

    /// Virtual: options are destroyed through a pointer to this base class.
    virtual ~Option() {}

    /// Payoff of the option on a path.
    /// @param path   nb_assets x (nb_fixing_dates + 1), see the path convention above
    virtual double payoff(const PnlMat *path) const = 0;

    /// @return the maturity T
    double getT() const;
    /// @return the number of fixing dates N
    int getN() const;
    /// @return the number of assets D
    int getD() const;
};