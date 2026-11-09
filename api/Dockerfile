# Build context: repository root  ->  docker build -f api/Dockerfile -t quant-api .
# =====================================================================
# Stage 1: build the C++ engine -> run GoogleTest -> build the Python wheel
# =====================================================================
FROM python:3.12-slim AS engine-builder

RUN apt-get update \
 && apt-get install -y --no-install-recommends g++ cmake make git ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY engine/ ./engine/

# C++ unit tests run during the build: a failing test means no image
RUN cmake -S engine -B engine/build-tests -DCMAKE_BUILD_TYPE=Release \
 && cmake --build engine/build-tests -j"$(nproc)" \
 && ./engine/build-tests/engine_tests --gtest_brief=1 \
 && rm -rf engine/build-tests

RUN pip wheel --no-cache-dir --wheel-dir /wheels ./engine

# =====================================================================
# Stage 2: runtime — no compiler, runs as a non-root user
# =====================================================================
FROM python:3.12-slim

ARG APP_VERSION=1.0.0
LABEL org.opencontainers.image.title="quant-api" \
      org.opencontainers.image.description="FastAPI over a C++ quant engine" \
      org.opencontainers.image.version="${APP_VERSION}"

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    APP_VERSION=${APP_VERSION}

RUN groupadd --gid 10001 app && useradd --uid 10001 --gid app --no-create-home app

WORKDIR /srv
COPY api/requirements.txt .
COPY --from=engine-builder /wheels /wheels
RUN pip install --no-cache-dir -r requirements.txt /wheels/*.whl && rm -rf /wheels

COPY api/app ./app

USER 10001
EXPOSE 8000
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s \
  CMD python -c "import urllib.request,sys; sys.exit(0 if urllib.request.urlopen('http://127.0.0.1:8000/health').status==200 else 1)"
CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000", "--workers", "2"]
