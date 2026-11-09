// pybind11 bindings: expose the C++ engine to Python as `quant_engine`.
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "quant/backtest.hpp"
#include "quant/pricing.hpp"
#include "quant/simulate.hpp"
#include "quant/stats.hpp"

namespace py = pybind11;

namespace {
quant::OptionParams makeParams(double spot, double strike, double rate, double vol, double time,
                               const std::string& type, double dividend) {
    return quant::OptionParams{spot,    strike, rate, vol, time, quant::parseOptionType(type),
                               dividend};
}
}  // namespace

PYBIND11_MODULE(quant_engine, m) {
    m.doc() = "C++ quant engine: Black-Scholes pricing, risk analytics, backtesting";
    m.attr("__version__") = "1.0.0";

    // pybind11 translates std::invalid_argument into Python ValueError automatically.

    m.def(
        "bs_greeks",
        [](double spot, double strike, double rate, double vol, double time,
           const std::string& type, double dividend) {
            const auto g = quant::blackScholesGreeks(
                makeParams(spot, strike, rate, vol, time, type, dividend));
            py::dict d;
            d["price"] = g.price;
            d["delta"] = g.delta;
            d["gamma"] = g.gamma;
            d["vega"] = g.vega;
            d["theta"] = g.theta;
            d["rho"] = g.rho;
            return d;
        },
        py::arg("spot"), py::arg("strike"), py::arg("rate"), py::arg("vol"), py::arg("time"),
        py::arg("type") = "call", py::arg("dividend") = 0.0,
        "Black-Scholes price and Greeks for a European option");

    m.def(
        "implied_vol",
        [](double price, double spot, double strike, double rate, double time,
           const std::string& type, double dividend) {
            return quant::impliedVolatility(
                price, makeParams(spot, strike, rate, 0.2, time, type, dividend));
        },
        py::arg("price"), py::arg("spot"), py::arg("strike"), py::arg("rate"), py::arg("time"),
        py::arg("type") = "call", py::arg("dividend") = 0.0,
        "Implied volatility from market price");

    m.def("simple_returns", &quant::simpleReturns, py::arg("prices"));
    m.def("log_returns", &quant::logReturns, py::arg("prices"));
    m.def("annualized_return", &quant::annualizedReturn, py::arg("returns"),
          py::arg("periods_per_year") = 252.0);
    m.def("annualized_volatility", &quant::annualizedVolatility, py::arg("returns"),
          py::arg("periods_per_year") = 252.0);
    m.def("sharpe_ratio", &quant::sharpeRatio, py::arg("returns"),
          py::arg("risk_free_annual") = 0.0, py::arg("periods_per_year") = 252.0);
    m.def("max_drawdown", &quant::maxDrawdown, py::arg("equity"));
    m.def("rolling_volatility", &quant::rollingVolatility, py::arg("returns"), py::arg("window"),
          py::arg("periods_per_year") = 252.0);
    m.def("sma", &quant::simpleMovingAverage, py::arg("prices"), py::arg("window"));

    m.def(
        "sma_crossover",
        [](const std::vector<double>& prices, std::size_t fast, std::size_t slow, double costBps,
           double periods) {
            const auto r = quant::smaCrossover(prices, fast, slow, costBps, periods);
            py::dict d;
            d["equity"] = r.equity;
            d["positions"] = r.positions;
            d["trades"] = r.trades;
            d["total_return"] = r.totalReturn;
            d["annualized_return"] = r.annualizedReturn;
            d["annualized_volatility"] = r.annualizedVolatility;
            d["sharpe"] = r.sharpe;
            d["max_drawdown"] = r.maxDrawdown;
            return d;
        },
        py::arg("prices"), py::arg("fast"), py::arg("slow"), py::arg("cost_bps") = 0.0,
        py::arg("periods_per_year") = 252.0, "Long-only SMA crossover backtest");

    m.def("simulate_gbm", &quant::simulateGbm, py::arg("s0"), py::arg("mu"), py::arg("sigma"),
          py::arg("steps"), py::arg("seed"), py::arg("periods_per_year") = 252.0);
}
