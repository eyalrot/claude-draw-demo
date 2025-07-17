# Claude Draw Architecture Documentation

## Overview

Claude Draw is a Python-based 2D vector graphics library built on Pydantic v2 for strong typing and data validation. This document outlines the abstract base classes, protocols, and architectural patterns used throughout the library.

## Design Principles

### 1. Immutability
- All graphic objects are immutable using Pydantic's `frozen=True` configuration
- Operations return new instances rather than modifying existing ones
- Ensures thread safety and predictable behavior

### 2. Type Safety
- Comprehensive type hints throughout the codebase
- Pydantic validation for all data models
- Protocol-based interfaces for extensibility

### 3. Visitor Pattern
- Clean separation between data structures and operations
- Extensible rendering system through visitor protocols
- Easy to add new output formats without modifying existing classes

## Core Architecture

### Base Classes Hierarchy

```
DrawModel (Pydantic BaseModel)
├── Point2D
├── Color
├── Transform2D
├── BoundingBox
└── Drawable (Abstract Base Class)
    ├── Primitive (Abstract, extends StyleMixin)
    │   ├── Circle
    │   ├── Rectangle
    │   ├── Line
    │   └── Path
    └── Container (Abstract)
        ├── Group
        ├── Layer
        └── Canvas
```

### Abstract Base Classes

#### 1. Drawable
**Purpose**: Base class for all renderable objects in the graphics hierarchy.

**Key Responsibilities**:
- Unique identification (id field)
- Transform management
- Bounding box calculations
- Visitor pattern support

**Abstract Methods**:
- `get_bounds() -> BoundingBox`: Calculate object's bounding box
- `accept(visitor: DrawableVisitor) -> Any`: Accept visitor for operations

**Concrete Methods**:
- Transform application and manipulation
- Common property access

#### 2. StyleMixin
**Purpose**: Mixin class providing style-related functionality for objects that can be styled.

**Key Responsibilities**:
- Fill and stroke properties
- Opacity and visibility
- Style validation and defaults

**Properties**:
- `fill: Optional[Color]`: Fill color
- `stroke: Optional[Color]`: Stroke color
- `stroke_width: float`: Stroke width
- `opacity: float`: Opacity (0.0 to 1.0)

#### 3. Primitive
**Purpose**: Base class for basic geometric shapes that can be styled.

**Inheritance**: `Drawable + StyleMixin`

**Key Responsibilities**:
- Basic shape rendering
- Style application
- Geometric calculations

**Abstract Methods**:
- Implementation depends on specific shape

#### 4. Container
**Purpose**: Base class for objects that can contain other drawable objects.

**Key Responsibilities**:
- Child object management
- Hierarchical bounds calculation
- Visitor pattern propagation

**Abstract Methods**:
- `add_child(child: Drawable) -> Container`: Add child (returns new instance)
- `remove_child(child_id: str) -> Container`: Remove child (returns new instance)
- `get_children() -> List[Drawable]`: Get all children

### Protocol Definitions

#### 1. DrawableVisitor Protocol
**Purpose**: Define the interface for visiting drawable objects.

```python
from typing import Protocol, Any

class DrawableVisitor(Protocol):
    """Protocol for visiting drawable objects."""
    
    def visit_circle(self, circle: Circle) -> Any:
        """Visit a circle object."""
        ...
    
    def visit_rectangle(self, rectangle: Rectangle) -> Any:
        """Visit a rectangle object."""
        ...
    
    def visit_line(self, line: Line) -> Any:
        """Visit a line object."""
        ...
    
    def visit_path(self, path: Path) -> Any:
        """Visit a path object."""
        ...
    
    def visit_group(self, group: Group) -> Any:
        """Visit a group object."""
        ...
    
    def visit_layer(self, layer: Layer) -> Any:
        """Visit a layer object."""
        ...
    
    def visit_canvas(self, canvas: Canvas) -> Any:
        """Visit a canvas object."""
        ...
```

#### 2. Renderer Protocol
**Purpose**: Define the interface for rendering drawable objects to different output formats.

```python
from typing import Protocol, Any

class Renderer(Protocol):
    """Protocol for rendering drawable objects."""
    
    def render(self, drawable: Drawable) -> str:
        """Render a drawable object to output format."""
        ...
    
    def begin_render(self) -> None:
        """Initialize rendering context."""
        ...
    
    def end_render(self) -> str:
        """Finalize and return rendered output."""
        ...
```

