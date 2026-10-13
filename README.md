# Monte Carlo pricer and delta hedging

C++ tool that prices options by Monte Carlo in a multi-dimensional Black-Scholes model,
computes their delta by finite differences, and measures the profit and loss (P&L) of the
delta hedging along a market trajectory.

Supported products:

| Key (`option type`) | Product | Payoff |
|---|---|---|
| `basket` | Basket option | `( sum_i lambda_i * S_{T,i} - K )^+` |
| `asian` | Discrete Asian option | `( sum_i lambda_i * (1/(N+1)) * sum_{j=0..N} S_{t_j,i} - K )^+` |
| `performance` | Performance option on a basket | `1 + sum_{j=1..N} ( basket(t_j) / basket(t_{j-1}) - 1 )^+` |

`basket(t_j) = sum_i lambda_i * S_{t_j,i}`. The performance option has no strike.

## Requirements

- A C++11 compiler and CMake >= 3.11
- [PNL](https://github.com/pnlnum/pnl), compiled and installed (see below)
- [nlohmann/json](https://github.com/nlohmann/json): found on the system
  (`sudo apt install nlohmann-json3-dev`), otherwise downloaded by CMake

### Install PNL

```bash
sudo apt install build-essential cmake liblapack-dev libblas-dev
git clone https://github.com/pnlnum/pnl.git
cd pnl && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DPNL_INSTALL_PREFIX=$(pwd)/../../pnl-install ..
make -j4 && make install
```

## Build

From the root of the project:

```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=$(pwd)/../pnl-install -DCMAKE_BUILD_TYPE=Release ..
make
export LD_LIBRARY_PATH=$(pwd)/../pnl-install/lib:$LD_LIBRARY_PATH
```

## Usage

| Program | Command | Output |
|---|---|---|
| `test_price0` | `./test_price0 file.json` | price at time 0 and its standard deviation |
| `test_pricet` | `./test_pricet file.json market.txt market_line_t` | price and delta at time t, with standard deviations |
| `test_hedge` | `./test_hedge file.json market.txt` | final P&L, initial price and its standard deviation |
| `test_simul_market` | `./test_simul_market file.json` | check of the market simulation (`[OK]` / `[ECHEC]`) |
| `test_json_reader` | `./test_json_reader file.json` | provided parser test |

Examples:

```bash
./test_price0 ../data/asian/asian.json
./test_pricet ../data/asian/asian.json ../data/asian/asian_market.txt 180
./test_hedge  ../data/asian/asian.json ../data/asian/asian_market.txt
```

`market_line_t` is a row of the market file: the current time is `t = market_line_t * T / H`.
Row 0 is time 0 and row `H` is the maturity.

## Input files

### Parameter file (json)

| Key | Type | Meaning |
|---|---|---|
| `option type` | string | `basket`, `asian` or `performance` |
| `option size` | int | number of assets D |
| `spot` | vector | initial prices |
| `maturity` | float | maturity T |
| `volatility` | vector | volatilities |
| `interest rate` | float | risk-free rate r |
| `correlation` | float | correlation rho between any two assets, in `]-1/(D-1), 1[` |
| `trend` | vector | drift mu of each asset under the historical probability |
| `strike` | float | strike K (not used by `performance`) |
| `payoff coefficients` | vector | coefficients lambda |
| `fixing dates number` | int | number of fixing dates N |
| `hedging dates number` | int | number of market dates H (a multiple of N) |
| `sample number` | long | number of Monte Carlo samples M |
| `fd step` | float | relative step h of the finite differences |

A vector key may contain a single value: it is then used for every asset.

### Market file (txt)

`(H + 1)` rows and `D` columns, one row per market date and one column per asset. Lines starting
with `#` are comments. `H` must be a multiple of `N`, so that every fixing date is a market date.

## Conventions

| Object | Shape | Rows / columns |
|---|---|---|
| `path` (simulated path) | `D x (N + 1)` | row `i` = asset, column `j` = fixing date |
| `market` | `(H + 1) x D` | row = market date, column = asset |
| `past` | `D x (last_fixing_idx + 1)` or `D x (last_fixing_idx + 2)` | fixing dates already passed; the **last column is always the price at t** |

In the code, `i` indexes the assets and `j` indexes the dates.

## Method

- **Simulation.** `S_{t_{j+1}, i} = S_{t_j, i} exp((r - sigma_i^2 / 2) dt + sigma_i sqrt(dt) (L G)_i)`, where `L`
  is the Cholesky factor of the correlation matrix and `G` a standard gaussian vector.
  The market is simulated with the same formula, with the trend `mu` instead of `r`.
- **Price at time t.** The future is simulated from the observed price `s_t`, since
  `S_{t+u} = s_t * S~_u`. The first step lasts until the next fixing date, and the result is
  discounted by `exp(-r (T - t))`.
- **Delta.** Centred finite differences with relative step `h`. For each sample, a single path is
  simulated and the shifted paths `(1 + h)` and `(1 - h)` are built from it, so they share the same
  randomness.
- **Hedging.** At each market date, the price and the delta are computed with the observed past and
  the portfolio is rebalanced:
  `V_0 = p_0 - delta_0 . S_0`, `V_j = V_{j-1} exp(r T / H) - (delta_j - delta_{j-1}) . S_j`,
  `P&L = V_H + delta_H . S_T - payoff`.
- **Confidence interval.** `price +/- 1.96 * priceStdDev` (95%).

## Design choices

- `Option` is an abstract class (pure virtual `payoff`, virtual destructor): `MonteCarlo` is written
  once and works with every product.
- `BlackScholesModel`, `BasketOption`, `AsianOption` and `PerformanceOption` own PNL pointers: their
  copy is deleted (`= delete`) to prevent double frees.
- `MonteCarlo` stores references to the model and the option, which must outlive it.
- A time is located on the fixing grid with a relative tolerance (`time_grid::TOLERANCE = 1e-9`),
  shared by every file, to absorb floating point rounding errors.
- The random generator is created with a fixed seed in the tests, so the results are reproducible.


