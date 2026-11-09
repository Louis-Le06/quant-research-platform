# Build context: repository root  ->  docker build -f dashboard/Dockerfile -t quant-dashboard .
FROM python:3.12-slim

ARG APP_VERSION=1.0.0
LABEL org.opencontainers.image.title="quant-dashboard" \
      org.opencontainers.image.description="Dash UI for the Quant Research Platform" \
      org.opencontainers.image.version="${APP_VERSION}"

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    API_URL=http://quant-api:8000

RUN groupadd --gid 10001 app && useradd --uid 10001 --gid app --no-create-home app

WORKDIR /srv
COPY dashboard/requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY dashboard/app.py dashboard/api_client.py dashboard/figures.py ./
COPY dashboard/assets ./assets

USER 10001
EXPOSE 8050
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s \
  CMD python -c "import urllib.request,sys; sys.exit(0 if urllib.request.urlopen('http://127.0.0.1:8050/healthz').status==200 else 1)"
# --no-control-socket: gunicorn >= 26 writes a control socket under $HOME, which fails
# with readOnlyRootFilesystem; --worker-tmp-dir points at the writable /tmp emptyDir.
CMD ["gunicorn", "--bind", "0.0.0.0:8050", "--workers", "2", "--timeout", "60", \
     "--no-control-socket", "--worker-tmp-dir", "/tmp", "app:server"]
