# Claude Draw Architecture

## Overview

Claude Draw is a modern Python library for creating 2D vector graphics with a focus on type safety, immutability, and extensibility. The architecture follows several key design patterns to provide a robust, maintainable, and extensible foundation for graphics programming.

## Core Design Principles

### 1. Immutability
All objects in Claude Draw are immutable by design, using Pydantic's `frozen=True` configuration. This provides:
- **Thread Safety**: Objects can be safely shared across threads
- **Predictable Behavior**: No unexpected mutations
- **Copy-on-Write Semantics**: New instances are created for modifications
- **Functional Programming Support**: Enables fluent method chaining

### 2. Type Safety
The library leverages Python's type system extensively:
- **Runtime Validation**: Pydantic models validate data at runtime
- **Static Analysis**: Full type hints enable IDE support and mypy checking
- **Generic Types**: Type-safe containers and operations
- **Discriminated Unions**: Polymorphic type handling

### 3. Extensibility
The visitor pattern enables extending functionality without modifying core classes:
- **Open/Closed Principle**: Open for extension, closed for modification
- **Plugin Architecture**: New renderers can be added independently
- **Separation of Concerns**: Rendering logic separated from data models

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Claude Draw Library                      │
├─────────────────────────────────────────────────────────────────┤
│                    Application Layer                            │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │   Examples      │  │    Factories    │  │   User Code     │  │
│  │                 │  │                 │  │                 │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                     Service Layer                               │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │   Renderers     │  │  Serialization  │  │    Visitors     │  │
│  │  - SVGRenderer  │  │  - JSON Export  │  │ - BaseRenderer  │  │
│  │  - BoundsCalc   │  │  - Type Safety  │  │ - RenderContext │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                      Domain Layer                               │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │   Containers    │  │     Shapes      │  │   Protocols     │  │
│  │  - Drawing      │  │   - Circle      │  │  - Drawable     │  │
│  │  - Layer        │  │   - Rectangle   │  │  - Visitor      │  │
│  │  - Group        │  │   - Ellipse     │  │  - Renderer     │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                   Infrastructure Layer                          │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │     Models      │  │     Base        │  │      Core       │  │
│  │   - Point2D     │  │   - DrawModel   │  │   - Validation  │  │
│  │   - Color       │  │   - Drawable    │  │   - Exceptions  │  │
│  │   - Transform   │  │   - Container   │  │   - Utilities   │  │
│  │   - BoundBox    │  │   - Primitive   │  │                 │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Module Architecture

### Core Infrastructure (`models/`)

#### `base.py` - Foundation Classes
```python
class DrawModel(BaseModel):
    """Base for all data models with enhanced serialization"""

class Drawable(DrawModel, ABC):
    """Abstract base for all drawable objects"""
    - id: str
    - transform: Transform2D
    - get_bounds() -> BoundingBox
    - accept(visitor) -> Any
```

#### `point.py` - 2D Point Mathematics
```python
class Point2D(DrawModel):
    """Immutable 2D point with mathematical operations"""
    - x, y: float
    - distance_to(other) -> float
    - midpoint_to(other) -> Point2D
    - rotate_around(center, angle) -> Point2D
```

#### `color.py` - Color Management
```python
class Color(DrawModel):
    """RGB color with multiple creation methods"""
    - r, g, b, a: int/float
    - from_hex(hex_string) -> Color
    - from_hsl(h, s, l) -> Color
    - to_hex() -> str
```

#### `transform.py` - 2D Transformations
```python
class Transform2D(DrawModel):
    """Affine transformation matrix"""
    - a, b, c, d, tx, ty: float
    - translate(dx, dy) -> Transform2D
    - rotate(angle) -> Transform2D
    - scale(sx, sy) -> Transform2D
    - compose(other) -> Transform2D
```

### Domain Models (`base.py`)

#### Abstract Hierarchy
```
Drawable (ABC)
├── Primitive (ABC)
│   └── StyleMixin
└── Container (ABC)
    └── children: List[Drawable]
```

#### Inheritance Chain
- **DrawModel**: Pydantic base with enhanced serialization
- **Drawable**: Core interface with ID, transform, bounds, visitor acceptance
- **Primitive**: Base for atomic shapes with styling
- **Container**: Base for composite objects with children

### Shape Implementations (`shapes.py`)

Each shape implements the `Primitive` interface:

```python
class Circle(Primitive):
    center: Point2D
    radius: float
    
class Rectangle(Primitive):
    x, y: float
    width, height: float
    
class Ellipse(Primitive):
    center: Point2D
    rx, ry: float
    
class Line(Primitive):
    start, end: Point2D
```

