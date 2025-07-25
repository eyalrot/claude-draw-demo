# Code Style and Conventions

## Python Code Style
- **Follow PEP8** strictly
- **Use type hints** everywhere
- **Format with Black** (line length: 88)
- **Lint with Ruff** (pycodestyle, pyflakes, isort, flake8 rules)
- **Type check with mypy** (strict mode)
- **Pydantic v2** for all data models with validation

## Docstring Style
Google style docstrings:
```python
def example(param1: int) -> str:
    """
    Brief summary.

    Args:
        param1: Description.

    Returns:
        Description.
    """
```

## Code Organization
- Keep files under 500 lines
- Organize by feature/responsibility
- Prefer relative imports within packages
- Use clear, consistent naming conventions

## Design Principles
- **KISS**: Keep It Simple, Stupid
- **YAGNI**: You Aren't Gonna Need It
- **Immutable objects** using Pydantic's frozen configuration
- **Visitor pattern** for extensible rendering
- **Factory pattern** for convenient object creation
- **Fluent API** for method chaining

## Testing Requirements
- Always create pytest unit tests for new features
- Tests in `/tests` folder mirroring app structure
- Include: expected use, edge case, failure case tests
- Maintain >80% code coverage
- Use pytest fixtures for test data