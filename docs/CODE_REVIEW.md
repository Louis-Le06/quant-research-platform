# Code Review Process

Most mechanical checks are **automated**, so human review can focus on correctness and design rather than formatting.

## 1. What the machines check (before a human looks)

| Check | Tool | Where |
|---|---|---|
| C++ formatting | clang-format | CI job `lint` |
| C++ static analysis | cppcheck, `-Wall -Wextra -Wpedantic -Wconversion` | CI jobs `lint`, `engine` |
| Python lint + format | ruff | CI job `lint` |
| C++ unit tests | GoogleTest (GCC and Clang) | CI job `engine` |
| Python tests | pytest | CI job `python` |
| Images still build | Docker Buildx | CI job `docker-build` |
| Who must review | CODEOWNERS | `.github/CODEOWNERS` |
| Outdated dependencies | Dependabot | `.github/dependabot.yml` |

## 2. Branch protection (one-off setup on GitHub)

**Settings → Branches → Add branch ruleset** (or *Add classic branch protection rule*) for `main`:

1. ✅ **Require a pull request before merging**
   - Required approvals: **1**
   - ✅ Dismiss stale pull request approvals when new commits are pushed
   - ✅ Require review from Code Owners
2. ✅ **Require status checks to pass** — select:
   `Lint & format`, `C++ engine — GoogleTest (gcc)`, `C++ engine — GoogleTest (clang)`,
   `Python — pytest (API + dashboard)`, `Docker build check (api)`, `Docker build check (dashboard)`
   *(checks only appear in the list after CI has run at least once)*
3. ✅ **Require conversation resolution before merging**
4. ✅ **Block force pushes**

> Working alone? GitHub does not let you approve your own PR. Add a classmate as a collaborator and ask them to review, or set *Required approvals = 0* while keeping the status checks, and self-review using the checklist below, leaving your findings as PR comments.

## 3. Reviewer checklist

**Correctness**
- [ ] Logic matches the PR description; edge cases (empty, negative, NaN, zero) are handled
- [ ] Time-series code never uses future data (look-ahead bias)
- [ ] Units are explicit (annual vs. percent volatility? time in years or days?)
- [ ] Bad input produces HTTP 422, never crashes the service

**Tests**
- [ ] New behaviour is tested, and the test would fail without the new code
- [ ] Floating-point assertions use tolerances (`EXPECT_NEAR`, `pytest.approx`), never `==`

**Design & maintainability**
- [ ] Business logic lives in `engine/`; the API stays a thin layer
- [ ] Clear names; no dead code or redundant comments
- [ ] No new dependency unless it is clearly justified

**Operations**
- [ ] New env vars or ports are reflected in `k8s/` and `docker-compose.yml`
- [ ] Containers still run as non-root without writing to the root filesystem

## 4. Comment conventions

Prefix comments so the author knows how much each one matters:

| Prefix | Meaning |
|---|---|
| **blocker:** | must be fixed before merging |
| **suggestion:** | worth changing, open to discussion |
| **nit:** | minor detail, author's call |
| **question:** | asking to understand, no change requested |

Use GitHub's **Suggest changes** for small fixes so the author can commit them in one click.

## 5. Creating review evidence

To build real review history, open 3–4 small PRs from this list, get them reviewed (or self-review against the checklist), respond to the comments and merge:

1. `feat(engine): add Sortino ratio` — new function + GoogleTest + API endpoint
2. `fix(api): reject non-positive prices in analytics with a clear message` — fix with a regression test
3. `feat(dashboard): add rolling volatility chart` — uses `rolling_volatility`, already returned by the API
4. `ci: add code coverage report (gcovr)` — infrastructure change

Screenshot a PR page with review comments, its green *Checks* tab, and the merge history.
