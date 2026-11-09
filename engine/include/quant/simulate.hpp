#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace quant {

// Geometric Brownian Motion price path (synthetic data for research and strategy testing).
// mu and sigma are annual; returns steps + 1 prices starting at s0. Same seed -> same path.
std::vector<double> simulateGbm(double s0, double mu, double sigma, std::size_t steps,
                                std::uint64_t seed, double periodsPerYear = 252.0);

}  // namespace quant