All shapes provide:
- **Geometric Operations**: Move, resize, transform
- **Bounds Calculation**: Accurate bounding box computation
- **Visitor Acceptance**: Integration with rendering system
- **Immutable Updates**: Copy-on-write modifications

### Container System (`containers.py`)

#### Hierarchical Organization
```
Drawing (Root Canvas)
├── width, height: float
├── title: str
├── background_color: Optional[str]
└── children: List[Layer]

Layer (Rendering Layer)
├── name: Optional[str]
├── opacity: float
├── blend_mode: BlendMode
├── visible: bool
├── z_index: int
└── children: List[Union[Group, Primitive]]

Group (Organizational Container)
├── name: Optional[str]
├── z_index: int
└── children: List[Union[Group, Primitive]]
```

## Visitor Pattern Architecture

### Core Components

#### `protocols.py` - Visitor Interface
```python
class DrawableVisitor(Protocol):
    """Protocol defining visitor interface"""
    def visit_circle(self, circle: Circle) -> Any
    def visit_rectangle(self, rectangle: Rectangle) -> Any
    def visit_group(self, group: Group) -> Any
    def visit_layer(self, layer: Layer) -> Any
    def visit_drawing(self, drawing: Drawing) -> Any
```

#### `visitors.py` - Visitor Infrastructure
```python
class RenderContext:
    """Manages rendering state and transformations"""
    - _transform_stack: List[Transform2D]
    - _state_stack: List[RenderState]
    - push_transform(transform)
    - push_state(**updates)
    - get_effective_opacity() -> float

class BaseRenderer(ABC):
    """Abstract base for all renderers"""
    - context: RenderContext
    - render(drawable) -> str
    - pre_visit(drawable)
    - post_visit(drawable)
```

### Visitor Implementation Flow

```
1. Client calls renderer.render(drawing)
   ↓
2. BaseRenderer.render():
   - Calls begin_render()
   - Calls drawing.accept(self)
   - Calls end_render()
   ↓
3. drawing.accept(visitor):
   - Calls visitor.visit_drawing(self)
   ↓
4. Visitor processes object:
   - Calls pre_visit() for setup
   - Processes object-specific logic
   - Recursively visits children
   - Calls post_visit() for cleanup
   ↓
5. Result returned to client
```

### Concrete Renderers (`renderers.py`)

#### SVGRenderer
```python
class SVGRenderer(BaseRenderer):
    """Generates SVG markup from drawable objects"""
    - Handles coordinate transformations
    - Manages style attributes (fill, stroke, opacity)
    - Supports nested groups and layers
    - Outputs valid SVG XML
```

#### BoundingBoxCalculator
```python
class BoundingBoxCalculator(BaseRenderer):
    """Calculates overall bounding box of drawing"""
    - Tracks min/max coordinates
    - Handles transformations
    - Returns (x, y, width, height) bounds
```

## Serialization Architecture

### Type Discrimination System

#### Registry Pattern
```python
class SerializationRegistry:
    """Maps type names to classes for polymorphic deserialization"""
    - _class_registry: Dict[str, Type[DrawModel]]
    - _reverse_registry: Dict[Type[DrawModel], str]
    - register(type_name, cls)
    - get_class(type_name) -> Type
```

#### Enhanced JSON Encoder
```python
class EnhancedJSONEncoder(json.JSONEncoder):
    """Custom encoder with type discrimination"""
    - Adds __type__ discriminators
    - Handles circular references
    - Recursively processes nested objects
    - Maintains object reference IDs
```

### Serialization Flow

```
1. serialize_drawable(obj) called
   ↓
2. EnhancedJSONEncoder processes object:
   - Assigns reference ID
   - Gets all fields from actual class
   - Recursively processes nested DrawModel objects
   - Adds __type__ discriminator
   - Adds __version__ metadata
   ↓
3. JSON string returned with type information

4. deserialize_drawable(json_str) called
   ↓
5. Deserialization process:
   - Parses JSON to dictionary
   - Extracts __type__ discriminator
   - Looks up class in registry
   - Recursively deserializes nested objects
   - Handles enum conversion for strict validation
   - Creates typed object via model_validate()
   ↓
6. Fully typed object returned
```

### File Persistence
```python
def save_drawable(obj: Drawable, filename: str):
    """Save with proper formatting and encoding"""

def load_drawable(filename: str) -> Drawable:
    """Load with type safety and validation"""
```

## Factory Pattern (`factories.py`)

