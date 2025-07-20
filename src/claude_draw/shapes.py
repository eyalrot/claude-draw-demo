"""Shape primitives for Claude Draw."""

from typing import Any
from pydantic import Field, field_validator
import math

from claude_draw.base import Primitive
from claude_draw.models.point import Point2D
from claude_draw.models.bounding_box import BoundingBox


class Circle(Primitive):
    """Circle shape with center point and radius.
    
    A circle is defined by its center point and radius. The bounding box
    is calculated as center ± radius in both dimensions.
    """
    
    center: Point2D = Field(description="Center point of the circle")
    radius: float = Field(gt=0.0, description="Radius of the circle (must be positive)")
    
    @field_validator('radius')
    @classmethod
    def validate_radius(cls, v):
        """Validate that radius is positive."""
        if v <= 0:
            raise ValueError("Circle radius must be positive")
        return v
    
    def get_bounds(self) -> BoundingBox:
        """Calculate the bounding box of the circle.
        
        Returns:
            BoundingBox: The bounding box containing the circle
        """
        return BoundingBox(
            x=self.center.x - self.radius,
            y=self.center.y - self.radius,
            width=2 * self.radius,
            height=2 * self.radius
        )
    
    def accept(self, visitor) -> Any:
        """Accept a visitor for processing this circle.
        
        Args:
            visitor: The visitor object
            
        Returns:
            Any: Result of visitor processing
        """
        return visitor.visit_circle(self)
    
    def with_center(self, center: Point2D) -> "Circle":
        """Create a new circle with the specified center.
        
        Args:
            center: New center point
            
        Returns:
            Circle: New circle with updated center
        """
        return self.model_copy(update={"center": center})
    
    def with_radius(self, radius: float) -> "Circle":
        """Create a new circle with the specified radius.
        
        Args:
            radius: New radius (must be positive)
            
        Returns:
            Circle: New circle with updated radius
        """
        return self.model_copy(update={"radius": radius})
    
    def contains_point(self, point: Point2D) -> bool:
        """Check if a point is inside the circle.
        
        Args:
            point: Point to check
            
        Returns:
            bool: True if point is inside or on the circle boundary
        """
        distance = self.center.distance_to(point)
        return distance <= self.radius
    
    def area(self) -> float:
        """Calculate the area of the circle.
        
        Returns:
            float: Area of the circle (π * r²)
        """
        return math.pi * self.radius * self.radius
    
    def circumference(self) -> float:
        """Calculate the circumference of the circle.
        
        Returns:
            float: Circumference of the circle (2 * π * r)
        """
        return 2 * math.pi * self.radius
    
    def scaled(self, factor: float) -> "Circle":
        """Create a new circle scaled by the given factor.
        
        Args:
            factor: Scale factor (must be positive)
            
        Returns:
            Circle: New circle with scaled radius
        """
        if factor <= 0:
            raise ValueError("Scale factor must be positive")
        return self.with_radius(self.radius * factor)


