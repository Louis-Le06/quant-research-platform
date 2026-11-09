#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <vector>

#include "quant/backtest.hpp"

using namespace quant;

TEST(SmaTest, ComputesMovingAverageWithNanWarmup) {
    const auto s = simpleMovingAverage({1, 2, 3, 4, 5}, 3);
    ASSERT_EQ(s.size(), 5u);
    EXPECT_TRUE(std::isnan(s[0]));
    EXPECT_TRUE(std::isnan(s[1]));
    EXPECT_DOUBLE_EQ(s[2], 2.0);
    EXPECT_DOUBLE_EQ(s[3], 3.0);
    EXPECT_DOUBLE_EQ(s[4], 4.0);
}

TEST(SmaTest, WindowOneIsIdentity) {
    const std::vector<double> p{3, 1, 4, 1, 5};
    EXPECT_EQ(simpleMovingAverage(p, 1), p);
}

class SmaCrossoverTest : public ::testing::Test {
protected:
    // Steady uptrend: the strategy should enter once and stay long.
    std::vector<double> uptrend() const {
        std::vector<double> p;
        for (int i = 0; i < 60; ++i) p.push_back(100.0 * std::pow(1.01, i));
        return p;
    }
};

TEST_F(SmaCrossoverTest, ResultSizesMatchInput) {
    const auto prices = uptrend();
    const auto r = smaCrossover(prices, 5, 20);
    EXPECT_EQ(r.equity.size(), prices.size());
    EXPECT_EQ(r.positions.size(), prices.size());
    EXPECT_DOUBLE_EQ(r.equity.front(), 1.0);
}

TEST_F(SmaCrossoverTest, GoesLongInUptrendAndMakesMoney) {
    const auto r = smaCrossover(uptrend(), 5, 20);
    EXPECT_EQ(r.positions.back(), 1);
    EXPECT_EQ(r.trades, 1);  // enters once and holds
    EXPECT_GT(r.totalReturn, 0.0);
    EXPECT_DOUBLE_EQ(r.maxDrawdown, 0.0);
}

TEST_F(SmaCrossoverTest, NoLookAheadBias) {
    // No position before the slow SMA has enough data (index slow-1),
    // and a signal only takes effect on the following day.
    const auto r = smaCrossover(uptrend(), 5, 20);
    for (std::size_t t = 0; t <= 19; ++t) EXPECT_EQ(r.positions[t], 0) << "t=" << t;
    EXPECT_EQ(r.positions[20], 1);
}

TEST_F(SmaCrossoverTest, TransactionCostsReduceReturn) {
    const auto noCost = smaCrossover(uptrend(), 5, 20, 0.0);
    const auto withCost = smaCrossover(uptrend(), 5, 20, 50.0);
    EXPECT_LT(withCost.totalReturn, noCost.totalReturn);
}

TEST_F(SmaCrossoverTest, StaysFlatInDowntrend) {
    std::vector<double> p;
    for (int i = 0; i < 60; ++i) p.push_back(100.0 * std::pow(0.99, i));
    const auto r = smaCrossover(p, 5, 20);
    EXPECT_EQ(r.trades, 0);
    EXPECT_DOUBLE_EQ(r.totalReturn, 0.0);
}

TEST(SmaCrossoverValidationTest, RejectsBadParameters) {
    const std::vector<double> p(50, 100.0);
    EXPECT_THROW(smaCrossover(p, 20, 5), std::invalid_argument);  // fast >= slow
    EXPECT_THROW(smaCrossover(p, 0, 5), std::invalid_argument);
    EXPECT_THROW(smaCrossover(p, 5, 60), std::invalid_argument);  // not enough data
    EXPECT_THROW(smaCrossover(p, 5, 20, -1.0), std::invalid_argument);
}
