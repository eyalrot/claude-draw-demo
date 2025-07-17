"""BoundingBox model for representing rectangular bounds in 2D space."""

from typing import List, Optional, Union
from pydantic import field_validator, model_validator

from claude_draw.models.base import DrawModel
from claude_draw.models.point import Point2D
from claude_draw.models.validators import validate_finite_number


class BoundingBox(DrawModel):
    """A 2D bounding box represented by position and dimensions.
    
    The bounding box is defined by its top-left corner (x, y) and
    its width and height. All coordinates use the standard screen
    coordinate system where (0,0) is top-left.
    
    Attributes:
        x: X coordinate of the top-left corner
        y: Y coordinate of the top-left corner  
        width: Width of the bounding box (must be >= 0)
        height: Height of the bounding box (must be >= 0)
    """
    
    x: float
    y: float
    width: float
    height: float
    
    @field_validator('x', 'y', 'width', 'height')
    @classmethod
    def validate_coordinates(cls, value: float, info) -> float:
        """Validate that coordinates are finite numbers."""
        return validate_finite_number(value, info.field_name)
    
    @field_validator('width', 'height')
    @classmethod
    def validate_dimensions(cls, value: float, info) -> float:
        """Validate that dimensions are non-negative."""
        if value < 0:
            raise ValueError(f"{info.field_name} must be non-negative, got {value}")
        return value
    
    @classmethod
    def from_points(cls, point1: Point2D, point2: Point2D) -> "BoundingBox":
        """Create a bounding box from two corner points.
        
        Args:
            point1: First corner point
            point2: Second corner point (can be any corner)
            
        Returns:
            BoundingBox that encompasses both points
        """
        min_x = min(point1.x, point2.x)
        max_x = max(point1.x, point2.x)
        min_y = min(point1.y, point2.y)
        max_y = max(point1.y, point2.y)
        
        return cls(
            x=min_x,
            y=min_y,
            width=max_x - min_x,
            height=max_y - min_y
        )
    
    @classmethod
    def from_center(cls, center: Point2D, width: float, height: float) -> "BoundingBox":
        """Create a bounding box from center point and dimensions.
        
        Args:
            center: Center point of the bounding box
            width: Width of the bounding box
            height: Height of the bounding box
            
        Returns:
            BoundingBox centered at the given point
        """
        return cls(
            x=center.x - width / 2,
            y=center.y - height / 2,
            width=width,
            height=height
        )
    
    @classmethod
    def empty(cls) -> "BoundingBox":
        """Create an empty bounding box.
        
        Returns:
            BoundingBox with zero dimensions
        """
        return cls(x=0, y=0, width=0, height=0)
    
    def is_empty(self) -> bool:
        """Check if the bounding box is empty.
        
        Returns:
            True if width or height is zero
        """
        return self.width == 0 or self.height == 0
    
    def area(self) -> float:
        """Calculate the area of the bounding box.
        
        Returns:
            Area as width * height
        """
        return self.width * self.height
    
    def perimeter(self) -> float:
        """Calculate the perimeter of the bounding box.
        
        Returns:
            Perimeter as 2 * (width + height)
        """
        return 2 * (self.width + self.height)
    
    def center(self) -> Point2D:
        """Get the center point of the bounding box.
        
        Returns:
            Center point
        """
        return Point2D(
            x=self.x + self.width / 2,
            y=self.y + self.height / 2
        )
    
    def top_left(self) -> Point2D:
        """Get the top-left corner point.
        
        Returns:
            Top-left corner point
        """
        return Point2D(x=self.x, y=self.y)
    
    def top_right(self) -> Point2D:
        """Get the top-right corner point.
        
        Returns:
            Top-right corner point
        """
        return Point2D(x=self.x + self.width, y=self.y)
    
    def bottom_left(self) -> Point2D:
        """Get the bottom-left corner point.
        
        Returns:
            Bottom-left corner point
        """
        return Point2D(x=self.x, y=self.y + self.height)
    
    def bottom_right(self) -> Point2D:
        """Get the bottom-right corner point.
        
        Returns:
            Bottom-right corner point
        """
        return Point2D(x=self.x + self.width, y=self.y + self.height)
    
    def corners(self) -> List[Point2D]:
        """Get all four corner points.
        
        Returns:
            List of corner points [top_left, top_right, bottom_right, bottom_left]
        """
        return [
            self.top_left(),
            self.top_right(),
            self.bottom_right(),
            self.bottom_left()
        ]
    
    def min_point(self) -> Point2D:
        """Get the minimum point (top-left corner).
        
        Returns:
            Minimum point
        """
        return Point2D(x=self.x, y=self.y)
    
    def max_point(self) -> Point2D:
        """Get the maximum point (bottom-right corner).
        
        Returns:
            Maximum point
        """
        return Point2D(x=self.x + self.width, y=self.y + self.height)
    
    def contains_point(self, point: Point2D) -> bool:
        """Check if a point is inside the bounding box.
        
        Args:
            point: Point to check
            
        Returns:
            True if point is inside or on the boundary
        """
        return (self.x <= point.x <= self.x + self.width and
                self.y <= point.y <= self.y + self.height)
    
    def contains_box(self, other: "BoundingBox") -> bool:
        """Check if another bounding box is entirely contained within this one.
        
        Args:
            other: Another bounding box
            
        Returns:
            True if other box is entirely contained
        """
        return (self.x <= other.x and
                self.y <= other.y and
                self.x + self.width >= other.x + other.width and
                self.y + self.height >= other.y + other.height)
    
    def intersects(self, other: "BoundingBox") -> bool:
        """Check if this bounding box intersects with another.
        
        Args:
            other: Another bounding box
            
        Returns:
            True if boxes intersect (touching counts as intersecting)
        """
        return not (self.x + self.width < other.x or
                   other.x + other.width < self.x or
                   self.y + self.height < other.y or
                   other.y + other.height < self.y)
    
    def intersection(self, other: "BoundingBox") -> Optional["BoundingBox"]:
        """Get the intersection of this bounding box with another.
        
        Args:
            other: Another bounding box
            
        Returns:
            Intersection bounding box, or None if no intersection
        """
        if not self.intersects(other):
            return None
        
        left = max(self.x, other.x)
        top = max(self.y, other.y)
        right = min(self.x + self.width, other.x + other.width)
        bottom = min(self.y + self.height, other.y + other.height)
        
        return BoundingBox(
            x=left,
            y=top,
            width=right - left,
            height=bottom - top
        )
    
    def union(self, other: "BoundingBox") -> "BoundingBox":
        """Get the union of this bounding box with another.
        
        Args:
            other: Another bounding box
            
        Returns:
            Union bounding box that encompasses both boxes
        """
        if self.is_empty():
            return other
        if other.is_empty():
            return self
        
        left = min(self.x, other.x)
        top = min(self.y, other.y)
        right = max(self.x + self.width, other.x + other.width)
        bottom = max(self.y + self.height, other.y + other.height)
        
        return BoundingBox(
            x=left,
            y=top,
            width=right - left,
            height=bottom - top
        )
    
    def expand(self, margin: float) -> "BoundingBox":
        """Expand the bounding box by a margin in all directions.
        
        Args:
            margin: Margin to add (can be negative to shrink)
            
        Returns:
            Expanded bounding box
        """
        return BoundingBox(
            x=self.x - margin,
            y=self.y - margin,
            width=self.width + 2 * margin,
            height=self.height + 2 * margin
        )
    
    def expand_to_point(self, point: Point2D) -> "BoundingBox":
        """Expand the bounding box to include a point.
        
        Args:
            point: Point to include
            
        Returns:
            Expanded bounding box
        """
        if self.is_empty():
            return BoundingBox(x=point.x, y=point.y, width=0, height=0)
        
        left = min(self.x, point.x)
        top = min(self.y, point.y)
        right = max(self.x + self.width, point.x)
        bottom = max(self.y + self.height, point.y)
        
        return BoundingBox(
            x=left,
            y=top,
            width=right - left,
            height=bottom - top
        )
    
    def translate(self, dx: float, dy: float) -> "BoundingBox":
        """Translate the bounding box by given offsets.
        
        Args:
            dx: X offset
            dy: Y offset
            
        Returns:
            Translated bounding box
        """
        return BoundingBox(
            x=self.x + dx,
            y=self.y + dy,
            width=self.width,
            height=self.height
        )
    
    def scale(self, factor: float) -> "BoundingBox":
        """Scale the bounding box by a factor.
        
        Args:
            factor: Scale factor
            
        Returns:
            Scaled bounding box
        """
        return BoundingBox(
            x=self.x * factor,
            y=self.y * factor,
            width=self.width * factor,
            height=self.height * factor
        )
    
    def scale_from_center(self, factor: float) -> "BoundingBox":
        """Scale the bounding box from its center.
        
        Args:
            factor: Scale factor
            
        Returns:
            Scaled bounding box
        """
        center = self.center()
        new_width = self.width * factor
        new_height = self.height * factor
        
        return BoundingBox(
            x=center.x - new_width / 2,
            y=center.y - new_height / 2,
            width=new_width,
            height=new_height
        )
    
    def __eq__(self, other: object) -> bool:
        """Check equality with another bounding box.
        
        Args:
            other: Another object to compare with
            
        Returns:
            True if bounding boxes are equal
        """
        if not isinstance(other, BoundingBox):
            return False
        
        epsilon = 1e-10
        return (abs(self.x - other.x) < epsilon and
                abs(self.y - other.y) < epsilon and
                abs(self.width - other.width) < epsilon and
                abs(self.height - other.height) < epsilon)
    
    def __hash__(self) -> int:
        """Get hash of the bounding box."""
        return hash((
            round(self.x, 10),
            round(self.y, 10),
            round(self.width, 10),
            round(self.height, 10)
        ))
    
    def __str__(self) -> str:
        """String representation of the bounding box."""
        return f"BoundingBox(x={self.x:.3f}, y={self.y:.3f}, w={self.width:.3f}, h={self.height:.3f})"
    
    def __repr__(self) -> str:
        """Detailed string representation."""
        return f"BoundingBox(x={self.x}, y={self.y}, width={self.width}, height={self.height})"