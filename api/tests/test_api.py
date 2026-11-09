"""Integration tests: FastAPI layer + real C++ engine (no mocks)."""

import math

import pytest
from fastapi.testclient import TestClient

from app.main import app

client = TestClient(app)

ATM = {"spot": 100, "strike": 100, "rate": 0.05, "vol": 0.2, "time": 1, "type": "call"}


def test_health():
    r = client.get("/health")
    assert r.status_code == 200
    assert r.json() == {"status": "ok"}


def test_version_reports_engine():
    body = client.get("/version").json()
    assert body["engine_version"] == "1.0.0"
    assert "pod" in body


def test_greeks_match_textbook():
    body = client.post("/options/greeks", json=ATM).json()
    assert body["price"] == pytest.approx(10.4506, abs=1e-4)
    assert body["delta"] == pytest.approx(0.6368, abs=1e-4)


def test_put_call_parity_through_api():
    call = client.post("/options/greeks", json=ATM).json()["price"]
    put = client.post("/options/greeks", json={**ATM, "type": "put"}).json()["price"]
    assert call - put == pytest.approx(100 - 100 * math.exp(-0.05), abs=1e-9)


@pytest.mark.parametrize(
    "payload",
    [
        {**ATM, "spot": -1},
        {**ATM, "vol": 0},
        {**ATM, "type": "straddle"},
        {k: v for k, v in ATM.items() if k != "time"},
    ],
)
def test_invalid_option_input_returns_422(payload):
    assert client.post("/options/greeks", json=payload).status_code == 422


def test_implied_vol_round_trip():
    price = client.post("/options/greeks", json={**ATM, "vol": 0.33}).json()["price"]
    payload = {k: v for k, v in ATM.items() if k != "vol"} | {"price": price}
    body = client.post("/options/implied-vol", json=payload).json()
    assert body["implied_vol"] == pytest.approx(0.33, abs=1e-6)


def test_implied_vol_arbitrage_violation_is_422():
    payload = {k: v for k, v in ATM.items() if k != "vol"} | {"price": 500}
    r = client.post("/options/implied-vol", json=payload)
    assert r.status_code == 422
    assert "arbitrage" in r.json()["detail"]


def test_curve_shape():
    body = client.post("/options/curve", json={**ATM, "points": 11}).json()
    assert len(body["spots"]) == 11
    assert body["delta"] == sorted(body["delta"])  # call delta increases with spot


def test_curve_rejects_bad_range():
    assert client.post("/options/curve", json={**ATM, "spot_min": 200}).status_code == 422


def test_simulate_is_deterministic():
    req = {"steps": 100, "seed": 7}
    a = client.post("/data/simulate", json=req).json()["prices"]
    b = client.post("/data/simulate", json=req).json()["prices"]
    assert a == b
    assert len(a) == 101


def test_metrics():
    prices = client.post("/data/simulate", json={"steps": 252}).json()["prices"]
    body = client.post("/analytics/metrics", json={"prices": prices}).json()
    assert 0 <= body["max_drawdown"] < 1
    assert body["annualized_volatility"] > 0
    assert len(body["rolling_volatility"]) == 252 - 21 + 1


def test_backtest_end_to_end():
    prices = client.post("/data/simulate", json={"steps": 500}).json()["prices"]
    r = client.post("/backtest/sma-crossover", json={"prices": prices, "fast": 10, "slow": 50})
    assert r.status_code == 200
    body = r.json()
    assert len(body["equity"]) == len(prices)
    assert body["slow_sma"][0] is None and body["slow_sma"][49] is not None
    assert body["buy_and_hold"][0] == 1.0


def test_backtest_rejects_fast_ge_slow():
    r = client.post("/backtest/sma-crossover", json={"prices": [1, 2, 3], "fast": 5, "slow": 5})
    assert r.status_code == 422
