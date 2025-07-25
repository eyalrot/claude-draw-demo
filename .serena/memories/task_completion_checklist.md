# Task Completion Checklist

When completing any coding task, ensure to:

## Python Tasks
1. **Format code**: `black src/ tests/`
2. **Lint code**: `ruff check src/ tests/` (fix any issues)
3. **Type check**: `mypy src/` (ensure no type errors)
4. **Run tests**: `pytest tests/ -v`
5. **Check coverage**: Ensure >80% coverage
6. **Update tests** if logic changed
7. **Update README.md** if features added

## C++ Tasks (in cpp/ directory)
1. **Format code**: `make format`
2. **Check formatting**: `make format-check`
3. **Lint code**: `make lint`
4. **Run tests**: `make test`
5. **Run benchmarks**: `make bench` (if performance-critical)
6. **Full check**: `make check`

## General
- Commit only when explicitly asked
- Follow existing code patterns and conventions
- Write tests for new features
- Add docstrings for all functions
- Comment non-obvious code with `# Reason:` comments
- Never create files longer than 500 lines