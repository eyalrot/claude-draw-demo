# Development Commands

## Python Environment Setup
```bash
python3 -m venv venv
source venv/bin/activate
pip install -e ".[dev]"
```

## Python Development
- **Run tests**: `pytest tests/ -v --cov=src/claude_draw`
- **Format code**: `black src/ tests/`
- **Lint code**: `ruff check src/ tests/`
- **Type check**: `mypy src/`
- **Pre-commit hooks**: `pre-commit run --all-files`
- **Run benchmarks**: `python benchmarks/run_benchmarks.py`

## C++ Development (in cpp/ directory)
- **Build Release**: `make` or `make release`
- **Build Debug**: `make debug`
- **Run tests**: `make test`
- **Run benchmarks**: `make bench`
- **Format C++ code**: `make format`
- **Lint C++ code**: `make lint`
- **Generate coverage**: `make coverage-html`
- **Run all checks**: `make check`
- **Clean build**: `make clean`

## Combined Workflow
- **Full check**: `make check` (in cpp/) + `pytest` (in root)
- **Quick rebuild**: `make quick` (C++ debug rebuild)

## System Commands
- `git status`, `git diff`, `git log`
- `ls`, `cd`, `grep` (use ripgrep `rg` if available)
- `find` for file searching