"""HTTP client for the Quant Research API."""

import os

import requests


class ApiError(RuntimeError):
    pass


class ApiClient:
    def __init__(self, base_url: str | None = None, timeout: float = 10.0):
        self.base_url = (base_url or os.getenv("API_URL", "http://localhost:8000")).rstrip("/")
        self.timeout = timeout
        self.session = requests.Session()

    def _post(self, path: str, payload: dict) -> dict:
        try:
            r = self.session.post(f"{self.base_url}{path}", json=payload, timeout=self.timeout)
        except requests.RequestException as exc:
            raise ApiError(f"Cannot reach API at {self.base_url}: {exc}") from exc
        if r.status_code != 200:
            detail = (
                r.json().get("detail", r.text)
                if r.headers.get("content-type", "").startswith("application/json")
                else r.text
            )
            raise ApiError(f"API error {r.status_code}: {detail}")
        return r.json()

    def version(self) -> dict:
        try:
            return self.session.get(f"{self.base_url}/version", timeout=self.timeout).json()
        except requests.RequestException:
            return {}

    def greeks(self, **params) -> dict:
        return self._post("/options/greeks", params)

    def curve(self, **params) -> dict:
        return self._post("/options/curve", params)

    def simulate(self, **params) -> list[float]:
        return self._post("/data/simulate", params)["prices"]

    def metrics(self, prices: list[float]) -> dict:
        return self._post("/analytics/metrics", {"prices": prices})

    def backtest(self, prices: list[float], fast: int, slow: int, cost_bps: float) -> dict:
        return self._post(
            "/backtest/sma-crossover",
            {"prices": prices, "fast": fast, "slow": slow, "cost_bps": cost_bps},
        )
