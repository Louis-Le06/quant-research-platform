# Reflection — Software Development Tools & DevOps

> **How to use this file:** this is a structured template, not a finished essay. Replace every `[ ... ]` with your own experience: what broke, how you diagnosed it, what you would do differently. Reflections are assessed on what *you* learned, so write in your own voice and delete these instruction lines before submitting. Target length: ~800–1200 words.

**Name:** [ ]  **Student ID:** [ ]  **Repository:** [GitHub link]

---

## 1. Context and goals

[2–3 sentences. What the project is (C++ quant engine + FastAPI + Dash on Kubernetes), and the two goals from your plan: enterprise toolchain practice (CI, unit testing, code review) and Docker/Kubernetes/Linux for IMC-style roles.]

## 2. Version control and code review

- [How did your Git workflow change once `main` was protected? Did anything feel slower at first?]
- [Describe one real review comment you received or wrote. What did it catch?]
- [What did automating format/lint in CI change about what reviewers focus on?]

## 3. Unit testing with GoogleTest

- [Which test caught a real bug in your code? What was the bug?]
  *Hint: implied-vol bounds, look-ahead in the backtest, Greeks sign conventions, off-by-one in SMA windows are common candidates.*
- [Why test Greeks against finite differences instead of hard-coded numbers? What does each approach guarantee?]
- [When did you use `TEST_F` vs `TEST_P`? Give one example of each from your repo.]
- [How did separating `engine/` from the API make testing easier?]

## 4. Continuous integration

- [What made the pipeline fail the first time, and how did you read the logs to fix it?]
- [Why build with both GCC and Clang? Did one of them report something the other didn't?]
- [How long does CI take? What did caching change?]

## 5. Docker

- [Compare image sizes (`docker images`) of a single-stage vs multi-stage build. What is left out of the runtime image and why does that matter?]
- [Why run GoogleTest inside the Docker build?]
- [What changed when the container ran as a non-root user with a read-only filesystem? Anything break?]

## 6. Kubernetes and Minikube

- [Explain Deployment, ReplicaSet, Pod, Service and ConfigMap in your own words, using this project.]
- [Delete an API pod (`kubectl delete pod ...`) while the dashboard is open. What happened and why?]
- [Readiness vs liveness probes: what's the difference, and what would go wrong without them?]
- [Describe one deployment problem you hit (ImagePullBackOff, CrashLoopBackOff, wrong API_URL...) and how you diagnosed it.]

## 7. Linux

- [Which commands from `docs/LINUX_RUNBOOK.md` did you actually need? Which one saved you the most time?]

## 8. Connecting to the target role (IMC)

[How do these practices map to a trading firm's engineering? E.g. reproducible builds, tested pricing code, safe rollouts, low-latency C++ alongside Python research tooling.]

## 9. Self-assessment

| Area | Before (1–5) | After (1–5) | Evidence |
|---|---|---|---|
| Git & code review | [ ] | [ ] | [PR link] |
| Unit testing | [ ] | [ ] | [CI run link] |
| CI/CD | [ ] | [ ] | [Actions link] |
| Docker | [ ] | [ ] | [Dockerfile] |
| Kubernetes | [ ] | [ ] | [screenshot] |
| Linux | [ ] | [ ] | [ ] |

**Hours actually spent vs plan:** Goal 3 [ ] / 40–50 h · Goal 4 [ ] / 50 h

**What I'd do differently / next steps:** [ ]
