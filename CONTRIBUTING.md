# Contributing

## Workflow

`main` is protected: nobody pushes to it directly, every change goes through a pull request.

```bash
git switch -c feat/implied-vol-surface     # 1. branch off main
# ... code + tests ...
make format lint test                      # 2. run the same checks as CI locally
git commit -m "feat(engine): add implied vol surface"
git push -u origin feat/implied-vol-surface
# 3. open a PR, fill in the template, wait for green CI + 1 approval, then "Squash and merge"
```

### Branch names

| Prefix | Use for |
|---|---|
| `feat/` | new functionality |
| `fix/` | bug fixes |
| `refactor/` | restructuring without behaviour change |
| `test/` | tests only |
| `ci/` | GitHub Actions, Docker, Kubernetes |
| `docs/` | documentation |

### Commit messages — [Conventional Commits](https://www.conventionalcommits.org/)

```
<type>(<scope>): <short imperative summary>

feat(engine): add rolling Sharpe ratio
fix(api): return 422 when strike <= 0
test(engine): cover put greeks with finite differences
ci: cache GoogleTest build in CI
```

Typical scopes: `engine`, `api`, `dashboard`, `k8s`, `ci`.

## Code style

- **C++**: `.clang-format` (Google style, 4-space indent, 100 columns). Run `make format`.
- **Python**: `ruff` for linting and formatting, configured in `ruff.toml`.
- CI fails on unformatted code, so reviewers never have to comment on style.

## Code review

See [docs/CODE_REVIEW.md](docs/CODE_REVIEW.md) for the review process, branch protection settings and the reviewer checklist.
