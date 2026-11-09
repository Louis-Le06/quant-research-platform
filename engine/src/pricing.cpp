#include "quant/pricing.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace quant {

namespace {
constexpr double kInvSqrt2Pi = 0.39894228040143267794;

struct D1D2 {
    double d1;
    double d2;
};

D1D2 computeD(const OptionParams& p) {
    const double sqrtT = std::sqrt(p.time);
    const double d1 =
        (std::log(p.spot / p.strike) + (p.rate - p.dividend + 0.5 * p.vol * p.vol) * p.time) /
        (p.vol * sqrtT);
    return {d1, d1 - p.vol * sqrtT};
}
}  // namespace

OptionType parseOptionType(const std::string& s) {
    std::string lower(s);
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower == "call") return OptionType::Call;
    if (lower == "put") return OptionType::Put;
    throw std::invalid_argument("option type must be 'call' or 'put'");
}

double normPdf(double x) {
    return kInvSqrt2Pi * std::exp(-0.5 * x * x);
}

double normCdf(double x) {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

void validate(const OptionParams& p) {
    if (!(p.spot > 0.0)) throw std::invalid_argument("spot must be > 0");
    if (!(p.strike > 0.0)) throw std::invalid_argument("strike must be > 0");
    if (!(p.vol > 0.0)) throw std::invalid_argument("vol must be > 0");
    if (!(p.time > 0.0)) throw std::invalid_argument("time must be > 0");
    if (!std::isfinite(p.rate) || !std::isfinite(p.dividend))
        throw std::invalid_argument("rate and dividend must be finite");
}

double blackScholesPrice(const OptionParams& p) {
    validate(p);
    const auto [d1, d2] = computeD(p);
    const double dfR = std::exp(-p.rate * p.time);
    const double dfQ = std::exp(-p.dividend * p.time);
    if (p.type == OptionType::Call) {
        return p.spot * dfQ * normCdf(d1) - p.strike * dfR * normCdf(d2);
    }
    return p.strike * dfR * normCdf(-d2) - p.spot * dfQ * normCdf(-d1);
}

Greeks blackScholesGreeks(const OptionParams& p) {
    validate(p);
    const auto [d1, d2] = computeD(p);
    const double sqrtT = std::sqrt(p.time);
    const double dfR = std::exp(-p.rate * p.time);
    const double dfQ = std::exp(-p.dividend * p.time);
    const double pdf = normPdf(d1);

    Greeks g;
    g.price = blackScholesPrice(p);
    g.gamma = dfQ * pdf / (p.spot * p.vol * sqrtT);
    g.vega = p.spot * dfQ * pdf * sqrtT;
    const double decay = -p.spot * dfQ * pdf * p.vol / (2.0 * sqrtT);

    if (p.type == OptionType::Call) {
        g.delta = dfQ * normCdf(d1);
        g.theta =
            decay - p.rate * p.strike * dfR * normCdf(d2) + p.dividend * p.spot * dfQ * normCdf(d1);
        g.rho = p.strike * p.time * dfR * normCdf(d2);
    } else {
        g.delta = -dfQ * normCdf(-d1);
        g.theta = decay + p.rate * p.strike * dfR * normCdf(-d2) -
                  p.dividend * p.spot * dfQ * normCdf(-d1);
        g.rho = -p.strike * p.time * dfR * normCdf(-d2);
    }
    return g;
}

double impliedVolatility(double marketPrice, OptionParams p, double tol, int maxIter) {
    p.vol = 0.2;
    validate(p);
    const double dfR = std::exp(-p.rate * p.time);
    const double dfQ = std::exp(-p.dividend * p.time);
    const double fwdS = p.spot * dfQ;
    const double pvK = p.strike * dfR;
    const double lower =
        p.type == OptionType::Call ? std::max(0.0, fwdS - pvK) : std::max(0.0, pvK - fwdS);
    const double upper = p.type == OptionType::Call ? fwdS : pvK;
    if (!(marketPrice > lower) || !(marketPrice < upper)) {
        throw std::invalid_argument("market price outside no-arbitrage bounds");
    }

    double lo = 1e-6;
    double hi = 5.0;
    double sigma = 0.2;
    for (int i = 0; i < maxIter; ++i) {
        p.vol = sigma;
        const double diff = blackScholesPrice(p) - marketPrice;
        if (std::fabs(diff) < tol) return sigma;
        if (diff > 0.0)
            hi = sigma;
        else
            lo = sigma;

        const double vega = blackScholesGreeks(p).vega;
        double next = vega > 1e-10 ? sigma - diff / vega : 0.5 * (lo + hi);
        if (!(next > lo && next < hi)) next = 0.5 * (lo + hi);  // stay inside the bracket
        sigma = next;
    }
    return sigma;
}

}  // namespace quant
