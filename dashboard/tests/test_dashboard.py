"""Dashboard tests: figure builders and callbacks with a fake API client."""

import math

import pytest

import app as dash_app
import figures
from api_client import ApiError

CURVE = {
    "spots": [80, 100, 120],
    "price": [1.0, 10.0, 25.0],
    "delta": [0.2, 0.6, 0.9],
    "gamma": [0.01, 0.02, 0.01],
}

BT = {
    "equity": [1.0, 1.0, 1.01, 1.02],
    "positions": [0, 0, 1, 1],
    "fast_sma": [None, 1.0, 1.1, 1.2],
    "slow_sma": [None, None, 1.0, 1.1],
    "buy_and_hold": [1.0, 1.01, 1.02, 1.03],
    "trades": 1,
    "total_return": 0.02,
    "annualized_return": 0.3,
    "annualized_volatility": 0.1,
    "sharpe": 1.5,
    "max_drawdown": 0.0,
}
BENCH = {
    "total_return": 0.03,
    "annualized_return": 0.4,
    "annualized_volatility": 0.15,
    "sharpe": 1.2,
    "max_drawdown": 0.05,
    "rolling_volatility": [],
}


class FakeClient:
    base_url = "http://fake"

    def __init__(self, fail: bool = False):
        self.fail = fail

    def _maybe_fail(self):
        if self.fail:
            raise ApiError("API error 422: spot must be > 0")

    def version(self):
        return {"version": "1.0.0", "engine_version": "1.0.0", "pod": "pod-abc"}

    def greeks(self, **_):
        self._maybe_fail()
        return {
            "price": 10.45,
            "delta": 0.64,
            "gamma": 0.019,
            "vega": 37.5,
            "theta": -6.4,
            "rho": 53.2,
        }

    def curve(self, **_):
        self._maybe_fail()
        return CURVE

    def simulate(self, **_):
        self._maybe_fail()
        return [100.0, 101.0, 102.0, 103.0]

    def backtest(self, *_):
        return BT

    def metrics(self, _):
        return BENCH


# ---------------------------------------------------------------- figures
@pytest.mark.parametrize("kind, expected_payoff", [("call", [0, 0, 20]), ("put", [20, 0, 0])])
def test_option_curve_payoff(kind, expected_payoff):
    fig = figures.option_curve_figure(CURVE, strike=100, option_type=kind)
    payoff = next(t for t in fig.data if t.name == "Payoff at expiry")
    assert list(payoff.y) == expected_payoff


def test_price_signals_marks_entries():
    fig = figures.price_signals_figure([100, 101, 102, 103], BT, 2, 3)
    entries = next(t for t in fig.data if t.name == "Enter long")
    assert list(entries.x) == [2]


def test_metrics_rows_formatting():
    rows = figures.metrics_rows(BT, BENCH)
    by_metric = {r["metric"]: r for r in rows}
    assert by_metric["Total return"]["strategy"] == "2.00%"
    assert by_metric["Max drawdown"]["buy_and_hold"] == "-5.00%"
    assert by_metric["Sharpe ratio"]["strategy"] == "1.50"


# -------------------------------------------------------------- callbacks
def test_layout_has_expected_components():
    layout = str(dash_app.app.layout)
    for component_id in ("opt-chart", "bt-run", "bt-metrics", "bt-equity-chart"):
        assert component_id in layout


def test_option_callback_success(monkeypatch):
    monkeypatch.setattr(dash_app, "client", FakeClient())
    cards, fig, err = dash_app.update_option("call", 100, 100, 20, 5, 0, 1)
    assert err == ""
    assert len(cards) == 6
    assert fig.data


def test_option_callback_shows_api_error(monkeypatch):
    monkeypatch.setattr(dash_app, "client", FakeClient(fail=True))
    cards, fig, err = dash_app.update_option("call", 100, 100, 20, 5, 0, 1)
    assert "422" in err and cards == []


def test_option_callback_requires_all_fields():
    _, _, err = dash_app.update_option("call", None, 100, 20, 5, 0, 1)
    assert "fill in" in err


def test_backtest_callback(monkeypatch):
    monkeypatch.setattr(dash_app, "client", FakeClient())
    rows, price_fig, equity_fig, err = dash_app.run_backtest(1, 8, 20, 756, 42, 2, 3, 5)
    assert err == "" and len(rows) == 6
    assert not math.isnan(equity_fig.data[0].y[-1])


def test_api_info(monkeypatch):
    monkeypatch.setattr(dash_app, "client", FakeClient())
    assert "pod-abc" in dash_app.show_api_info(1)
