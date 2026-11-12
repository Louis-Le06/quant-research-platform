#!/usr/bin/env bash
# Build both images, load them into Minikube, deploy, print status and smoke-test.
# Requires: docker, minikube, kubectl. Run from anywhere: ./scripts/deploy-minikube.sh
set -euo pipefail
cd "$(dirname "$0")/.."

NS=quant
step() { printf '\n\033[1;34m==> %s\033[0m\n' "$*"; }

step "1. Start Minikube (if not running)"
minikube status >/dev/null 2>&1 || minikube start --driver=docker --cpus=2 --memory=4096

step "2. Build Docker images"
docker build -f api/Dockerfile       -t quant-api:latest .
docker build -f dashboard/Dockerfile -t quant-dashboard:latest .

step "3. Load images into Minikube"
minikube image load quant-api:latest
minikube image load quant-dashboard:latest

step "4. Apply Kubernetes manifests"
kubectl apply -k k8s/
kubectl -n "$NS" rollout restart deployment/quant-api deployment/quant-dashboard
kubectl -n "$NS" rollout status deployment/quant-api --timeout=180s
kubectl -n "$NS" rollout status deployment/quant-dashboard --timeout=180s

step "5. Cluster status  (📸 screenshot this)"
kubectl get nodes -o wide
kubectl -n "$NS" get deploy,rs,pods,svc,cm -o wide

step "6. Smoke test"
kubectl -n "$NS" port-forward svc/quant-api 18000:8000 >/dev/null 2>&1 &
PF_API=$!
kubectl -n "$NS" port-forward svc/quant-dashboard 18050:8050 >/dev/null 2>&1 &
PF_DASH=$!
trap 'kill $PF_API $PF_DASH 2>/dev/null || true' EXIT
sleep 3
bash scripts/smoke-test.sh http://127.0.0.1:18000 http://127.0.0.1:18050

step "Done! Open the dashboard with:"
echo "   kubectl -n $NS port-forward svc/quant-dashboard 8050:8050   ->  http://localhost:8050"