class Rectangle(Primitive):
    """Rectangle shape with position and dimensions.
    
    A rectangle is defined by its position (x, y) and dimensions (width, height).
    The position represents the top-left corner of the rectangle.
    """
    
    x: float = Field(description="X coordinate of the top-left corner")
    y: float = Field(description="Y coordinate of the top-left corner")
    width: float = Field(gt=0.0, description="Width of the rectangle (must be positive)")
    height: float = Field(gt=0.0, description="Height of the rectangle (must be positive)")
    
    @field_validator('width')
    @classmethod
    def validate_width(cls, v):
        """Validate that width is positive."""
        if v <= 0:
            raise ValueError("Rectangle width must be positive")
        return v
    
    @field_validator('height')
    @classmethod
    def validate_height(cls, v):
        """Validate that height is positive."""
        if v <= 0:
            raise ValueError("Rectangle height must be positive")
        return v
    
    def get_bounds(self) -> BoundingBox:
        """Calculate the bounding box of the rectangle.
        
        Returns:
            BoundingBox: The bounding box containing the rectangle
        """
        return BoundingBox(
            x=self.x,
            y=self.y,
            width=self.width,
            height=self.height
        )
    
    def accept(self, visitor) -> Any:
        """Accept a visitor for processing this rectangle.
        
        Args:
            visitor: The visitor object
            
        Returns:
            Any: Result of visitor processing
        """
        return visitor.visit_rectangle(self)
    
    @property
    def position(self) -> Point2D:
        """Get the position (top-left corner) of the rectangle.
        
        Returns:
            Point2D: Position of the rectangle
        """
        return Point2D(x=self.x, y=self.y)
    
    @property
    def center(self) -> Point2D:
        """Get the center point of the rectangle.
        
        Returns:
            Point2D: Center point of the rectangle
        """
        return Point2D(
            x=self.x + self.width / 2,
            y=self.y + self.height / 2
        )
    
    @property
    def top_left(self) -> Point2D:
        """Get the top-left corner of the rectangle.
        
        Returns:
            Point2D: Top-left corner
        """
        return Point2D(x=self.x, y=self.y)
    
    @property
    def top_right(self) -> Point2D:
        """Get the top-right corner of the rectangle.
        
        Returns:
            Point2D: Top-right corner
        """
        return Point2D(x=self.x + self.width, y=self.y)
    
    @property
    def bottom_left(self) -> Point2D:
        """Get the bottom-left corner of the rectangle.
        
        Returns:
            Point2D: Bottom-left corner
        """
        return Point2D(x=self.x, y=self.y + self.height)
    
    @property
    def bottom_right(self) -> Point2D:
        """Get the bottom-right corner of the rectangle.
        
        Returns:
            Point2D: Bottom-right corner
        """
        return Point2D(x=self.x + self.width, y=self.y + self.height)
    
    def with_position(self, x: float, y: float) -> "Rectangle":
        """Create a new rectangle with the specified position.
        
        Args:
            x: New X coordinate
            y: New Y coordinate
            
        Returns:
            Rectangle: New rectangle with updated position
        """
        return self.model_copy(update={"x": x, "y": y})
    
    def with_size(self, width: float, height: float) -> "Rectangle":
        """Create a new rectangle with the specified size.
        
        Args:
            width: New width (must be positive)
            height: New height (must be positive)
            
        Returns:
            Rectangle: New rectangle with updated size
        """
        return self.model_copy(update={"width": width, "height": height})
    
    def contains_point(self, point: Point2D) -> bool:
        """Check if a point is inside the rectangle.
        
        Args:
            point: Point to check
            
        Returns:
            bool: True if point is inside or on the rectangle boundary
        """
        return (self.x <= point.x <= self.x + self.width and
                self.y <= point.y <= self.y + self.height)
    
    def area(self) -> float:
        """Calculate the area of the rectangle.
        
        Returns:
            float: Area of the rectangle (width * height)
        """
        return self.width * self.height
    
    def perimeter(self) -> float:
        """Calculate the perimeter of the rectangle.
        
        Returns:
            float: Perimeter of the rectangle (2 * (width + height))
        """
        return 2 * (self.width + self.height)
    
    def is_square(self) -> bool:
        """Check if the rectangle is a square.
        
        Returns:
            bool: True if width equals height
        """
        return abs(self.width - self.height) < 1e-10


class Ellipse(Primitive):
    """Ellipse shape with center point and two radii.
    
    An ellipse is defined by its center point and two radii (rx, ry).
    The bounding box is calculated as center ± radii in each dimension.
    """
    
    center: Point2D = Field(description="Center point of the ellipse")
    rx: float = Field(gt=0.0, description="Horizontal radius (must be positive)")
    ry: float = Field(gt=0.0, description="Vertical radius (must be positive)")
    
    @field_validator('rx')
    @classmethod
    def validate_rx(cls, v):
        """Validate that horizontal radius is positive."""
        if v <= 0:
            raise ValueError("Ellipse horizontal radius must be positive")
        return v
    
    @field_validator('ry')
    @classmethod
    def validate_ry(cls, v):
        """Validate that vertical radius is positive."""
        if v <= 0:
            raise ValueError("Ellipse vertical radius must be positive")
        return v
    
    def get_bounds(self) -> BoundingBox:
        """Calculate the bounding box of the ellipse.
        
        Returns:
            BoundingBox: The bounding box containing the ellipse
        """
        return BoundingBox(
            x=self.center.x - self.rx,
            y=self.center.y - self.ry,
            width=2 * self.rx,
            height=2 * self.ry
        )
    
    def accept(self, visitor) -> Any:
        """Accept a visitor for processing this ellipse.
        
        Args:
            visitor: The visitor object
            
        Returns:
            Any: Result of visitor processing
        """
        return visitor.visit_ellipse(self)
    
    def with_center(self, center: Point2D) -> "Ellipse":
        """Create a new ellipse with the specified center.
        
        Args:
            center: New center point
            
        Returns:
            Ellipse: New ellipse with updated center
        """
        return self.model_copy(update={"center": center})
    
    def with_radii(self, rx: float, ry: float) -> "Ellipse":
        """Create a new ellipse with the specified radii.
        
        Args:
            rx: New horizontal radius (must be positive)
            ry: New vertical radius (must be positive)
            
        Returns:
            Ellipse: New ellipse with updated radii
        """
        return self.model_copy(update={"rx": rx, "ry": ry})
    
    def contains_point(self, point: Point2D) -> bool:
        """Check if a point is inside the ellipse.
        
        Uses the ellipse equation: (x-cx)²/rx² + (y-cy)²/ry² <= 1
        
        Args:
            point: Point to check
            
        Returns:
            bool: True if point is inside or on the ellipse boundary
        """
        dx = point.x - self.center.x
        dy = point.y - self.center.y
        return (dx * dx) / (self.rx * self.rx) + (dy * dy) / (self.ry * self.ry) <= 1.0
    
    def area(self) -> float:
        """Calculate the area of the ellipse.
        
        Returns:
            float: Area of the ellipse (π * rx * ry)
        """
        return math.pi * self.rx * self.ry
    
    def perimeter(self) -> float:
        """Calculate the approximate perimeter of the ellipse.
        
        Uses Ramanujan's approximation for ellipse perimeter.
        
        Returns:
            float: Approximate perimeter of the ellipse
        """
        # Ramanujan's approximation: π * (3(a+b) - sqrt((3a+b)(a+3b)))
        a, b = self.rx, self.ry
        return math.pi * (3 * (a + b) - math.sqrt((3 * a + b) * (a + 3 * b)))
    
    def is_circle(self) -> bool:
        """Check if the ellipse is a circle.
        
        Returns:
            bool: True if horizontal and vertical radii are equal
        """
        return abs(self.rx - self.ry) < 1e-10
    
    def scaled(self, factor_x: float, factor_y: float = None) -> "Ellipse":
        """Create a new ellipse scaled by the given factors.
        
        Args:
            factor_x: Scale factor for horizontal radius
            factor_y: Scale factor for vertical radius (defaults to factor_x)
            
        Returns:
            Ellipse: New ellipse with scaled radii
        """
        if factor_y is None:
            factor_y = factor_x
        
        if factor_x <= 0 or factor_y <= 0:
            raise ValueError("Scale factors must be positive")
        
        return self.with_radii(self.rx * factor_x, self.ry * factor_y)


