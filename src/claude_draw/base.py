"""Abstract base classes for Claude Draw."""

from abc import ABC, abstractmethod
from typing import Any, Optional, List
from uuid import uuid4
from pydantic import Field, ConfigDict

from claude_draw.models.base import DrawModel
from claude_draw.models.transform import Transform2D
from claude_draw.models.bounding_box import BoundingBox
from claude_draw.models.color import Color


class Drawable(DrawModel, ABC):
    """Abstract base class for all drawable objects.
    
    This class provides the foundation for all objects that can be rendered
    in the graphics system. It includes support for transformations, unique
    identification, and the visitor pattern.
    """
    
    model_config = ConfigDict(
        frozen=True,  # Make instances immutable
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )
    
    # Unique identifier for the drawable object
    id: str = Field(default_factory=lambda: str(uuid4()), description="Unique identifier")
    
    # Transform applied to this object
    transform: Transform2D = Field(default_factory=Transform2D, description="Object transformation")
    
    @abstractmethod
    def get_bounds(self) -> BoundingBox:
        """Calculate and return the bounding box of this drawable object.
        
        Returns:
            BoundingBox: The bounding box containing this object
        """
        ...
    
    @abstractmethod
    def accept(self, visitor: "DrawableVisitor") -> Any:
        """Accept a visitor for processing this drawable object.
        
        This method implements the visitor pattern, allowing different operations
        to be performed on drawable objects without modifying their classes.
        
        Args:
            visitor: The visitor object that will process this drawable
            
        Returns:
            Any: The result of the visitor's processing (depends on visitor type)
        """
        ...
    
    def with_transform(self, transform: Transform2D) -> "Drawable":
        """Create a new instance with the specified transform.
        
        Args:
            transform: The new transform to apply
            
        Returns:
            Drawable: A new instance with the updated transform
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
        
        Args:
            dx: Translation distance in x direction
            dy: Translation distance in y direction
            
        Returns:
            Drawable: A new instance with the updated transform
        """
        new_transform = self.transform.translate(dx, dy)
        return self.with_transform(new_transform)
    
    def rotate(self, angle: float) -> "Drawable":
        """Create a new instance rotated by the specified angle.
        
        Args:
            angle: Rotation angle in radians
            
        Returns:
            Drawable: A new instance with the updated transform
        """
        new_transform = self.transform.rotate(angle)
        return self.with_transform(new_transform)
    
    def scale(self, sx: float, sy: Optional[float] = None) -> "Drawable":
        """Create a new instance scaled by the specified factors.
        
        Args:
            sx: Scale factor in x direction
            sy: Scale factor in y direction (defaults to sx for uniform scaling)
            
        Returns:
            Drawable: A new instance with the updated transform
        """
        if sy is None:
            sy = sx
        new_transform = self.transform.scale(sx, sy)
        return self.with_transform(new_transform)


class StyleMixin(DrawModel, ABC):
    """Mixin class providing style-related functionality.
    
    This mixin can be combined with drawable classes to provide styling
    capabilities such as fill color, stroke properties, and opacity.
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )
    
    # Fill properties
    fill: Optional[Color] = Field(None, description="Fill color")
    
    # Stroke properties
    stroke: Optional[Color] = Field(None, description="Stroke color")
    stroke_width: float = Field(1.0, ge=0.0, description="Stroke width")
    
    # Opacity
    opacity: float = Field(1.0, ge=0.0, le=1.0, description="Opacity (0.0 to 1.0)")
    
    # Visibility
    visible: bool = Field(True, description="Whether the object is visible")
    
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
    to create the foundation for basic shapes like circles, rectangles, and lines.
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )


class Container(Drawable, ABC):
    """Abstract base class for objects that can contain other drawable objects.
    
    This class provides the foundation for grouping and organizing drawable
    objects in hierarchical structures such as groups, layers, and canvases.
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )
    
    # Child objects
    children: List[Drawable] = Field(default_factory=list, description="Child drawable objects")
    
    @abstractmethod
    def add_child(self, child: Drawable) -> "Container":
        """Add a child drawable object.
        
        Args:
            child: The drawable object to add as a child
            
        Returns:
            Container: A new instance with the child added
        """
        ...
    
    @abstractmethod
    def remove_child(self, child_id: str) -> "Container":
        """Remove a child drawable object by ID.
        
        Args:
            child_id: The ID of the child to remove
            
        Returns:
            Container: A new instance with the child removed
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
        
        Returns:
            BoundingBox: The bounding box containing all child objects
        """
        if not self.children:
            return BoundingBox(x=0, y=0, width=0, height=0)
        
        # Get bounds of all children
        child_bounds = [child.get_bounds() for child in self.children]
        
        # Calculate union of all bounds
        min_x = min(bounds.x for bounds in child_bounds)
        min_y = min(bounds.y for bounds in child_bounds)
        max_x = max(bounds.x + bounds.width for bounds in child_bounds)
        max_y = max(bounds.y + bounds.height for bounds in child_bounds)
        
        return BoundingBox(
            x=min_x,
            y=min_y,
            width=max_x - min_x,
            height=max_y - min_y
        )