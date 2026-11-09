"""Quant Research API: thin FastAPI layer over the C++ `quant_engine` module."""

import math
import os
import socket

import quant_engine as qe
from fastapi import FastAPI, Request
from fastapi.responses import JSONResponse

from app import schemas

APP_VERSION = os.getenv("APP_VERSION", "1.0.0")

app = FastAPI(
    title="Quant Research API",
    version=APP_VERSION,
    description="Option pricing, risk analytics and backtesting powered by a C++ engine.",
)


@app.exception_handler(ValueError)
async def engine_value_error(_: Request, exc: ValueError) -> JSONResponse:
    """The C++ engine raises std::invalid_argument -> Python ValueError -> HTTP 422."""
    return JSONResponse(status_code=422, content={"detail": str(exc)})


def _nan_to_none(xs: list[float]) -> list[float | None]:
    return [None if math.isnan(x) else x for x in xs]


# ----------------------------------------------------------------- meta
@app.get("/health", tags=["meta"])
def health() -> dict:
    return {"status": "ok"}


@app.get("/version", tags=["meta"])
def version() -> dict:
    return {
        "service": "quant-api",
        "version": APP_VERSION,
        "engine_version": qe.__version__,
        "pod": socket.gethostname(),
    }


# -------------------------------------------------------------- options
@app.post("/options/greeks", response_model=schemas.Greeks, tags=["options"])
def option_greeks(req: schemas.OptionRequest) -> dict:
    return qe.bs_greeks(req.spot, req.strike, req.rate, req.vol, req.time, req.type, req.dividend)


@app.post("/options/implied-vol", response_model=schemas.ImpliedVolResponse, tags=["options"])
def implied_vol(req: schemas.ImpliedVolRequest) -> dict:
    iv = qe.implied_vol(req.price, req.spot, req.strike, req.rate, req.time, req.type, req.dividend)
    return {"implied_vol": iv}


@app.post("/options/curve", response_model=schemas.CurveResponse, tags=["options"])
def option_curve(req: schemas.CurveRequest) -> dict:
    step = (req.spot_max - req.spot_min) / (req.points - 1)
    spots = [req.spot_min + i * step for i in range(req.points)]
    rows = [
        qe.bs_greeks(s, req.strike, req.rate, req.vol, req.time, req.type, req.dividend)
        for s in spots
    ]
    return {
        "spots": spots,
        "price": [r["price"] for r in rows],
        "delta": [r["delta"] for r in rows],
        "gamma": [r["gamma"] for r in rows],
    }


# ----------------------------------------------------------------- data
@app.post("/data/simulate", response_model=schemas.PricesResponse, tags=["data"])
def simulate(req: schemas.SimulateRequest) -> dict:
    return {"prices": qe.simulate_gbm(req.s0, req.mu, req.sigma, req.steps, req.seed)}


# ------------------------------------------------------------ analytics
@app.post("/analytics/metrics", response_model=schemas.MetricsResponse, tags=["analytics"])
def metrics(req: schemas.MetricsRequest) -> dict:
    rets = qe.simple_returns(req.prices)
    window = min(req.rolling_window, len(rets))
    return {
        "total_return": req.prices[-1] / req.prices[0] - 1.0,
        "annualized_return": qe.annualized_return(rets, req.periods_per_year),
        "annualized_volatility": qe.annualized_volatility(rets, req.periods_per_year),
        "sharpe": qe.sharpe_ratio(rets, req.risk_free, req.periods_per_year),
        "max_drawdown": qe.max_drawdown(req.prices),
        "rolling_volatility": qe.rolling_volatility(rets, window, req.periods_per_year),
    }


# ------------------------------------------------------------- backtest
@app.post("/backtest/sma-crossover", response_model=schemas.BacktestResponse, tags=["backtest"])
def backtest_sma(req: schemas.BacktestRequest) -> dict:
    res = qe.sma_crossover(req.prices, req.fast, req.slow, req.cost_bps, req.periods_per_year)
    first = req.prices[0]
    return {
        **res,
        "fast_sma": _nan_to_none(qe.sma(req.prices, req.fast)),
        "slow_sma": _nan_to_none(qe.sma(req.prices, req.slow)),
        "buy_and_hold": [p / first for p in req.prices],
    }
