"""Abstract base classes for Claude Draw.

This module defines the core abstract base classes that form the foundation
of the Claude Draw graphics library. These classes establish the fundamental
interfaces and behaviors for all drawable objects, containers, and styled
elements in the system.

Key classes:
- Drawable: Base interface for all objects that can be rendered
- StyleMixin: Provides styling capabilities (colors, strokes, opacity)
- Primitive: Base for atomic shapes (circles, rectangles, etc.)
- Container: Base for objects that can contain other drawables

The module implements several important design patterns:
- Visitor Pattern: Through the accept() method for extensible rendering
- Immutability: All objects are frozen for thread safety
- Fluent Interface: Methods return new instances for chaining
- Composition: Complex graphics built from simple primitives
"""

from abc import ABC, abstractmethod
from typing import Any, Optional, List
from uuid import uuid4
from pydantic import Field, ConfigDict

from claude_draw.models.base import DrawModel
from claude_draw.models.transform import Transform2D
from claude_draw.models.bounding_box import BoundingBox
from claude_draw.models.color import Color


class Drawable(DrawModel, ABC):
    """Abstract base class for all drawable objects in Claude Draw.
    
    This class provides the foundation for all objects that can be rendered
    in the graphics system. It establishes the core interface that all
    drawable objects must implement, including support for transformations,
    unique identification, bounds calculation, and the visitor pattern.
    
    The Drawable class embodies several key architectural principles:
    
    1. **Immutability**: Instances are frozen, ensuring thread safety and
       predictable behavior. All modifications create new instances.
       
    2. **Unique Identity**: Each drawable has a UUID for tracking and
       manipulation in complex scenes.
       
    3. **Transformability**: Built-in support for 2D affine transformations
       (translation, rotation, scaling) via the Transform2D system.
       
    4. **Visitor Pattern**: The accept() method enables extensible rendering
       and processing without modifying the class hierarchy.
       
    5. **Bounds Awareness**: Every drawable can calculate its bounding box
       for layout, collision detection, and rendering optimization.
    
    Attributes:
        id (str): Unique identifier (UUID) for this drawable instance
        transform (Transform2D): Affine transformation matrix applied to this object
    
    Example:
        >>> # Concrete implementations like Circle, Rectangle inherit from this
        >>> circle = Circle(center=Point2D(x=0, y=0), radius=10)
        >>> transformed = circle.rotate(math.pi / 4).translate(5, 5)
        >>> bounds = transformed.get_bounds()
    """
    
    model_config = ConfigDict(
        frozen=True,  # Make instances immutable for thread safety
        validate_assignment=True,  # Validate on field updates
        extra="forbid",  # Reject undefined fields
        strict=True,  # Enable strict validation
    )
    
    # Unique identifier for the drawable object, automatically generated
    id: str = Field(
        default_factory=lambda: str(uuid4()), 
        description="Unique identifier for tracking and referencing this drawable"
    )
    
    # Transform applied to this object, defaults to identity transform
    transform: Transform2D = Field(
        default_factory=Transform2D, 
        description="2D affine transformation matrix (translation, rotation, scale)"
    )
    
    @abstractmethod
    def get_bounds(self) -> BoundingBox:
        """Calculate and return the bounding box of this drawable object.
        
        This method must be implemented by all concrete drawable classes.
        The bounding box represents the minimal axis-aligned rectangle that
        completely contains the drawable object, taking into account any
        transformations applied to it.
        
        The bounding box is essential for:
        - Determining drawing order and overlaps
        - Optimizing rendering (culling off-screen objects)  
        - Hit testing and collision detection
        - Automatic layout and sizing
        
        Returns:
            BoundingBox: The minimal rectangle containing this entire object,
                with x, y representing the top-left corner and width, height
                representing the dimensions
                
        Note:
            Implementations should account for stroke width, transformations,
            and any other visual properties that affect the object's extent.
        """
        ...
    
    @abstractmethod
    def accept(self, visitor: "DrawableVisitor") -> Any:
        """Accept a visitor for processing this drawable object.
        
        This method implements the visitor pattern, which is the cornerstone
        of Claude Draw's extensible architecture. It allows different operations
        (rendering, analysis, transformation) to be performed on drawable objects
        without modifying their classes.
        
        The visitor pattern enables:
        - Multiple rendering backends (SVG, Canvas, PDF, etc.)
        - Analysis operations (bounds calculation, statistics)
        - Transformations and filters
        - Serialization strategies
        - Any future operations without changing existing code
        
        Args:
            visitor: The visitor object that will process this drawable.
                Must implement the DrawableVisitor protocol with appropriate
                visit methods for each drawable type.
            
        Returns:
            Any: The result of the visitor's processing. The return type
                depends on the visitor implementation (e.g., string for
                SVGRenderer, BoundingBox for BoundsCalculator).
                
        Example:
            >>> svg_renderer = SVGRenderer()
            >>> svg_output = drawable.accept(svg_renderer)
            >>> 
            >>> bounds_calc = BoundingBoxCalculator()
            >>> bounds = drawable.accept(bounds_calc)
        """
        ...
    
    def with_transform(self, transform: Transform2D) -> "Drawable":
        """Create a new instance with the specified transform.
        
        This method supports the immutability pattern by creating a new
        instance rather than modifying the existing one. It replaces the
        current transformation matrix entirely.
        
        Args:
            transform: The new transform to apply. This completely replaces
                the existing transform rather than composing with it.
            
        Returns:
            Drawable: A new instance of the same type with the updated
                transform. All other properties remain unchanged.
                
        Example:
            >>> identity = Transform2D()  # Identity transform
            >>> rotated = Transform2D().rotate(math.pi / 2)
            >>> shape_with_new_transform = shape.with_transform(rotated)
        """
        return self.model_copy(update={"transform": transform})
    
    def with_id(self, new_id: str) -> "Drawable":
        """Create a new instance with the specified ID.
        
        Args:
            new_id: The new ID to assign
            
        Returns:
            Drawable: A new instance with the updated ID
        """
        return self.model_copy(update={"id": new_id})
    
    def translate(self, dx: float, dy: float) -> "Drawable":
        """Create a new instance translated by the specified amount.
        
        This method composes a translation with the existing transform,
        preserving any previous transformations (rotations, scales, etc.).
        The translation is applied in the current coordinate system.
        
        Args:
            dx: Translation distance along the x-axis (positive = right)
            dy: Translation distance along the y-axis (positive = down)
            
        Returns:
            Drawable: A new instance with the translation applied to
                the existing transform
                
        Example:
            >>> # Move a shape 10 units right and 20 units down
            >>> moved_shape = shape.translate(10, 20)
            >>> 
            >>> # Chain translations
            >>> final_shape = shape.translate(5, 0).translate(0, 10)
        """
        new_transform = self.transform.translate(dx, dy)
        return self.with_transform(new_transform)
    
    def rotate(self, angle: float) -> "Drawable":
        """Create a new instance rotated by the specified angle.
        
        This method composes a rotation with the existing transform.
        The rotation is applied around the origin (0, 0) of the current
        coordinate system. To rotate around a different point, combine
        with translations.
        
        Args:
            angle: Rotation angle in radians. Positive values rotate
                counter-clockwise, negative values rotate clockwise.
            
        Returns:
            Drawable: A new instance with the rotation applied to
                the existing transform
                
        Example:
            >>> import math
            >>> # Rotate 45 degrees counter-clockwise
            >>> rotated = shape.rotate(math.pi / 4)
            >>> 
            >>> # Rotate around a specific point (cx, cy)
            >>> rotated_around_point = (
            ...     shape.translate(-cx, -cy)
            ...          .rotate(angle)
            ...          .translate(cx, cy)
            ... )
        """
        new_transform = self.transform.rotate(angle)
        return self.with_transform(new_transform)
    
    def scale(self, sx: float, sy: Optional[float] = None) -> "Drawable":
        """Create a new instance scaled by the specified factors.
        
        This method composes a scaling transformation with the existing
        transform. Scaling is applied from the origin (0, 0). To scale
        from a different point, combine with translations.
        
        Args:
            sx: Scale factor along the x-axis. Values > 1 enlarge,
                values < 1 shrink, negative values flip.
            sy: Scale factor along the y-axis. If None, uses sx for
                uniform scaling (maintains aspect ratio).
            
        Returns:
            Drawable: A new instance with the scaling applied to
                the existing transform
                
        Example:
            >>> # Double the size uniformly
            >>> larger = shape.scale(2.0)
            >>> 
            >>> # Stretch horizontally, shrink vertically  
            >>> stretched = shape.scale(2.0, 0.5)
            >>> 
            >>> # Flip horizontally
            >>> flipped = shape.scale(-1.0, 1.0)
            >>> 
            >>> # Scale from center point (cx, cy)
            >>> scaled_from_center = (
            ...     shape.translate(-cx, -cy)
            ...          .scale(sx, sy)
            ...          .translate(cx, cy)
            ... )
        """
        if sy is None:
            sy = sx
        new_transform = self.transform.scale(sx, sy)
        return self.with_transform(new_transform)


