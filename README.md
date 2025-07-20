# Claude Draw

A Python library for creating 2D vector graphics with an intuitive API and strong type safety.

## Features

### ✅ Implemented
- **Core Data Models**: Point2D, Color (RGB/HSL/Hex), Transform2D, BoundingBox
- **Basic Shapes**: Circle, Rectangle, Ellipse, Line with full geometric operations
- **Container System**: Groups, Layers with blend modes, Drawing canvas
- **Immutable Design**: Pydantic-based models with validation
- **Type Safety**: Full type hints and runtime validation
- **Transformations**: Translate, rotate, scale operations
- **Factory Functions**: Convenient shape creation utilities

### 🚧 In Development
- Path drawing with SVG-like commands
- Complex shapes (Polygon, Polyline, Arc)
- Styling system with gradients and patterns
- Text rendering with font support
- Visitor pattern renderer architecture
- SVG export functionality
- Serialization support

### 📋 Planned
- PNG and PDF export
- Performance optimizations
- Documentation and examples

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
from claude_draw import Drawing, Layer, Group, Circle, Rectangle
from claude_draw.models.point import Point2D
from claude_draw.models.color import Color

# Create a drawing canvas
drawing = Drawing(width=400, height=300, title="My Drawing")

# Create shapes with styling
circle = Circle(
    center=Point2D(x=200, y=150), 
    radius=50,
    fill=Color.from_hex("#3498db"),
    stroke=Color.from_hex("#2c3e50"),
    stroke_width=2
)

rectangle = Rectangle(
    x=50, y=50, 
    width=100, height=80,
    fill=Color.from_rgb(231, 76, 60),
    opacity=0.8
)

# Organize with layers and groups
background_layer = Layer(name="background", z_index=1)
foreground_layer = Layer(name="foreground", z_index=2)

shapes_group = Group(name="shapes")
shapes_group = shapes_group.add_child(circle).add_child(rectangle)

# Build the hierarchy
drawing = drawing.add_child(background_layer).add_child(foreground_layer)
foreground_layer = foreground_layer.add_child(shapes_group)

# Apply transformations
rotated_circle = circle.rotate(0.785)  # 45 degrees
scaled_rect = rectangle.scale(1.5, 1.2)
```

## Architecture

The library follows a hierarchical, immutable design:

```
Drawing (Canvas)
├── Layer (with blend modes, opacity)
│   ├── Group (organizational container)
│   │   ├── Circle, Rectangle, Ellipse, Line
│   │   └── Nested Groups
│   └── Direct shapes
└── Multiple Layers (z-index ordered)
```

## Current API Examples

### Basic Shapes
```python
from claude_draw import Circle, Rectangle, Ellipse, Line
from claude_draw.models.point import Point2D

# Circle
circle = Circle(center=Point2D(x=100, y=100), radius=50)

# Rectangle  
rect = Rectangle(x=0, y=0, width=200, height=100)

# Ellipse
ellipse = Ellipse(center=Point2D(x=150, y=75), rx=80, ry=40)

# Line
line = Line(start=Point2D(x=0, y=0), end=Point2D(x=100, y=100))
```

### Factory Functions
```python
from claude_draw.factories import (
    create_circle, create_rectangle, create_square, 
    create_ellipse, create_line
)

# Convenient creation methods
circle = create_circle(100, 100, 50)
square = create_square(0, 0, 100)
line = create_horizontal_line(y=50, x_start=0, x_end=200)
```

## Development Status

This library is currently in active development. The core foundation is solid with:

- ✅ **33% Complete**: 4 of 12 major tasks implemented
- ✅ **Type-Safe Foundation**: Pydantic v2 with immutable models
- ✅ **Test Coverage**: 60% overall, 93% for containers
- ✅ **Modern Python**: 3.12+ with full type hints

### Running Tests

```bash
# Activate virtual environment
python3 -m venv venv
source venv/bin/activate

# Install dependencies
pip install -e ".[dev]"

# Run tests
pytest tests/ -v --cov=src/claude_draw
```

## Architecture Design

The library uses several key design patterns:

- **Immutable Objects**: All models use Pydantic's frozen configuration
- **Visitor Pattern**: Extensible rendering through visitor implementations  
- **Factory Pattern**: Convenient object creation with validation
- **Hierarchical Containers**: Clean organization with Drawing → Layer → Group → Shape
- **Transform Composition**: Matrix-based transformations with proper inheritance

## Documentation

Full documentation will be available at [https://claude-draw.readthedocs.io](https://claude-draw.readthedocs.io)

For now, see:
- API examples in this README
- Docstrings in source code
- Test files for usage patterns

## Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

Key areas needing work:
- Visitor pattern renderer implementation
- SVG export functionality
- Complex shape primitives (Path, Polygon)
- Styling system with gradients
- Text rendering support

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.