## Design Patterns

### 1. Visitor Pattern
**Implementation**: DrawableVisitor protocol with accept() method on Drawable

**Benefits**:
- Separation of concerns between data and operations
- Easy to add new operations without modifying existing classes
- Type-safe visitor dispatch

**Usage**:
```python
svg_renderer = SVGRenderer()
result = drawable.accept(svg_renderer)
```

### 2. Immutable Builder Pattern
**Implementation**: All modification methods return new instances

**Benefits**:
- Thread safety
- Predictable state management
- Easier debugging and testing

**Usage**:
```python
# Returns new instance, original unchanged
new_circle = circle.with_fill(Color.red()).with_radius(50)
```

### 3. Strategy Pattern
**Implementation**: Renderer protocol for different output formats

**Benefits**:
- Pluggable rendering backends
- Easy to add new output formats
- Clean separation of rendering logic

### 4. Composite Pattern
**Implementation**: Container classes that can hold other drawables

**Benefits**:
- Hierarchical object composition
- Uniform treatment of individual objects and compositions
- Recursive operations on object trees

## Pydantic Configuration

### Immutability Configuration
```python
model_config = ConfigDict(
    frozen=True,  # Make instances immutable
    validate_assignment=True,  # Validate on field assignment
    extra="forbid",  # Reject extra fields
    strict=True,  # Strict validation
)
```

### Custom Validators
- Numeric range validation for coordinates and dimensions
- Color format validation for hex, RGB, and HSL
- Transform matrix validation for numerical stability

## Type System Integration

### Generic Types
- `Container[T]` for type-safe child management
- `Visitor[T]` for type-safe visitor results

### Protocol Usage
- Runtime type checking for visitor implementations
- Structural typing for renderer compatibility

### Type Annotations
- All methods fully annotated with type hints
- Generic protocols for extensibility
- Union types for optional parameters

## Error Handling

### Custom Exceptions
- `DrawingError`: Base exception for all drawing operations
- `ValidationError`: Data validation failures
- `TransformError`: Transform matrix operations
- `RenderingError`: Rendering operation failures

### Validation Strategy
- Pydantic validators for data integrity
- Custom validators for domain-specific rules
- Comprehensive error messages for debugging

## Extension Points

### Adding New Shapes
1. Inherit from `Primitive`
2. Implement required abstract methods
3. Add visitor method to `DrawableVisitor`
4. Update renderer implementations

### Adding New Renderers
1. Implement `Renderer` protocol
2. Handle all drawable types
3. Follow visitor pattern for traversal

### Adding New Containers
1. Inherit from `Container`
2. Implement child management methods
3. Handle bounds calculation for children

## Performance Considerations

### Immutability Performance
- Uses Pydantic's efficient copying mechanisms
- Structural sharing where possible
- Lazy evaluation for expensive operations

### Visitor Pattern Performance
- Single dispatch for O(1) method resolution
- Minimal overhead for visitor calls
- Efficient traversal of object hierarchies

### Memory Management
- Immutable objects enable aggressive caching
- Garbage collection friendly with clear ownership
- Minimal memory footprint for simple objects

## Testing Strategy

### Unit Tests
- Test abstract method implementations
- Verify immutability enforcement
- Validate type annotations with mypy
- Test visitor pattern dispatch

### Integration Tests
- Test inheritance hierarchies
- Verify protocol implementations
- Test complex object compositions
- Validate rendering outputs

### Property-Based Tests
- Generate random valid inputs
- Test invariants across operations
- Verify immutability properties
- Test visitor pattern correctness

## Migration Path

### From Current State
1. Implement abstract base classes
2. Add protocol definitions
3. Update existing models to inherit from new hierarchy
4. Add visitor pattern support
5. Implement renderer protocols

### Backward Compatibility
- Existing models will continue to work
- New features will be additive
- Deprecation warnings for old patterns
- Gradual migration path provided

## Future Considerations

### Performance Optimizations
- Compiled visitors for hot paths
- Batch operations for multiple objects
- Caching for expensive calculations

### Additional Features
- Animation support through time-based visitors
- Serialization/deserialization for persistence
- Plugin system for custom renderers
- Interactive editing capabilities

This architecture provides a solid foundation for the Claude Draw library while maintaining flexibility for future enhancements and extensions.