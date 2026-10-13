# Monte Carlo pricer and delta hedging

C++ implementation of a Monte Carlo pricer for path-dependent options in a multi-dimensional
Black-Scholes model. The tool computes prices (at time 0 and at any later time), deltas by finite
differences, and the profit and loss (P&L) of the delta hedging along a market trajectory.

## Problem

The payoff of the options only depends on the prices on a regular grid of fixing dates
$0 = t_0 < t_1 < \dots < t_N = T$, with $t_k = kT/N$:

$$
\varphi(S_{t_0}, S_{t_1}, \dots, S_{t_N})
$$

For a constant interest rate $r$, the price at time $t$, with $t_i \le t < t_{i+1}$, is

$$
v(t, S_{t_0}, \dots, S_{t_i}, S_t) = e^{-r(T-t)} \mathbb{E}\big[\varphi(S_{t_0}, \dots, S_{t_N}) \mid \mathcal{F}_t\big]
$$

and it is computed by Monte Carlo. The delta of the option, the derivative of the price with respect
to each asset price at time $t$, gives the quantity of each asset to hold in the hedging portfolio.

## Model

$D$ assets follow correlated Black-Scholes dynamics:

$$
S_{t,d} = S_{0,d} \exp\Big(\big(r - \tfrac{\sigma_d^2}{2}\big) t + \sigma_d B_{t,d}\Big), \qquad d = 1, \dots, D
$$

