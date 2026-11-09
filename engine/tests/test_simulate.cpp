#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

#include "quant/simulate.hpp"
#include "quant/stats.hpp"

using namespace quant;

TEST(GbmTest, PathHasCorrectLengthAndStart) {
    const auto path = simulateGbm(100.0, 0.05, 0.2, 252, 42);
    ASSERT_EQ(path.size(), 253u);
    EXPECT_DOUBLE_EQ(path.front(), 100.0);
    for (double p : path) EXPECT_GT(p, 0.0);
}

TEST(GbmTest, SameSeedIsDeterministic) {
    EXPECT_EQ(simulateGbm(100, 0.05, 0.2, 100, 7), simulateGbm(100, 0.05, 0.2, 100, 7));
    EXPECT_NE(simulateGbm(100, 0.05, 0.2, 100, 7), simulateGbm(100, 0.05, 0.2, 100, 8));
}

TEST(GbmTest, ZeroVolIsDeterministicGrowth) {
    const auto path = simulateGbm(100.0, 0.10, 0.0, 252, 1);
    EXPECT_NEAR(path.back(), 100.0 * std::exp(0.10), 1e-9);
}

TEST(GbmTest, RealizedVolCloseToInput) {
    // Law of large numbers: 20 years of daily data -> realised vol close to sigma
    const auto path = simulateGbm(100.0, 0.0, 0.25, 252 * 20, 123);
    EXPECT_NEAR(annualizedVolatility(logReturns(path)), 0.25, 0.01);
}

TEST(GbmTest, RejectsInvalidInputs) {
    EXPECT_THROW(simulateGbm(0.0, 0.05, 0.2, 10, 1), std::invalid_argument);
    EXPECT_THROW(simulateGbm(100.0, 0.05, -0.2, 10, 1), std::invalid_argument);
    EXPECT_THROW(simulateGbm(100.0, 0.05, 0.2, 0, 1), std::invalid_argument);
}
