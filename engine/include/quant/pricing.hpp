#pragma once

#include <string>

namespace quant {

enum class OptionType { Call, Put };

OptionType parseOptionType(const std::string& s);  // "call" / "put"

// European option parameters. Rate, dividend yield and vol are annual; time is in years.
struct OptionParams {
    double spot{};
    double strike{};
    double rate{};
    double vol{};
    double time{};
    OptionType type{OptionType::Call};
    double dividend{0.0};
};

struct Greeks {
    double price{};
    double delta{};
    double gamma{};
    double vega{};   // per 1.00 vol (not per 1%)
    double theta{};  // per year
    double rho{};
};

double normPdf(double x);
double normCdf(double x);

// Throws std::invalid_argument on invalid parameters.
void validate(const OptionParams& p);

double blackScholesPrice(const OptionParams& p);
Greeks blackScholesGreeks(const OptionParams& p);

// Implied volatility via Newton-Raphson, falling back to bisection when vega is tiny.
// p.vol is ignored. Throws std::invalid_argument if the price violates no-arbitrage bounds.
double impliedVolatility(double marketPrice, OptionParams p, double tol = 1e-8, int maxIter = 100);

}  // namespace quant