class StyleMixin(DrawModel, ABC):
    """Mixin class providing comprehensive style-related functionality.
    
    This mixin can be combined with drawable classes to provide rich styling
    capabilities for visual appearance. It encapsulates all the common visual
    properties that affect how a shape is rendered.
    
    The StyleMixin follows these design principles:
    
    1. **Separation of Concerns**: Style properties are separated from
       geometric properties, allowing independent evolution.
       
    2. **Optional Styling**: All style properties are optional with sensible
       defaults, allowing minimal specifications.
       
    3. **Immutable Updates**: All style modifications return new instances,
       maintaining the immutability guarantee.
       
    4. **Validation**: Built-in validation ensures valid ranges for properties
       like opacity and stroke width.
    
    Style Properties:
    - **Fill**: Interior color of shapes (None = no fill/transparent)
    - **Stroke**: Outline color and width (None = no stroke)
    - **Opacity**: Overall transparency (0.0 = invisible, 1.0 = opaque)
    - **Visibility**: Boolean flag to hide/show objects
    
    Example:
        >>> # Create a red circle with blue outline
        >>> circle = Circle(
        ...     center=Point2D(x=50, y=50),
        ...     radius=30,
        ...     fill=Color(r=255, g=0, b=0),
        ...     stroke=Color(r=0, g=0, b=255),
        ...     stroke_width=2.0
        ... )
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )
    
    # Fill properties
    fill: Optional[Color] = Field(
        None, 
        description="Interior fill color. None means no fill (transparent interior)"
    )
    
    # Stroke properties  
    stroke: Optional[Color] = Field(
        None, 
        description="Outline stroke color. None means no stroke (no outline)"
    )
    stroke_width: float = Field(
        1.0, 
        ge=0.0, 
        description="Width of the stroke outline in pixels. 0 means no stroke"
    )
    
    # Opacity
    opacity: float = Field(
        1.0, 
        ge=0.0, 
        le=1.0, 
        description="Overall opacity/transparency. 0.0 is fully transparent, 1.0 is fully opaque"
    )
    
    # Visibility
    visible: bool = Field(
        True, 
        description="Whether the object is rendered. False objects are completely hidden"
    )
    
    def with_fill(self, fill: Optional[Color]) -> "StyleMixin":
        """Create a new instance with the specified fill color.
        
        Args:
            fill: The new fill color (None for no fill)
            
        Returns:
            StyleMixin: A new instance with the updated fill
        """
        return self.model_copy(update={"fill": fill})
    
    def with_stroke(self, stroke: Optional[Color], width: Optional[float] = None) -> "StyleMixin":
        """Create a new instance with the specified stroke properties.
        
        Args:
            stroke: The new stroke color (None for no stroke)
            width: The new stroke width (if not specified, keeps current)
            
        Returns:
            StyleMixin: A new instance with the updated stroke
        """
        updates = {"stroke": stroke}
        if width is not None:
            updates["stroke_width"] = width
        return self.model_copy(update=updates)
    
    def with_opacity(self, opacity: float) -> "StyleMixin":
        """Create a new instance with the specified opacity.
        
        Args:
            opacity: The new opacity value (0.0 to 1.0)
            
        Returns:
            StyleMixin: A new instance with the updated opacity
        """
        return self.model_copy(update={"opacity": opacity})
    
    def with_visibility(self, visible: bool) -> "StyleMixin":
        """Create a new instance with the specified visibility.
        
        Args:
            visible: Whether the object should be visible
            
        Returns:
            StyleMixin: A new instance with the updated visibility
        """
        return self.model_copy(update={"visible": visible})
    
    def is_filled(self) -> bool:
        """Check if the object has a fill color.
        
        Returns:
            bool: True if the object has a fill color
        """
        return self.fill is not None
    
    def is_stroked(self) -> bool:
        """Check if the object has a stroke.
        
        Returns:
            bool: True if the object has a stroke color
        """
        return self.stroke is not None and self.stroke_width > 0


class Primitive(Drawable, StyleMixin, ABC):
    """Abstract base class for primitive geometric shapes.
    
    This class combines the drawable interface with styling capabilities
    to create the foundation for basic atomic shapes that form the building
    blocks of more complex graphics.
    
    Primitives are the fundamental visual elements in Claude Draw:
    - They represent single, indivisible geometric shapes
    - They can be styled with colors, strokes, and opacity
    - They can be transformed (translated, rotated, scaled)
    - They can be composed into more complex structures via containers
    
    The Primitive class serves as the base for concrete shapes like:
    - Circle: Defined by center and radius
    - Rectangle: Defined by position and dimensions
    - Ellipse: Defined by center and two radii
    - Line: Defined by start and end points
    - Path: Defined by a series of commands (future)
    - Text: Defined by position and content (future)
    
    By inheriting from both Drawable and StyleMixin, primitives get:
    - Unique identification and transformation support (from Drawable)
    - Fill, stroke, opacity, and visibility control (from StyleMixin)
    - Visitor pattern support for extensible rendering
    - Immutability guarantees for thread safety
    
    Example:
        >>> # Concrete primitive with full styling
        >>> rect = Rectangle(
        ...     x=10, y=20, width=100, height=50,
        ...     fill=Color.from_hex("#FF0000"),
        ...     stroke=Color.from_hex("#000000"),
        ...     stroke_width=2.0,
        ...     opacity=0.8
        ... )
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )


