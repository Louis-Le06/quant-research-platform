#include "quant/stats.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace quant {

namespace {
void checkPrices(const std::vector<double>& prices) {
    if (prices.size() < 2) throw std::invalid_argument("need at least 2 prices");
    for (double p : prices) {
        if (!(p > 0.0) || !std::isfinite(p)) throw std::invalid_argument("prices must be > 0");
    }
}
}  // namespace

std::vector<double> simpleReturns(const std::vector<double>& prices) {
    checkPrices(prices);
    std::vector<double> r(prices.size() - 1);
    for (std::size_t i = 1; i < prices.size(); ++i) r[i - 1] = prices[i] / prices[i - 1] - 1.0;
    return r;
}

std::vector<double> logReturns(const std::vector<double>& prices) {
    checkPrices(prices);
    std::vector<double> r(prices.size() - 1);
    for (std::size_t i = 1; i < prices.size(); ++i) r[i - 1] = std::log(prices[i] / prices[i - 1]);
    return r;
}

double mean(const std::vector<double>& xs) {
    if (xs.empty()) throw std::invalid_argument("empty series");
    return std::accumulate(xs.begin(), xs.end(), 0.0) / static_cast<double>(xs.size());
}

double stddev(const std::vector<double>& xs) {
    if (xs.size() < 2) throw std::invalid_argument("need at least 2 observations");
    const double m = mean(xs);
    double ss = 0.0;
    for (double x : xs) ss += (x - m) * (x - m);
    return std::sqrt(ss / static_cast<double>(xs.size() - 1));
}

double annualizedReturn(const std::vector<double>& returns, double periodsPerYear) {
    if (returns.empty()) throw std::invalid_argument("empty series");
    double growth = 1.0;
    for (double r : returns) growth *= (1.0 + r);
    return std::pow(growth, periodsPerYear / static_cast<double>(returns.size())) - 1.0;
}

double annualizedVolatility(const std::vector<double>& returns, double periodsPerYear) {
    return stddev(returns) * std::sqrt(periodsPerYear);
}

double sharpeRatio(const std::vector<double>& returns, double riskFreeAnnual,
                   double periodsPerYear) {
    const double sd = stddev(returns);
    if (sd == 0.0) return 0.0;
    const double excess = mean(returns) - riskFreeAnnual / periodsPerYear;
    return excess / sd * std::sqrt(periodsPerYear);
}

double maxDrawdown(const std::vector<double>& equity) {
    if (equity.empty()) throw std::invalid_argument("empty series");
    double peak = equity.front();
    double mdd = 0.0;
    for (double v : equity) {
        peak = std::max(peak, v);
        if (peak > 0.0) mdd = std::max(mdd, (peak - v) / peak);
    }
    return mdd;
}

std::vector<double> rollingVolatility(const std::vector<double>& returns, std::size_t window,
                                      double periodsPerYear) {
    if (window < 2) throw std::invalid_argument("window must be >= 2");
    if (returns.size() < window) throw std::invalid_argument("series shorter than window");
    std::vector<double> out;
    out.reserve(returns.size() - window + 1);
    for (std::size_t i = 0; i + window <= returns.size(); ++i) {
        const std::vector<double> slice(returns.begin() + static_cast<std::ptrdiff_t>(i),
                                        returns.begin() + static_cast<std::ptrdiff_t>(i + window));
        out.push_back(annualizedVolatility(slice, periodsPerYear));
    }
    return out;
}

}  // namespace quant
