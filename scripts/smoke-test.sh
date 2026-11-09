#!/usr/bin/env bash
# Quick end-to-end check after a deployment.
# Usage: ./scripts/smoke-test.sh <API_URL> [DASHBOARD_URL]
set -euo pipefail

API="${1:-http://localhost:8000}"
DASH="${2:-}"
pass=0; fail=0

check() {  # check <description> <expected HTTP code> <curl args...>
  local desc="$1" expected="$2"; shift 2
  local out code body
  out=$(curl -s --max-time 10 -w '\n%{http_code}' "$@" || true)
  code=$(tail -n1 <<<"$out"); body=$(sed '$d' <<<"$out" | tr -d '\n' | cut -c1-100)
  if [[ "$code" == "$expected" ]]; then
    printf '  ✔ %-34s [%s] %s\n' "$desc" "$code" "$body"; pass=$((pass+1))
  else
    printf '  ✘ %-34s expected %s got %s — %s\n' "$desc" "$expected" "$code" "$body"; fail=$((fail+1))
  fi
}

json() { curl -s --max-time 10 -H 'Content-Type: application/json' "$@"; }

echo "== API: $API"
check "Health"                          200 "$API/health"
check "Version"                         200 "$API/version"
check "Black-Scholes greeks"            200 -H 'Content-Type: application/json' \
      -d '{"spot":100,"strike":100,"rate":0.05,"vol":0.2,"time":1,"type":"call"}' "$API/options/greeks"
check "Implied vol"                     200 -H 'Content-Type: application/json' \
      -d '{"price":10.4506,"spot":100,"strike":100,"rate":0.05,"time":1}' "$API/options/implied-vol"
check "Invalid input -> 422"            422 -H 'Content-Type: application/json' \
      -d '{"spot":-1,"strike":100,"time":1}' "$API/options/greeks"

PRICES=$(json -d '{"steps":300,"seed":7}' "$API/data/simulate" | python3 -c 'import json,sys;print(json.dumps(json.load(sys.stdin)["prices"]))')
check "Analytics metrics"               200 -H 'Content-Type: application/json' \
      -d "{\"prices\":$PRICES}" "$API/analytics/metrics"
check "SMA crossover backtest"          200 -H 'Content-Type: application/json' \
      -d "{\"prices\":$PRICES,\"fast\":10,\"slow\":50}" "$API/backtest/sma-crossover"

if [[ -n "$DASH" ]]; then
  echo "== Dashboard: $DASH"
  check "Dashboard health"              200 "$DASH/healthz"
  check "Dashboard page"                200 "$DASH/"
fi

echo "Result: $pass passed, $fail failed"
[[ $fail -eq 0 ]]