class Line(Primitive):
    """Line shape with start and end points.
    
    A line is defined by its start and end points. The bounding box
    is calculated from the min/max coordinates of the endpoints.
    """
    
    start: Point2D = Field(description="Start point of the line")
    end: Point2D = Field(description="End point of the line")
    
    def get_bounds(self) -> BoundingBox:
        """Calculate the bounding box of the line.
        
        Returns:
            BoundingBox: The bounding box containing the line
        """
        min_x = min(self.start.x, self.end.x)
        max_x = max(self.start.x, self.end.x)
        min_y = min(self.start.y, self.end.y)
        max_y = max(self.start.y, self.end.y)
        
        return BoundingBox(
            x=min_x,
            y=min_y,
            width=max_x - min_x,
            height=max_y - min_y
        )
    
    def accept(self, visitor) -> Any:
        """Accept a visitor for processing this line.
        
        Args:
            visitor: The visitor object
            
        Returns:
            Any: Result of visitor processing
        """
        return visitor.visit_line(self)
    
    def with_start(self, start: Point2D) -> "Line":
        """Create a new line with the specified start point.
        
        Args:
            start: New start point
            
        Returns:
            Line: New line with updated start point
        """
        return self.model_copy(update={"start": start})
    
    def with_end(self, end: Point2D) -> "Line":
        """Create a new line with the specified end point.
        
        Args:
            end: New end point
            
        Returns:
            Line: New line with updated end point
        """
        return self.model_copy(update={"end": end})
    
    def length(self) -> float:
        """Calculate the length of the line.
        
        Returns:
            float: Length of the line
        """
        return self.start.distance_to(self.end)
    
    def midpoint(self) -> Point2D:
        """Calculate the midpoint of the line.
        
        Returns:
            Point2D: Midpoint of the line
        """
        return Point2D(
            x=(self.start.x + self.end.x) / 2,
            y=(self.start.y + self.end.y) / 2
        )
    
    def slope(self) -> float:
        """Calculate the slope of the line.
        
        Returns:
            float: Slope of the line (rise/run)
            
        Raises:
            ValueError: If the line is vertical (infinite slope)
        """
        if abs(self.end.x - self.start.x) < 1e-10:
            raise ValueError("Vertical line has infinite slope")
        
        return (self.end.y - self.start.y) / (self.end.x - self.start.x)
    
    def angle(self) -> float:
        """Calculate the angle of the line in radians.
        
        Returns:
            float: Angle from start to end point in radians
        """
        return math.atan2(self.end.y - self.start.y, self.end.x - self.start.x)
    
    def is_horizontal(self) -> bool:
        """Check if the line is horizontal.
        
        Returns:
            bool: True if start and end have the same y coordinate
        """
        return abs(self.start.y - self.end.y) < 1e-10
    
    def is_vertical(self) -> bool:
        """Check if the line is vertical.
        
        Returns:
            bool: True if start and end have the same x coordinate
        """
        return abs(self.start.x - self.end.x) < 1e-10
    
    def is_point(self) -> bool:
        """Check if the line is actually a point (zero length).
        
        Returns:
            bool: True if start and end are the same point
        """
        return self.start.distance_to(self.end) < 1e-10