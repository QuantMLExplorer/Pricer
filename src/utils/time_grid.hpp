#pragma once

#include <cmath>

/// Helpers to locate a time t on the grid of fixing dates t_j = j * maturity / nb_fixing_dates.
/// Header-only, shared by every class that needs it so that they all classify t identically.
namespace time_grid {

/// Relative tolerance, in number of time steps, absorbing floating point rounding errors
/// (e.g. t / time_step = 11.999999999999998 instead of 12).
const double TOLERANCE = 1e-9;

/// @return the index of the last fixing date <= t, capped at nb_fixing_dates.
inline int last_fixing_index(double t, double maturity, int nb_fixing_dates)
{
    double time_step = maturity / nb_fixing_dates;
    int index = (int)std::floor(t / time_step + TOLERANCE);
    return index > nb_fixing_dates ? nb_fixing_dates : index;
}

/// @return true if t is (up to the tolerance) one of the fixing dates.
inline bool is_fixing_date(double t, double maturity, int nb_fixing_dates)
{
    double time_step = maturity / nb_fixing_dates;
    int index = last_fixing_index(t, maturity, nb_fixing_dates);
    return std::fabs(t - index * time_step) < TOLERANCE * time_step;
}

}  // namespace time_grid