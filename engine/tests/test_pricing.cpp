#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

#include "quant/pricing.hpp"

using namespace quant;

namespace {
OptionParams atm(OptionType type = OptionType::Call) {
    return OptionParams{100.0, 100.0, 0.05, 0.20, 1.0, type, 0.0};
}
}  // namespace

// ---------- Normal distribution ----------

TEST(NormalTest, CdfKnownValues) {
    EXPECT_NEAR(normCdf(0.0), 0.5, 1e-12);
    EXPECT_NEAR(normCdf(1.96), 0.9750021, 1e-6);
    EXPECT_NEAR(normCdf(-1.96), 0.0249979, 1e-6);
}

TEST(NormalTest, PdfIsSymmetric) {
    EXPECT_NEAR(normPdf(0.0), 0.3989423, 1e-6);
    EXPECT_DOUBLE_EQ(normPdf(1.3), normPdf(-1.3));
}

// ---------- Black-Scholes price ----------

TEST(BlackScholesTest, MatchesTextbookValues) {
    // Standard textbook case (Hull): S=K=100, r=5%, sigma=20%, T=1
    EXPECT_NEAR(blackScholesPrice(atm(OptionType::Call)), 10.4506, 1e-4);
    EXPECT_NEAR(blackScholesPrice(atm(OptionType::Put)), 5.5735, 1e-4);
}

TEST(BlackScholesTest, PutCallParityHolds) {
    for (double k : {80.0, 95.0, 100.0, 110.0, 130.0}) {
        OptionParams c{100.0, k, 0.03, 0.25, 0.75, OptionType::Call, 0.01};
        OptionParams p = c;
        p.type = OptionType::Put;
        const double lhs = blackScholesPrice(c) - blackScholesPrice(p);
        const double rhs = c.spot * std::exp(-c.dividend * c.time) - k * std::exp(-c.rate * c.time);
        EXPECT_NEAR(lhs, rhs, 1e-10) << "strike=" << k;
    }
}

TEST(BlackScholesTest, CallPriceIncreasesWithVol) {
    auto low = atm();
    auto high = atm();
    high.vol = 0.4;
    EXPECT_GT(blackScholesPrice(high), blackScholesPrice(low));
}

TEST(BlackScholesTest, DeepItmCallApproachesForwardIntrinsic) {
    OptionParams p{200.0, 50.0, 0.05, 0.2, 1.0, OptionType::Call, 0.0};
    EXPECT_NEAR(blackScholesPrice(p), 200.0 - 50.0 * std::exp(-0.05), 1e-6);
}

TEST(BlackScholesTest, RejectsInvalidInputs) {
    auto p = atm();
    p.spot = 0.0;
    EXPECT_THROW(blackScholesPrice(p), std::invalid_argument);
    p = atm();
    p.vol = -0.1;
    EXPECT_THROW(blackScholesPrice(p), std::invalid_argument);
    p = atm();
    p.time = 0.0;
    EXPECT_THROW(blackScholesPrice(p), std::invalid_argument);
    p = atm();
    p.rate = std::nan("");
    EXPECT_THROW(blackScholesPrice(p), std::invalid_argument);
}

TEST(OptionTypeTest, ParsesCaseInsensitive) {
    EXPECT_EQ(parseOptionType("call"), OptionType::Call);
    EXPECT_EQ(parseOptionType("PUT"), OptionType::Put);
    EXPECT_THROW(parseOptionType("straddle"), std::invalid_argument);
}

// ---------- Greeks ----------

TEST(GreeksTest, AtmCallKnownValues) {
    const Greeks g = blackScholesGreeks(atm());
    EXPECT_NEAR(g.delta, 0.636831, 1e-5);
    EXPECT_NEAR(g.gamma, 0.018762, 1e-5);
    EXPECT_NEAR(g.vega, 37.524, 1e-3);
}

TEST(GreeksTest, PutDeltaEqualsCallDeltaMinusOne) {
    const Greeks c = blackScholesGreeks(atm(OptionType::Call));
    const Greeks p = blackScholesGreeks(atm(OptionType::Put));
    EXPECT_NEAR(c.delta - p.delta, 1.0, 1e-12);
    EXPECT_NEAR(c.gamma, p.gamma, 1e-12);
    EXPECT_NEAR(c.vega, p.vega, 1e-12);
}

// Analytic Greeks must match central finite differences, for calls and puts.
class GreeksFiniteDiffTest : public ::testing::TestWithParam<OptionType> {};

TEST_P(GreeksFiniteDiffTest, AnalyticMatchesFiniteDifference) {
    OptionParams p{105.0, 100.0, 0.04, 0.3, 0.5, GetParam(), 0.02};
    const Greeks g = blackScholesGreeks(p);
    const double h = 1e-4;

    auto bump = [&](auto field, double dx) {
        OptionParams up = p, dn = p;
        up.*field += dx;
        dn.*field -= dx;
        return (blackScholesPrice(up) - blackScholesPrice(dn)) / (2 * dx);
    };

    EXPECT_NEAR(g.delta, bump(&OptionParams::spot, h), 1e-6);
    EXPECT_NEAR(g.vega, bump(&OptionParams::vol, h), 1e-4);
    EXPECT_NEAR(g.rho, bump(&OptionParams::rate, h), 1e-4);
    EXPECT_NEAR(g.theta, -bump(&OptionParams::time, h), 1e-4);

    OptionParams up = p, dn = p;
    up.spot += 0.01;
    dn.spot -= 0.01;
    const double gammaFd =
        (blackScholesPrice(up) - 2 * blackScholesPrice(p) + blackScholesPrice(dn)) / (0.01 * 0.01);
    EXPECT_NEAR(g.gamma, gammaFd, 1e-4);
}

INSTANTIATE_TEST_SUITE_P(CallAndPut, GreeksFiniteDiffTest,
                         ::testing::Values(OptionType::Call, OptionType::Put),
                         [](const auto& info) {
                             return info.param == OptionType::Call ? "Call" : "Put";
                         });

// ---------- Implied volatility ----------

struct IvCase {
    double strike;
    double vol;
    OptionType type;
};

class ImpliedVolTest : public ::testing::TestWithParam<IvCase> {};

TEST_P(ImpliedVolTest, RecoversInputVolatility) {
    const auto c = GetParam();
    OptionParams p{100.0, c.strike, 0.03, c.vol, 0.5, c.type, 0.0};
    const double price = blackScholesPrice(p);
    EXPECT_NEAR(impliedVolatility(price, p), c.vol, 1e-6);
}

INSTANTIATE_TEST_SUITE_P(
    Smile, ImpliedVolTest,
    ::testing::Values(IvCase{80, 0.35, OptionType::Put}, IvCase{90, 0.25, OptionType::Call},
                      IvCase{100, 0.20, OptionType::Call}, IvCase{110, 0.18, OptionType::Put},
                      IvCase{130, 0.60, OptionType::Call}, IvCase{100, 1.50, OptionType::Put}));

TEST(ImpliedVolEdgeTest, RejectsPriceOutsideArbitrageBounds) {
    auto p = atm();
    EXPECT_THROW(impliedVolatility(150.0, p), std::invalid_argument);  // > spot
    EXPECT_THROW(impliedVolatility(0.0, p), std::invalid_argument);
}