where $B = L W$, $W$ is a standard Brownian motion in dimension $D$ and $L$ is the lower triangular
Cholesky factor of the correlation matrix $\Gamma = L L^{T}$, with $\Gamma_{ii} = 1$ and
$\Gamma_{ij} = \rho$ for $i \ne j$. The matrix $\Gamma$ is positive definite if and only if
$\rho \in \left]-\frac{1}{D-1}, 1\right[$, and the constructor checks it.

On the grid of fixing dates, the simulation uses a standard gaussian vector $G_{j+1}$ for each date,
shared by all the assets :

$$
S_{t_{j+1},d} = S_{t_j,d} \exp\Big(\big(r - \tfrac{\sigma_d^2}{2}\big)\Delta t + \sigma_d \sqrt{\Delta t} L_d G_{j+1}\Big), \qquad \Delta t = t_{j+1} - t_j
$$

where $L_d$ is the row $d$ of $L$.

**Historical probability.** To simulate market data, the drift $r$ is replaced by the trend
$\mu_d$ of each asset:

$$
S_{t,d} = S_{0,d} \exp\Big(\big(\mu_d - \tfrac{\sigma_d^2}{2}\big) t + \sigma_d L_d W_t\Big)
$$

## Supported products

| Key (`option type`) | Product | Payoff |
|---|---|---|
| `basket` | Basket option | $\Big(\sum_{d=1}^{D} \lambda_d S_{T,d} - K\Big)^+$ |
| `asian` | Discrete Asian option | $\Big(\sum_{d=1}^{D} \lambda_d \frac{1}{N+1} \sum_{i=0}^{N} S_{t_i,d} - K\Big)^+$ |
| `performance` | Performance option on a basket | $1 + \sum_{i=1}^{N}\Big(\frac{B(t_i)}{B(t_{i-1})} - 1\Big)^+$ |

where $B(t_i) = \sum_{d=1}^{D} \lambda_d S_{t_i,d}$ is the value of the basket at $t_i$.

- For the basket option, $\lambda_d \in \mathbb{R}$ and $K \in \mathbb{R}$: the case $K \le 0$ gives a put.
- The performance option has no strike, and the positive part applies to **each term** of the sum.

## Methods

### Price at time 0

With $M$ independent paths $(S^{(j)}_{t_0}, \dots, S^{(j)}_{t_N})$:

$$
\hat{p} = e^{-rT} \frac{1}{M} \sum_{j=1}^{M} \varphi\big(S^{(j)}_{t_0}, \dots, S^{(j)}_{t_N}\big)
$$

The variance of the estimator is approximated by

$$
\xi_M^2 = e^{-2rT}\Bigg(\frac{1}{M}\sum_{j=1}^{M} \varphi_j^2 - \Big(\frac{1}{M}\sum_{j=1}^{M} \varphi_j\Big)^2\Bigg)
$$

and the 95% confidence interval is $\hat{p} \pm 1.96 \xi_M / \sqrt{M}$.
In the code, `priceStdDev` is $\xi_M / \sqrt{M}$, so the interval is `price ± 1.96 × priceStdDev`.

### Price at time t

Since $S_{t+u} = S_t \tilde{S}_u$, where $\tilde{S}$ has the same law as $S$, starts at 1 and is
independent of $\mathcal{F}_t$, only the future is simulated, starting from the observed price $s_t$:

$$
v(t) \approx e^{-r(T-t)} \frac{1}{M} \sum_{j=1}^{M} \varphi\Big(s_{t_0}, \dots, s_{t_i}, s_t \tilde{S}^{(j)}_{t_{i+1}-t}, \dots, s_t \tilde{S}^{(j)}_{t_N-t}\Big)
$$

The values $s_{t_0}, \dots, s_{t_i}, s_t$ are the quotes observed on the market up to time $t$ (the
`past`). The first simulated step goes from $t$ to the next fixing date $t_{i+1}$, so it lasts
$t_{i+1} - t$ and not $T/N$.

### Delta

Centred finite differences with relative step $h$ (`fd step`):

$$
\delta_d \approx \frac{e^{-r(T-t)}}{2 h s_{t,d}} \cdot \frac{1}{M} \sum_{j=1}^{M} \Big(\varphi\big(\dots, g_{d,+}(s_t)\tilde{S}^{(j)}_{t_{i+1}-t}, \dots\big) - \varphi\big(\dots, g_{d,-}(s_t)\tilde{S}^{(j)}_{t_{i+1}-t}, \dots\big)\Big)
$$

where $g_{d,\pm}(x)$ is the vector $x$ with its component $d$ multiplied by $(1 \pm h)$.
For each sample, **one single path** is simulated, then the two shifted paths are built from it
(`BlackScholesModel::shift_asset`). They share the same randomness, which keeps the variance of the
difference low.

### Hedging

Let $\tau_j = jT/H$ be the market dates, with $\{t_0, \dots, t_N\} \subset \{\tau_0, \dots, \tau_H\}$.
At each $\tau_j$, the price $p_j$ and the delta $\delta_j$ are computed with the observed past, and
the portfolio is rebalanced. The part invested at the risk-free rate evolves as

$$
V_0 = p_0 - \delta_0 \cdot S_0, \qquad V_j = V_{j-1} e^{rT/H} - (\delta_j - \delta_{j-1}) \cdot S_{\tau_j}, \quad j = 1, \dots, H
$$

and the hedging error is

$$
\text{PnL} = V_H + \delta_H \cdot S_{\tau_H} - \text{payoff}
$$

Two grids are involved: the fixing dates (step $T/N$, used by the payoff) and the finer rebalancing
dates (step $T/H$, used by the hedging). $H$ must be a multiple of $N$.



| Class | Role |
|---|---|
| `BlackScholesModel` | simulates the assets: `asset` (risk-neutral, from 0 or from a time t knowing the past), `shift_asset` (shifted path for the delta), `simul_market` (historical probability) |
| `Option` | abstract class: maturity, number of fixing dates, number of assets and the pure virtual `payoff` |
| `BasketOption`, `AsianOption`, `PerformanceOption` | one payoff each |
| `MonteCarlo` | `price` (time 0 and time t) and `delta` |
| `compute_pnl` | P&L of the delta hedging along a market trajectory |
| `pricer_setup` | reads the json and the market file and builds the model, the option, the past and the random generator |

## Requirements and build

- A C++11 compiler and CMake >= 3.11
- [PNL](https://github.com/pnlnum/pnl), compiled and installed (linear algebra, random numbers)
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

### Build the pricer

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
| `test_simul_market` | `./test_simul_market file.json` | check of the market simulation (`[OK]` or `[ECHEC]`) |
| `test_json_reader` | `./test_json_reader file.json` | provided parser test |

Examples:

```bash
./test_price0 ../data/asian/asian.json
./test_pricet ../data/asian/asian.json ../data/asian/asian_market.txt 180
./test_hedge  ../data/asian/asian.json ../data/asian/asian_market.txt
```

`market_line_t` is a row of the market file: the current time of row $j$ is $t = j T / H$.
Row 0 is time 0 and row $H$ is the maturity.

## Input files

### Parameter file (json)

| Key | Type | Meaning |
|---|---|---|
| `option type` | string | `basket`, `asian` or `performance` |
| `option size` | int | number of assets $D$ |
| `spot` | vector | initial prices $S_0$ |
| `maturity` | float | maturity $T$ |
| `volatility` | vector | volatilities $\sigma$ |
| `interest rate` | float | risk-free rate $r$ |
| `correlation` | float | correlation $\rho$ between any two assets |
| `trend` | vector | drift $\mu$ of each asset under the historical probability |
| `strike` | float | strike $K$ (not used by `performance`) |
| `payoff coefficients` | vector | coefficients $\lambda$ |
| `fixing dates number` | int | number of fixing dates $N$ |
| `hedging dates number` | int | number of market dates $H$ |
| `sample number` | long | number of Monte Carlo samples $M$ |
| `fd step` | float | relative step $h$ of the finite differences |

A vector key may contain a single value: it is then used for every asset.



### Market file (txt)

$(H+1)$ rows and $D$ columns: one row per market date $\tau_j$ and one column per asset. 
$H$ must be a multiple of $N$, so that every fixing date is a market date.

## Conventions

| Object | Shape | Rows / columns |
|---|---|---|
| `path` (simulated path) | $D \times (N+1)$ | row `i` = asset, column `j` = fixing date |
| `market` | $(H+1) \times D$ | row = market date, column = asset |
| `past` | $D \times (k+1)$ or $D \times (k+2)$ | fixing dates already passed (up to $t_k$, the last fixing date $\le t$); the **last column is always the price at t** |

`past` has one extra column holding $s_t$ when $t$ is not a fixing date.



