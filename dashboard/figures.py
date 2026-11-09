"""Pure functions that turn API data into Plotly figures (easy to unit test)."""

import plotly.graph_objects as go
from plotly.subplots import make_subplots

TEMPLATE = "plotly_white"
ACCENT = "#2563eb"
MUTED = "#94a3b8"
GOOD = "#16a34a"
WARN = "#ea580c"


def option_curve_figure(curve: dict, strike: float, option_type: str) -> go.Figure:
    spots = curve["spots"]
    if option_type == "call":
        payoff = [max(s - strike, 0.0) for s in spots]
    else:
        payoff = [max(strike - s, 0.0) for s in spots]

    fig = make_subplots(specs=[[{"secondary_y": True}]])
    fig.add_trace(go.Scatter(x=spots, y=curve["price"], name="Option value", line_color=ACCENT))
    fig.add_trace(
        go.Scatter(x=spots, y=payoff, name="Payoff at expiry", line=dict(color=MUTED, dash="dash"))
    )
    fig.add_trace(
        go.Scatter(x=spots, y=curve["delta"], name="Delta", line_color=WARN), secondary_y=True
    )
    fig.add_vline(x=strike, line=dict(color=MUTED, width=1, dash="dot"))
    fig.update_layout(
        template=TEMPLATE,
        title="Value & delta vs underlying price",
        legend=dict(orientation="h", y=-0.2),
        margin=dict(l=40, r=40, t=50, b=40),
    )
    fig.update_xaxes(title_text="Underlying price")
    fig.update_yaxes(title_text="Value", secondary_y=False)
    delta_range = [-0.05, 1.05] if option_type == "call" else [-1.05, 0.05]
    fig.update_yaxes(
        title_text="Delta", range=delta_range, tickformat=".1f", showgrid=False, secondary_y=True
    )
    return fig


def price_signals_figure(prices: list[float], bt: dict, fast: int, slow: int) -> go.Figure:
    x = list(range(len(prices)))
    fig = go.Figure()
    fig.add_trace(go.Scatter(x=x, y=prices, name="Price", line=dict(color=MUTED, width=1)))
    fig.add_trace(go.Scatter(x=x, y=bt["fast_sma"], name=f"SMA {fast}", line_color=ACCENT))
    fig.add_trace(go.Scatter(x=x, y=bt["slow_sma"], name=f"SMA {slow}", line_color=WARN))

    # Mark entries and exits
    pos = bt["positions"]
    entries = [i for i in range(1, len(pos)) if pos[i] == 1 and pos[i - 1] == 0]
    exits = [i for i in range(1, len(pos)) if pos[i] == 0 and pos[i - 1] == 1]
    fig.add_trace(
        go.Scatter(
            x=entries,
            y=[prices[i] for i in entries],
            mode="markers",
            name="Enter long",
            marker=dict(symbol="triangle-up", size=10, color=GOOD),
        )
    )
    fig.add_trace(
        go.Scatter(
            x=exits,
            y=[prices[i] for i in exits],
            mode="markers",
            name="Exit",
            marker=dict(symbol="triangle-down", size=10, color=WARN),
        )
    )
    fig.update_layout(
        template=TEMPLATE,
        title="Price & SMA crossover signals",
        xaxis_title="Trading day",
        yaxis_title="Price",
        legend=dict(orientation="h", y=-0.2),
        margin=dict(l=40, r=20, t=50, b=40),
    )
    return fig


def equity_figure(bt: dict) -> go.Figure:
    x = list(range(len(bt["equity"])))
    fig = go.Figure()
    fig.add_trace(go.Scatter(x=x, y=bt["equity"], name="Strategy", line_color=ACCENT))
    fig.add_trace(
        go.Scatter(
            x=x, y=bt["buy_and_hold"], name="Buy & hold", line=dict(color=MUTED, dash="dash")
        )
    )
    fig.update_layout(
        template=TEMPLATE,
        title="Equity curve (start = 1.0)",
        xaxis_title="Trading day",
        yaxis_title="Growth of 1",
        legend=dict(orientation="h", y=-0.2),
        margin=dict(l=40, r=20, t=50, b=40),
    )
    return fig


def metrics_rows(strategy: dict, benchmark: dict) -> list[dict]:
    def pct(v: float) -> str:
        return f"{v * 100:.2f}%"

    return [
        {
            "metric": "Total return",
            "strategy": pct(strategy["total_return"]),
            "buy_and_hold": pct(benchmark["total_return"]),
        },
        {
            "metric": "Annualized return",
            "strategy": pct(strategy["annualized_return"]),
            "buy_and_hold": pct(benchmark["annualized_return"]),
        },
        {
            "metric": "Annualized volatility",
            "strategy": pct(strategy["annualized_volatility"]),
            "buy_and_hold": pct(benchmark["annualized_volatility"]),
        },
        {
            "metric": "Sharpe ratio",
            "strategy": f"{strategy['sharpe']:.2f}",
            "buy_and_hold": f"{benchmark['sharpe']:.2f}",
        },
        {
            "metric": "Max drawdown",
            "strategy": pct(-strategy["max_drawdown"]),
            "buy_and_hold": pct(-benchmark["max_drawdown"]),
        },
        {"metric": "Trades", "strategy": str(strategy["trades"]), "buy_and_hold": "1"},
    ]
