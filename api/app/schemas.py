"""Request/response models. Validation happens here before reaching the C++ engine."""

from typing import Literal

from pydantic import BaseModel, Field, model_validator

OptionKind = Literal["call", "put"]


class OptionRequest(BaseModel):
    spot: float = Field(..., gt=0, examples=[100.0])
    strike: float = Field(..., gt=0, examples=[100.0])
    rate: float = Field(0.05, ge=-0.1, le=1.0, description="Annual risk-free rate")
    vol: float = Field(0.2, gt=0, le=5.0, description="Annual volatility")
    time: float = Field(..., gt=0, le=50, description="Time to expiry in years")
    type: OptionKind = "call"
    dividend: float = Field(0.0, ge=0, le=1.0)


class Greeks(BaseModel):
    price: float
    delta: float
    gamma: float
    vega: float
    theta: float
    rho: float


class ImpliedVolRequest(BaseModel):
    price: float = Field(..., gt=0)
    spot: float = Field(..., gt=0)
    strike: float = Field(..., gt=0)
    rate: float = 0.05
    time: float = Field(..., gt=0)
    type: OptionKind = "call"
    dividend: float = Field(0.0, ge=0)


class ImpliedVolResponse(BaseModel):
    implied_vol: float


class CurveRequest(OptionRequest):
    spot_min: float = Field(50.0, gt=0)
    spot_max: float = Field(150.0, gt=0)
    points: int = Field(101, ge=2, le=1000)

    @model_validator(mode="after")
    def check_range(self):
        if self.spot_min >= self.spot_max:
            raise ValueError("spot_min must be < spot_max")
        return self


class CurveResponse(BaseModel):
    spots: list[float]
    price: list[float]
    delta: list[float]
    gamma: list[float]


class SimulateRequest(BaseModel):
    s0: float = Field(100.0, gt=0)
    mu: float = Field(0.08, ge=-1, le=1)
    sigma: float = Field(0.2, ge=0, le=3)
    steps: int = Field(504, ge=1, le=100_000)
    seed: int = Field(42, ge=0)


class PricesResponse(BaseModel):
    prices: list[float]


class MetricsRequest(BaseModel):
    prices: list[float] = Field(..., min_length=3)
    periods_per_year: float = Field(252.0, gt=0)
    risk_free: float = 0.0
    rolling_window: int = Field(21, ge=2)


class MetricsResponse(BaseModel):
    total_return: float
    annualized_return: float
    annualized_volatility: float
    sharpe: float
    max_drawdown: float
    rolling_volatility: list[float]


class BacktestRequest(BaseModel):
    prices: list[float] = Field(..., min_length=3)
    fast: int = Field(20, ge=1)
    slow: int = Field(50, ge=2)
    cost_bps: float = Field(5.0, ge=0, le=1000)
    periods_per_year: float = Field(252.0, gt=0)

    @model_validator(mode="after")
    def check_windows(self):
        if self.fast >= self.slow:
            raise ValueError("fast window must be smaller than slow window")
        return self


class BacktestResponse(BaseModel):
    equity: list[float]
    positions: list[int]
    fast_sma: list[float | None]
    slow_sma: list[float | None]
    buy_and_hold: list[float]
    trades: int
    total_return: float
    annualized_return: float
    annualized_volatility: float
    sharpe: float
    max_drawdown: float
