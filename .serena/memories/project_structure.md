# Project Structure

## Root Directory
- `src/claude_draw/`: Main Python package
  - `models/`: Core data models (Point2D, Color, Transform2D, etc.)
  - `__init__.py`: Package exports
  - `base.py`: Base classes (Drawable, Container, etc.)
  - `shapes.py`: Shape implementations (Circle, Rectangle, etc.)
  - `containers.py`: Container classes (Group, Layer, Drawing)
  - `renderers.py`: SVG renderer and bounding box calculator
  - `visitors.py`: Visitor pattern implementation
  - `factories.py`: Factory functions for shape creation
  - `serialization.py`: JSON serialization/deserialization
  - `protocols.py`: Protocol definitions

- `tests/`: Python test suite
  - Mirrors src/ structure
  - Uses pytest framework

- `cpp/`: C++ backend (optional)
  - `include/claude_draw/`: Header files
  - `src/`: Implementation files
  - `tests/`: C++ unit tests
  - `benchmarks/`: Performance benchmarks
  - `Makefile`: Build system

- `benchmarks/`: Python benchmarks
- `examples/`: Usage examples
- `docs/`: Documentation

## Key Files
- `pyproject.toml`: Python package configuration
- `CLAUDE.md`: AI assistant instructions
- `README.md`: Project documentation
- `.pre-commit-config.yaml`: Pre-commit hooks configuration