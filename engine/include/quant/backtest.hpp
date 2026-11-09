#pragma once

#include <cstddef>
#include <vector>

namespace quant {

// SMA with the same length as prices; positions before the window is full are NaN.
std::vector<double> simpleMovingAverage(const std::vector<double>& prices, std::size_t window);

struct BacktestResult {
    std::vector<double> equity;  // portfolio value, starts at 1.0
    std::vector<int> positions;  // 1 = long, 0 = flat
    int trades{0};               // number of position changes
    double totalReturn{0.0};
    double annualizedReturn{0.0};
    double annualizedVolatility{0.0};
    double sharpe{0.0};
    double maxDrawdown{0.0};
};

// Long-only SMA crossover: long while the fast SMA is above the slow SMA.
// The signal at t-1 sets the position at t (no look-ahead bias).
// costBps: transaction cost in basis points per position change.
BacktestResult smaCrossover(const std::vector<double>& prices, std::size_t fast, std::size_t slow,
                            double costBps = 0.0, double periodsPerYear = 252.0);

}  // namespace quant