### Convenience Creation Functions
```python
# Direct constructors with validation
create_circle(x, y, radius, **kwargs) -> Circle
create_rectangle(x, y, width, height, **kwargs) -> Rectangle
create_square(x, y, size, **kwargs) -> Rectangle

# Specialized constructors
create_circle_from_diameter(x, y, diameter) -> Circle
create_rectangle_from_corners(x1, y1, x2, y2) -> Rectangle
create_rectangle_from_center(cx, cy, width, height) -> Rectangle

# Line utilities
create_horizontal_line(y, x_start, x_end) -> Line
create_vertical_line(x, y_start, y_end) -> Line
```

## Key Design Patterns

### 1. Visitor Pattern
- **Purpose**: Separate algorithms from data structures
- **Benefits**: Extensible rendering without modifying core classes
- **Implementation**: Protocol-based with context management

### 2. Immutable Objects
- **Purpose**: Thread safety and predictable behavior
- **Benefits**: Copy-on-write semantics, functional programming support
- **Implementation**: Pydantic frozen models

### 3. Factory Pattern
- **Purpose**: Convenient object creation with validation
- **Benefits**: Simplified API, consistent validation
- **Implementation**: Module-level functions with kwargs

### 4. Strategy Pattern (via Visitors)
- **Purpose**: Interchangeable rendering algorithms
- **Benefits**: Runtime renderer selection
- **Implementation**: Common visitor interface

### 5. Composite Pattern
- **Purpose**: Hierarchical object structures
- **Benefits**: Uniform treatment of individual and composite objects
- **Implementation**: Container/Primitive hierarchy

### 6. Registry Pattern
- **Purpose**: Type resolution for polymorphic deserialization
- **Benefits**: Extensible type system
- **Implementation**: Centralized type mapping

## Performance Considerations

### Memory Efficiency
- **Immutable Objects**: Potential memory overhead offset by safety benefits
- **Copy-on-Write**: Only modified fields create new instances
- **Transform Composition**: Matrix operations avoid deep copying

### Rendering Performance
- **Visitor Pattern**: Single traversal for complex operations
- **Context Stack**: Efficient state management
- **Lazy Evaluation**: Bounds calculated only when needed

### Serialization Performance
- **Type Discrimination**: O(1) type lookup via registry
- **Circular Reference Detection**: Prevents infinite loops
- **Streaming**: Large objects can be streamed to files

## Testing Architecture

### Test Organization
```
tests/
├── models/           # Unit tests for data models
├── test_shapes.py    # Shape-specific functionality
├── test_containers.py # Container hierarchy tests
├── test_visitors.py  # Visitor pattern tests
├── test_serialization.py # Serialization round-trip tests
└── test_factories.py # Factory function tests
```

### Coverage Goals
- **Unit Tests**: Individual component functionality
- **Integration Tests**: Cross-component interactions
- **Round-trip Tests**: Serialization fidelity
- **Performance Tests**: Benchmarking critical paths

### Test Strategy
- **Property-based Testing**: Validation of mathematical operations
- **Mock Objects**: Isolated visitor testing
- **Fixture Sharing**: Consistent test data
- **Parameterized Tests**: Comprehensive edge case coverage

## Extension Points

### Adding New Shapes
1. Inherit from `Primitive`
2. Implement required abstract methods
3. Add visitor method to `DrawableVisitor`
4. Register type for serialization
5. Add factory functions

### Adding New Renderers
1. Inherit from `BaseRenderer`
2. Implement visitor methods
3. Handle pre/post visit logic
4. Manage renderer-specific state

### Adding New Export Formats
1. Create format-specific renderer
2. Implement visitor pattern
3. Handle format-specific features
4. Add convenience functions

## Future Architectural Considerations

### Planned Enhancements
- **Plugin System**: Dynamic renderer loading
- **Streaming API**: Large document handling
- **Async Support**: Non-blocking operations
- **GPU Acceleration**: Hardware-accelerated rendering

### Scalability Considerations
- **Memory Pooling**: Object reuse for performance
- **Spatial Indexing**: Efficient spatial queries
- **Level-of-Detail**: Adaptive rendering complexity
- **Caching**: Computed property memoization

### C++ Optimization Layer
For handling millions of objects with extreme performance requirements, a C++ optimization layer is being developed. See [C++ Architecture](cpp_architecture.md) for detailed design information including:
- 50-100x performance improvements
- SIMD optimizations for batch operations
- Custom memory management with arena allocators
- Zero-copy Python integration via pybind11
- Binary serialization format
- Spatial indexing structures

This architecture provides a solid foundation for a modern, extensible 2D graphics library while maintaining type safety, performance, and maintainability.