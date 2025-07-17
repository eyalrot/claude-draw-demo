# Claude Draw

A Python library for creating 2D vector graphics with an intuitive API.

## Features

- Simple and intuitive API for creating vector graphics
- Support for basic shapes (rectangles, circles, ellipses, polygons)
- Path drawing with SVG-like commands
- Transformations (translate, rotate, scale)
- Styling with colors, strokes, and fills
- Export to multiple formats (SVG, PNG, PDF)
- Type-safe with full type hints
- Zero dependencies for core functionality

## Installation

```bash
pip install claude-draw
```

For development:

```bash
git clone https://github.com/yourusername/claude-draw.git
cd claude-draw
pip install -e ".[dev]"
```

## Quick Start

```python
from claude_draw import Canvas, Circle, Rectangle

# Create a canvas
canvas = Canvas(width=400, height=300)

# Add shapes
canvas.add(Circle(x=200, y=150, radius=50, fill="blue"))
canvas.add(Rectangle(x=50, y=50, width=100, height=80, fill="red"))

# Save the result
canvas.save("my_drawing.svg")
```

## Documentation

Full documentation is available at [https://claude-draw.readthedocs.io](https://claude-draw.readthedocs.io)

## Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.