class Container(Drawable, ABC):
    """Abstract base class for objects that can contain other drawable objects.
    
    This class provides the foundation for creating hierarchical structures
    of drawable objects, enabling complex compositions and scene organization.
    Containers are essential for building layered graphics, managing groups
    of related objects, and creating reusable components.
    
    Key characteristics of containers:
    
    1. **Hierarchical Structure**: Containers can hold any drawable objects,
       including other containers, enabling deep nesting.
       
    2. **Composite Pattern**: Containers are treated uniformly as drawables,
       allowing recursive operations on entire subtrees.
       
    3. **Bounds Aggregation**: A container's bounds encompass all its children,
       automatically adjusting as children are added or removed.
       
    4. **Transform Propagation**: Transformations applied to containers affect
       all children, enabling group transformations.
       
    5. **Visitor Traversal**: The visitor pattern recursively processes the
       container and all its children in the correct order.
    
    Container types in Claude Draw:
    - **Group**: Basic container for organizing related shapes
    - **Layer**: Container with opacity and blend mode support
    - **Drawing**: Root container representing the entire canvas
    
    Operations:
    - Add/remove children while maintaining immutability
    - Find children by ID for targeted updates
    - Calculate aggregate bounds for layout
    - Apply transformations to entire groups
    
    Example:
        >>> # Create a group with multiple shapes
        >>> group = Group()
        >>> group = group.add_child(circle).add_child(rectangle)
        >>> # Transform entire group
        >>> rotated_group = group.rotate(math.pi / 4)
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )
    
    # Child objects
    children: List[Drawable] = Field(
        default_factory=list, 
        description="Ordered list of child drawable objects. Rendering order is typically back-to-front (first child is drawn first)"
    )
    
    @abstractmethod
    def add_child(self, child: Drawable) -> "Container":
        """Add a child drawable object to this container.
        
        This method must be implemented by concrete container classes.
        It should create a new container instance with the child appended
        to the children list, maintaining immutability.
        
        The child will be added to the end of the children list, making it
        the topmost object in the rendering order (drawn last, appears on top).
        
        Args:
            child: The drawable object to add as a child. Can be any object
                implementing the Drawable interface (primitives, other containers).
            
        Returns:
            Container: A new container instance with the child added. The original
                container remains unchanged (immutability principle).
                
        Example:
            >>> container = Group()
            >>> circle = Circle(center=Point2D(x=0, y=0), radius=10)
            >>> new_container = container.add_child(circle)
            >>> assert len(new_container.children) == 1
            >>> assert len(container.children) == 0  # Original unchanged
        """
        ...
    
    @abstractmethod
    def remove_child(self, child_id: str) -> "Container":
        """Remove a child drawable object by its unique ID.
        
        This method must be implemented by concrete container classes.
        It should create a new container instance with the specified child
        removed from the children list.
        
        Args:
            child_id: The unique ID of the child to remove. If no child
                with this ID exists, the container should typically be
                returned unchanged (no error).
            
        Returns:
            Container: A new container instance with the specified child
                removed. If the child was not found, returns an identical
                copy of the container.
                
        Note:
            This method only removes direct children. It does not recursively
            search nested containers. For deep removal, additional logic would
            be needed.
            
        Example:
            >>> container = Group(children=[circle1, circle2])
            >>> smaller_container = container.remove_child(circle1.id)
            >>> assert len(smaller_container.children) == 1
            >>> assert smaller_container.children[0].id == circle2.id
        """
        ...
    
    def get_children(self) -> List[Drawable]:
        """Get all child drawable objects.
        
        Returns:
            List[Drawable]: List of child objects
        """
        return self.children.copy()
    
    def get_child_by_id(self, child_id: str) -> Optional[Drawable]:
        """Get a child drawable object by ID.
        
        Args:
            child_id: The ID of the child to find
            
        Returns:
            Optional[Drawable]: The child object if found, None otherwise
        """
        for child in self.children:
            if child.id == child_id:
                return child
        return None
    
    def has_children(self) -> bool:
        """Check if this container has any child objects.
        
        Returns:
            bool: True if the container has children
        """
        return len(self.children) > 0
    
    def get_bounds(self) -> BoundingBox:
        """Calculate the bounding box that encompasses all children.
        
        This method computes the minimal axis-aligned bounding box that
        contains all child objects. It's essential for:
        - Determining the container's size for layout purposes
        - Culling optimization in rendering pipelines
        - Hit testing and collision detection
        - Automatic canvas sizing
        
        The algorithm:
        1. Gets the bounds of each child (which may be recursive for nested containers)
        2. Finds the minimum x and y coordinates across all bounds
        3. Finds the maximum x and y coordinates across all bounds
        4. Creates a bounding box from these extremes
        
        Special cases:
        - Empty container returns a zero-sized box at origin (0, 0)
        - Transformations on children are accounted for in their bounds
        - Invisible children are still included in bounds calculation
        
        Returns:
            BoundingBox: The minimal rectangle containing all child objects.
                For empty containers, returns a zero-sized box at (0, 0).
                
        Performance Note:
            This operation is O(n) where n is the number of direct children.
            For deeply nested structures, consider caching bounds calculations.
        """
        if not self.children:
            # Empty container has zero bounds at origin
            return BoundingBox(x=0, y=0, width=0, height=0)
        
        # Get bounds of all children (may trigger recursive calculations)
        child_bounds = [child.get_bounds() for child in self.children]
        
        # Calculate union of all bounds to find extremes
        min_x = min(bounds.x for bounds in child_bounds)
        min_y = min(bounds.y for bounds in child_bounds)
        max_x = max(bounds.x + bounds.width for bounds in child_bounds)
        max_y = max(bounds.y + bounds.height for bounds in child_bounds)
        
        # Return the encompassing bounding box
        return BoundingBox(
            x=min_x,
            y=min_y,
            width=max_x - min_x,
            height=max_y - min_y
        )