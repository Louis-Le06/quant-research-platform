# Linux & Ops Runbook

Linux, Docker and kubectl commands used to run and debug this project, grouped by task.

## System and processes

```bash
uname -a; cat /etc/os-release        # OS details
nproc; free -h; df -h                # CPU, RAM, disk (Minikube needs >= 2 CPUs, 4 GB RAM)
ps aux | grep -E 'uvicorn|gunicorn'  # running services
ss -tlnp | grep -E '8000|8050'       # which process is listening on which port
kill -TERM <pid>                     # graceful stop (Kubernetes also sends SIGTERM)
```

## Networking and HTTP

```bash
curl -s localhost:8000/health
curl -s -X POST localhost:8000/options/greeks \
     -H 'Content-Type: application/json' \
     -d '{"spot":100,"strike":100,"time":1}' | python3 -m json.tool
curl -sI localhost:8050/                 # headers only
```

## Files, permissions, logs

```bash
chmod +x scripts/*.sh                    # make scripts executable
ls -l scripts/                           # check rwx permissions
find engine -name '*.cpp' | xargs wc -l  # count C++ lines
grep -rn "TODO" engine api dashboard     # find TODOs
tail -f /var/log/syslog                  # follow system log
journalctl -u docker --since "10 min ago"
```

## Docker

```bash
docker build -f api/Dockerfile -t quant-api .
docker images | grep quant                            # compare image sizes
docker run --rm -p 8000:8000 quant-api
docker ps; docker logs -f <container>
docker exec -it <container> sh                        # shell into a container
docker inspect --format '{{.Config.User}}' quant-api  # confirm non-root (10001)
docker system df; docker system prune                 # clean up
```

## Kubernetes / Minikube

```bash
minikube start --driver=docker --cpus=2 --memory=4096
kubectl get all -n quant -o wide
kubectl describe pod <pod> -n quant              # Events explain why a pod is not starting
kubectl logs -f deploy/quant-api -n quant
kubectl exec -it deploy/quant-api -n quant -- sh
kubectl port-forward svc/quant-api 8000:8000 -n quant
kubectl scale deploy/quant-api --replicas=4 -n quant
kubectl rollout restart deploy/quant-api -n quant && kubectl rollout status deploy/quant-api -n quant
kubectl rollout undo deploy/quant-api -n quant   # roll back
kubectl top pods -n quant                        # needs: minikube addons enable metrics-server
minikube dashboard
```

## Troubleshooting

| Symptom | Usual cause | Check / fix |
|---|---|---|
| Pod `ErrImagePull` / `ImagePullBackOff` | Image not loaded into Minikube | `minikube image ls \| grep quant`, then `minikube image load quant-api:latest` |
| Pod `CrashLoopBackOff` | App fails on start-up | `kubectl logs <pod> --previous` |
| Pod `Running` but `0/1 READY` | Readiness probe failing | `kubectl describe pod` → Events |
| Dashboard shows "API unreachable" | Wrong `API_URL` or Service | `kubectl get svc,endpoints -n quant` |
| `Permission denied` running a script | Missing execute bit | `chmod +x scripts/*.sh` |
| Pod `OOMKilled` | Exceeded `limits.memory` | `kubectl describe pod`; raise the limit in `k8s/*.yaml` |
