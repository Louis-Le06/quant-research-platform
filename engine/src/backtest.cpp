#include "quant/backtest.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

#include "quant/stats.hpp"

namespace quant {

std::vector<double> simpleMovingAverage(const std::vector<double>& prices, std::size_t window) {
    if (window == 0) throw std::invalid_argument("window must be >= 1");
    std::vector<double> out(prices.size(), std::numeric_limits<double>::quiet_NaN());
    double sum = 0.0;
    for (std::size_t i = 0; i < prices.size(); ++i) {
        sum += prices[i];
        if (i >= window) sum -= prices[i - window];
        if (i + 1 >= window) out[i] = sum / static_cast<double>(window);
    }
    return out;
}

BacktestResult smaCrossover(const std::vector<double>& prices, std::size_t fast, std::size_t slow,
                            double costBps, double periodsPerYear) {
    if (fast == 0 || slow == 0) throw std::invalid_argument("windows must be >= 1");
    if (fast >= slow) throw std::invalid_argument("fast window must be smaller than slow window");
    if (prices.size() < slow + 2) throw std::invalid_argument("not enough prices for slow window");
    if (costBps < 0.0) throw std::invalid_argument("cost must be >= 0");

    const auto rets = simpleReturns(prices);  // also validates prices > 0
    const auto fastMa = simpleMovingAverage(prices, fast);
    const auto slowMa = simpleMovingAverage(prices, slow);
    const double cost = costBps / 10000.0;

    BacktestResult res;
    res.equity.assign(prices.size(), 1.0);
    res.positions.assign(prices.size(), 0);

    std::vector<double> strategyReturns;
    strategyReturns.reserve(rets.size());
    for (std::size_t t = 1; t < prices.size(); ++t) {
        const bool ready = !std::isnan(slowMa[t - 1]);
        const int pos = (ready && fastMa[t - 1] > slowMa[t - 1]) ? 1 : 0;
        res.positions[t] = pos;

        double r = pos * rets[t - 1];
        if (pos != res.positions[t - 1]) {
            r -= cost;
            ++res.trades;
        }
        strategyReturns.push_back(r);
        res.equity[t] = res.equity[t - 1] * (1.0 + r);
    }

    res.totalReturn = res.equity.back() - 1.0;
    res.annualizedReturn = annualizedReturn(strategyReturns, periodsPerYear);
    res.annualizedVolatility = annualizedVolatility(strategyReturns, periodsPerYear);
    res.sharpe = sharpeRatio(strategyReturns, 0.0, periodsPerYear);
    res.maxDrawdown = maxDrawdown(res.equity);
    return res;
}

}  // namespace quant
