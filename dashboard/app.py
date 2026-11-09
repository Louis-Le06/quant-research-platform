"""Quant Research Dashboard (Dash). Talks to the FastAPI service over HTTP."""

import os

from dash import Dash, Input, Output, State, dash_table, dcc, html

import figures
from api_client import ApiClient, ApiError

client = ApiClient()

app = Dash(__name__, title="Quant Research Platform")
server = app.server  # cho gunicorn: `gunicorn app:server`


@server.route("/healthz")
def healthz():
    return {"status": "ok"}


# ---------------------------------------------------------------- helpers
def number_input(label: str, id_: str, value, step="any", min_=None, max_=None):
    return html.Div(
        className="field",
        children=[
            html.Label(label, htmlFor=id_),
            dcc.Input(
                id=id_, type="number", value=value, step=step, min=min_, max=max_, debounce=True
            ),
        ],
    )


def stat_card(label: str, value: str):
    return html.Div(className="stat", children=[html.Span(label), html.Strong(value)])


# ----------------------------------------------------------------- layout
option_tab = html.Div(
    className="tab-body",
    children=[
        html.Div(
            className="panel controls",
            children=[
                html.H3("European option"),
                dcc.RadioItems(
                    id="opt-type",
                    value="call",
                    inline=True,
                    options=[
                        {"label": " Call", "value": "call"},
                        {"label": " Put", "value": "put"},
                    ],
                ),
                number_input("Spot", "opt-spot", 100, min_=0.01),
                number_input("Strike", "opt-strike", 100, min_=0.01),
                number_input("Volatility (%)", "opt-vol", 20, min_=0.1, max_=500),
                number_input("Risk-free rate (%)", "opt-rate", 5),
                number_input("Dividend yield (%)", "opt-div", 0, min_=0),
                number_input("Time to expiry (years)", "opt-time", 1, min_=0.01),
            ],
        ),
        html.Div(
            className="panel results",
            children=[
                html.Div(id="opt-error", className="error"),
                html.Div(id="opt-greeks", className="stats"),
                dcc.Graph(id="opt-chart", config={"displaylogo": False}),
            ],
        ),
    ],
)

backtest_tab = html.Div(
    className="tab-body",
    children=[
        html.Div(
            className="panel controls",
            children=[
                html.H3("Simulated market (GBM)"),
                number_input("Drift μ (%/yr)", "bt-mu", 8),
                number_input("Volatility σ (%/yr)", "bt-sigma", 20, min_=0),
                number_input("Trading days", "bt-steps", 756, step=1, min_=60, max_=10000),
                number_input("Random seed", "bt-seed", 42, step=1, min_=0),
                html.H3("SMA crossover strategy"),
                number_input("Fast window", "bt-fast", 20, step=1, min_=1),
                number_input("Slow window", "bt-slow", 50, step=1, min_=2),
                number_input("Cost (bps / trade)", "bt-cost", 5, min_=0),
                html.Button("Run backtest", id="bt-run", n_clicks=0, className="primary"),
            ],
        ),
        html.Div(
            className="panel results",
            children=[
                html.Div(id="bt-error", className="error"),
                dash_table.DataTable(
                    id="bt-metrics",
                    columns=[
                        {"name": "Metric", "id": "metric"},
                        {"name": "Strategy", "id": "strategy"},
                        {"name": "Buy & hold", "id": "buy_and_hold"},
                    ],
                    style_cell={"fontFamily": "inherit", "padding": "6px 10px"},
                    style_header={"fontWeight": "600"},
                ),
                dcc.Graph(id="bt-price-chart", config={"displaylogo": False}),
                dcc.Graph(id="bt-equity-chart", config={"displaylogo": False}),
            ],
        ),
    ],
)

app.layout = html.Div(
    className="page",
    children=[
        html.Header(
            children=[
                html.H1("Quant Research Platform"),
                html.P(id="api-info", className="muted"),
            ]
        ),
        dcc.Tabs(
            children=[
                dcc.Tab(label="Option Pricer", children=option_tab),
                dcc.Tab(label="Strategy Backtest", children=backtest_tab),
            ]
        ),
        dcc.Interval(id="once", interval=500, max_intervals=1),
    ],
)


# -------------------------------------------------------------- callbacks
@app.callback(Output("api-info", "children"), Input("once", "n_intervals"))
def show_api_info(_):
    v = client.version()
    if not v:
        return f"API: {client.base_url} (unreachable)"
    return (
        f"API {v.get('version')} · engine {v.get('engine_version')} · served by pod {v.get('pod')}"
    )


@app.callback(
    Output("opt-greeks", "children"),
    Output("opt-chart", "figure"),
    Output("opt-error", "children"),
    Input("opt-type", "value"),
    Input("opt-spot", "value"),
    Input("opt-strike", "value"),
    Input("opt-vol", "value"),
    Input("opt-rate", "value"),
    Input("opt-div", "value"),
    Input("opt-time", "value"),
)
def update_option(opt_type, spot, strike, vol, rate, div, time):
    if None in (spot, strike, vol, rate, div, time):
        return [], {}, "Please fill in all fields."
    params = dict(
        spot=spot,
        strike=strike,
        vol=vol / 100,
        rate=rate / 100,
        dividend=div / 100,
        time=time,
        type=opt_type,
    )
    try:
        g = client.greeks(**params)
        curve = client.curve(
            **params, spot_min=max(spot * 0.5, 0.01), spot_max=spot * 1.5, points=121
        )
    except ApiError as exc:
        return [], {}, str(exc)
    cards = [
        stat_card("Price", f"{g['price']:.4f}"),
        stat_card("Delta", f"{g['delta']:.4f}"),
        stat_card("Gamma", f"{g['gamma']:.4f}"),
        stat_card("Vega (1%)", f"{g['vega'] / 100:.4f}"),
        stat_card("Theta (1 day)", f"{g['theta'] / 365:.4f}"),
        stat_card("Rho (1%)", f"{g['rho'] / 100:.4f}"),
    ]
    return cards, figures.option_curve_figure(curve, strike, opt_type), ""


@app.callback(
    Output("bt-metrics", "data"),
    Output("bt-price-chart", "figure"),
    Output("bt-equity-chart", "figure"),
    Output("bt-error", "children"),
    Input("bt-run", "n_clicks"),
    State("bt-mu", "value"),
    State("bt-sigma", "value"),
    State("bt-steps", "value"),
    State("bt-seed", "value"),
    State("bt-fast", "value"),
    State("bt-slow", "value"),
    State("bt-cost", "value"),
)
def run_backtest(_, mu, sigma, steps, seed, fast, slow, cost):
    if None in (mu, sigma, steps, seed, fast, slow, cost):
        return [], {}, {}, "Please fill in all fields."
    try:
        prices = client.simulate(mu=mu / 100, sigma=sigma / 100, steps=int(steps), seed=int(seed))
        bt = client.backtest(prices, int(fast), int(slow), cost)
        bench = client.metrics(prices)
    except ApiError as exc:
        return [], {}, {}, str(exc)
    return (
        figures.metrics_rows(bt, bench),
        figures.price_signals_figure(prices, bt, int(fast), int(slow)),
        figures.equity_figure(bt),
        "",
    )


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=int(os.getenv("PORT", "8050")), debug=False)
