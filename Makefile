# Common tasks. Run `make help` for the list.
.PHONY: help engine test-engine install-engine test-api test-dashboard test format lint \
        run-api run-dashboard compose deploy smoke clean

PY ?= python3

help:  ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*?## ' $(MAKEFILE_LIST) | awk 'BEGIN{FS=":.*?## "}{printf "  \033[36m%-16s\033[0m %s\n",$$1,$$2}'

engine:  ## Build C++ engine + test binary
	cmake -S engine -B engine/build -DCMAKE_BUILD_TYPE=Release
	cmake --build engine/build -j

test-engine: engine  ## Run GoogleTest
	./engine/build/engine_tests

install-engine:  ## Build and install the quant_engine Python module (pybind11)
	$(PY) -m pip install ./engine

test-api:  ## Run API tests
	$(PY) -m pytest api/tests -v

test-dashboard:  ## Run dashboard tests
	$(PY) -m pytest dashboard/tests -v

test: test-engine test-api test-dashboard  ## Run all tests

format:  ## Auto-format C++ and Python
	find engine \( -name '*.cpp' -o -name '*.hpp' \) -not -path '*/build/*' | xargs clang-format -i
	ruff format api dashboard

lint:  ## Lint and format checks, same as CI
	find engine \( -name '*.cpp' -o -name '*.hpp' \) -not -path '*/build/*' | xargs clang-format --dry-run --Werror
	cppcheck --enable=warning,performance,portability --std=c++17 --error-exitcode=1 -q -I engine/include engine/src
	ruff check api dashboard
	ruff format --check api dashboard

run-api:  ## Run API on http://localhost:8000 (docs at /docs)
	cd api && $(PY) -m uvicorn app.main:app --reload --port 8000

run-dashboard:  ## Run dashboard on http://localhost:8050
	cd dashboard && API_URL=http://localhost:8000 $(PY) app.py

compose:  ## Run the whole stack with Docker Compose
	docker compose up --build

deploy:  ## Build images and deploy to Minikube
	bash scripts/deploy-minikube.sh

smoke:  ## Smoke-test the local stack
	bash scripts/smoke-test.sh http://localhost:8000 http://localhost:8050

clean:  ## Remove build artefacts
	rm -rf engine/build engine/build-* **/__pycache__ .pytest_cache
