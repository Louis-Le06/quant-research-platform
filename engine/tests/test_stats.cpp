#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <vector>

#include "quant/stats.hpp"

using namespace quant;

TEST(ReturnsTest, SimpleReturns) {
    const auto r = simpleReturns({100.0, 110.0, 99.0});
    ASSERT_EQ(r.size(), 2u);
    EXPECT_NEAR(r[0], 0.10, 1e-12);
    EXPECT_NEAR(r[1], -0.10, 1e-12);
}

TEST(ReturnsTest, LogReturnsSumToTotalLogReturn) {
    const std::vector<double> prices{100, 103, 101, 108, 104};
    const auto r = logReturns(prices);
    double sum = 0.0;
    for (double x : r) sum += x;
    EXPECT_NEAR(sum, std::log(104.0 / 100.0), 1e-12);
}

TEST(ReturnsTest, RejectsBadInput) {
    EXPECT_THROW(simpleReturns({100.0}), std::invalid_argument);
    EXPECT_THROW(simpleReturns({100.0, 0.0}), std::invalid_argument);
    EXPECT_THROW(logReturns({100.0, -5.0}), std::invalid_argument);
}

TEST(MomentsTest, MeanAndSampleStddev) {
    const std::vector<double> xs{2, 4, 4, 4, 5, 5, 7, 9};
    EXPECT_DOUBLE_EQ(mean(xs), 5.0);
    EXPECT_NEAR(stddev(xs), 2.138089935, 1e-9);  // n-1
    EXPECT_THROW(mean({}), std::invalid_argument);
    EXPECT_THROW(stddev({1.0}), std::invalid_argument);
}

TEST(AnnualizeTest, VolatilityScalesWithSqrtTime) {
    const std::vector<double> r{0.01, -0.01, 0.01, -0.01};
    EXPECT_NEAR(annualizedVolatility(r, 252), stddev(r) * std::sqrt(252.0), 1e-12);
}

TEST(AnnualizeTest, ConstantReturnCompounds) {
    const std::vector<double> r(252, 0.001);
    EXPECT_NEAR(annualizedReturn(r, 252), std::pow(1.001, 252) - 1.0, 1e-12);
}

TEST(SharpeTest, ZeroVolatilityGivesZero) {
    EXPECT_DOUBLE_EQ(sharpeRatio({0.01, 0.01, 0.01}), 0.0);
}

TEST(SharpeTest, KnownValue) {
    const std::vector<double> r{0.02, -0.01, 0.03, 0.00};
    const double expected = mean(r) / stddev(r) * std::sqrt(252.0);
    EXPECT_NEAR(sharpeRatio(r), expected, 1e-12);
    EXPECT_LT(sharpeRatio(r, 0.05), expected);  // higher risk-free rate -> lower Sharpe
}

TEST(DrawdownTest, FindsLargestPeakToTrough) {
    // peak 120 -> trough 90 = -25%, then recovery
    EXPECT_NEAR(maxDrawdown({100, 120, 110, 90, 130, 117}), 0.25, 1e-12);
}

TEST(DrawdownTest, MonotonicIncreaseHasNoDrawdown) {
    EXPECT_DOUBLE_EQ(maxDrawdown({1, 2, 3, 4}), 0.0);
}

TEST(RollingVolTest, OutputLengthAndValues) {
    const std::vector<double> r{0.01, -0.02, 0.015, 0.0, -0.01, 0.02};
    const auto rv = rollingVolatility(r, 3, 252);
    ASSERT_EQ(rv.size(), 4u);
    EXPECT_NEAR(rv[0], annualizedVolatility({0.01, -0.02, 0.015}, 252), 1e-12);
    EXPECT_THROW(rollingVolatility(r, 1), std::invalid_argument);
    EXPECT_THROW(rollingVolatility(r, 10), std::invalid_argument);
}
