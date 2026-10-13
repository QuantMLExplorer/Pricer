#pragma once

/// Result of a Monte Carlo pricing.
struct PricingResults {
    double price;           ///< price of the option
    double price_std_dev;   ///< standard deviation of the Monte Carlo estimator of the price
                            ///< (the 95% confidence interval is price +/- 1.96 * price_std_dev)
};