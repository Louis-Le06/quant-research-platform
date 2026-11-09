#pragma once

#include <cstddef>
#include <vector>

namespace quant {

// r_t = P_t / P_{t-1} - 1. Needs >= 2 prices, all > 0.
std::vector<double> simpleReturns(const std::vector<double>& prices);
// r_t = ln(P_t / P_{t-1})
std::vector<double> logReturns(const std::vector<double>& prices);

double mean(const std::vector<double>& xs);
double stddev(const std::vector<double>& xs);  // sample standard deviation (n-1)

double annualizedReturn(const std::vector<double>& returns, double periodsPerYear = 252.0);
double annualizedVolatility(const std::vector<double>& returns, double periodsPerYear = 252.0);

// Sharpe = (mean(r) - rf/periods) / stddev(r) * sqrt(periods). Returns 0 if stddev == 0.
double sharpeRatio(const std::vector<double>& returns, double riskFreeAnnual = 0.0,
                   double periodsPerYear = 252.0);

// Largest peak-to-trough decline as a positive fraction (0.25 = -25%).
double maxDrawdown(const std::vector<double>& equity);

// Annualised rolling volatility; returns returns.size() - window + 1 values.
std::vector<double> rollingVolatility(const std::vector<double>& returns, std::size_t window,
                                      double periodsPerYear = 252.0);

}  // namespace quant
