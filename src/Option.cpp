#include "Option.hpp"

#include <stdexcept>

Option::Option(double maturity_, int nb_fixing_dates_, int nb_assets_)
{
    if (maturity_ <= 0.0) {
        throw std::invalid_argument("the maturity must be > 0");
    }
    if (nb_fixing_dates_ < 1) {
        throw std::invalid_argument("the number of fixing dates must be >= 1");
    }
    if (nb_assets_ < 1) {
        throw std::invalid_argument("the number of assets must be >= 1");
    }

    maturity = maturity_;
    nb_fixing_dates = nb_fixing_dates_;
    nb_assets = nb_assets_;
}

double Option::getT() const
{
    return maturity;
}

int Option::getN() const
{
    return nb_fixing_dates;
}

int Option::getD() const
{
    return nb_assets;
}