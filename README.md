# Claude Draw

A Python library for creating 2D vector graphics with an intuitive API and strong type safety.

## Features

### ✅ Implemented
- **Core Data Models**: Point2D, Color (RGB/HSL/Hex), Transform2D, BoundingBox
- **Basic Shapes**: Circle, Rectangle, Ellipse, Line with full geometric operations
- **Container System**: Groups, Layers with blend modes, Drawing canvas
- **Immutable Design**: Pydantic-based models with validation
- **Type Safety**: Full type hints and runtime validation
- **Transformations**: Translate, rotate, scale operations with fluent API
- **Factory Functions**: Convenient shape creation utilities
- **Visitor Pattern**: Complete renderer architecture with extensible visitors
- **SVG Export**: Full SVG rendering with transformations and styling
- **Serialization**: Enhanced JSON serialization with type discrimination
- **Polymorphic Support**: Save/load complex drawings with full type safety

### 🚧 In Development
- Path drawing with SVG-like commands
- Complex shapes (Polygon, Polyline, Arc)
- Styling system with gradients and patterns
- Text rendering with font support

### 📋 Planned
- PNG and PDF export
- **C++ Performance Layer**: 50-100x speedup for millions of objects (see [C++ Architecture](cpp_architecture.md))
- GPU acceleration
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
from claude_draw.renderers import SVGRenderer
from claude_draw.serialization import save_drawable, load_drawable

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

# Apply transformations with fluent API
rotated_circle = circle.rotate(0.785).scale(1.2)  # Chain operations
scaled_rect = rectangle.scale(1.5, 1.2).translate(20, 10)

# Render to SVG
renderer = SVGRenderer()
svg_output = renderer.render(drawing)
print(svg_output)

# Save and load drawings
save_drawable(drawing, "my_drawing.json")
loaded_drawing = load_drawable("my_drawing.json")
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

### Visitor Pattern & Rendering
```python
from claude_draw.renderers import SVGRenderer, BoundingBoxCalculator
from claude_draw.visitors import BaseRenderer

# SVG Export
renderer = SVGRenderer(width=800, height=600)
svg_content = renderer.render(drawing)

# Calculate bounds
bounds_calc = BoundingBoxCalculator()
bounds_calc.render(drawing)
x, y, width, height = bounds_calc.get_bounding_box()

# Custom renderer example
class MyRenderer(BaseRenderer):
    def visit_circle(self, circle):
        print(f"Processing circle at {circle.center}")
    
    def visit_rectangle(self, rectangle):
        print(f"Processing rectangle {rectangle.width}x{rectangle.height}")
```

### Serialization & Persistence
```python
from claude_draw.serialization import (
    serialize_drawable, deserialize_drawable,
    save_drawable, load_drawable
)

# JSON serialization with type safety
json_str = serialize_drawable(drawing)
restored_drawing = deserialize_drawable(json_str)

# File persistence
save_drawable(drawing, "complex_artwork.json")
loaded_drawing = load_drawable("complex_artwork.json")

# All object relationships and types are preserved
assert type(loaded_drawing) == Drawing
assert type(loaded_drawing.children[0]) == Layer

## Development Status

This library is currently in active development. The core foundation is solid with:

- ✅ **75% Complete**: 9 of 12 major tasks implemented
- ✅ **Type-Safe Foundation**: Pydantic v2 with immutable models
- ✅ **Test Coverage**: 89% overall, 373 tests passing
- ✅ **Modern Python**: 3.12+ with full type hints
- ✅ **Visitor Pattern**: Complete architecture for extensible rendering
- ✅ **SVG Export**: Full featured SVG generation with styling
- ✅ **Serialization**: Robust JSON persistence with type safety

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
- **Type Discrimination**: Polymorphic serialization with automatic type resolution
- **Fluent API**: Method chaining for intuitive object manipulation

## Documentation

Full documentation will be available at [https://claude-draw.readthedocs.io](https://claude-draw.readthedocs.io)

For now, see:
- API examples in this README
- Docstrings in source code
- Test files for usage patterns

## Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

Key areas needing work:

- Complex shape primitives (Path, Polygon, Arc)
- Styling system with gradients and patterns
- Text rendering support
- PNG and PDF export capabilities
- Performance optimizations and benchmarking

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.