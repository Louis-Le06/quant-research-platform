#include "quant/simulate.hpp"

#include <cmath>
#include <random>
#include <stdexcept>

namespace quant {

std::vector<double> simulateGbm(double s0, double mu, double sigma, std::size_t steps,
                                std::uint64_t seed, double periodsPerYear) {
    if (!(s0 > 0.0)) throw std::invalid_argument("s0 must be > 0");
    if (sigma < 0.0) throw std::invalid_argument("sigma must be >= 0");
    if (steps == 0) throw std::invalid_argument("steps must be >= 1");
    if (!(periodsPerYear > 0.0)) throw std::invalid_argument("periodsPerYear must be > 0");

    std::mt19937_64 rng(seed);
    std::normal_distribution<double> z(0.0, 1.0);
    const double dt = 1.0 / periodsPerYear;
    const double drift = (mu - 0.5 * sigma * sigma) * dt;
    const double diffusion = sigma * std::sqrt(dt);

    std::vector<double> path(steps + 1);
    path[0] = s0;
    for (std::size_t i = 1; i <= steps; ++i) {
        path[i] = path[i - 1] * std::exp(drift + diffusion * z(rng));
    }
    return path;
}

}  // namespace quant
