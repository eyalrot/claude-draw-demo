"""Shape primitives for Claude Draw.

This module implements the concrete primitive shapes that form the visual
building blocks of the Claude Draw graphics library. Each shape is a
self-contained, immutable object that can be styled, transformed, and
composed into more complex graphics.

Available shapes:
- Circle: Perfect circle defined by center and radius
- Rectangle: Axis-aligned rectangle with position and dimensions
- Ellipse: General ellipse with independent horizontal/vertical radii
- Line: Straight line segment between two points

All shapes inherit from the Primitive base class, providing:
- Immutability for thread safety and predictable behavior
- Full styling support (fill, stroke, opacity)
- 2D transformations (translate, rotate, scale)
- Visitor pattern integration for extensible rendering
- Automatic bounds calculation
- Validation of geometric constraints

Design principles:
- Shapes store minimal geometric data (e.g., center + radius for circles)
- All derived properties are computed on demand
- Validation ensures geometric validity (e.g., positive dimensions)
- Methods return new instances to maintain immutability
"""

from typing import Any
from pydantic import Field, field_validator
import math

from claude_draw.base import Primitive
from claude_draw.models.point import Point2D
from claude_draw.models.bounding_box import BoundingBox


class Circle(Primitive):
    """A perfect circle shape defined by center point and radius.
    
    The Circle class represents a perfect circular shape in 2D space. It is
    one of the most fundamental geometric primitives, defined by just two
    properties: a center point and a radius.
    
    Mathematical definition:
        All points (x, y) where: (x - cx)² + (y - cy)² = r²
        Where (cx, cy) is the center and r is the radius
    
    Key properties:
    - **Simplicity**: Only requires center and radius
    - **Symmetry**: Perfectly symmetric in all directions
    - **Efficiency**: Fast hit testing and bounds calculation
    - **Scalability**: Uniform scaling preserves shape
    
    Common use cases:
    - Data visualization (scatter plots, bubble charts)
    - UI elements (buttons, indicators, avatars)
    - Diagrams (nodes in graphs, Venn diagrams)
    - Decorative elements and patterns
    
    Attributes:
        center (Point2D): The center point of the circle
        radius (float): The radius of the circle (must be positive)
    
    Example:
        >>> # Create a red circle at origin with radius 50
        >>> circle = Circle(
        ...     center=Point2D(x=0, y=0),
        ...     radius=50,
        ...     fill=Color(r=255, g=0, b=0)
        ... )
        >>> 
        >>> # Move it to a new position
        >>> moved = circle.with_center(Point2D(x=100, y=100))
        >>> 
        >>> # Check if a point is inside
        >>> circle.contains_point(Point2D(x=25, y=25))  # True
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
        """Calculate the axis-aligned bounding box of the circle.
        
        The bounding box of a circle is a square with sides equal to the
        diameter (2 * radius). The box is centered on the circle's center.
        
        Calculation:
        - Top-left corner: (center.x - radius, center.y - radius)
        - Width and height: 2 * radius
        
        Note: This returns the bounds in the circle's local coordinate space.
        Any transformations applied to the circle would need to be applied
        to these bounds for world-space calculations.
        
        Returns:
            BoundingBox: A square bounding box with width = height = 2 * radius,
                positioned such that the circle touches all four sides
                
        Example:
            >>> circle = Circle(center=Point2D(x=50, y=50), radius=30)
            >>> bounds = circle.get_bounds()
            >>> assert bounds.x == 20  # 50 - 30
            >>> assert bounds.y == 20  # 50 - 30  
            >>> assert bounds.width == 60  # 2 * 30
            >>> assert bounds.height == 60  # 2 * 30
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
    """An axis-aligned rectangle defined by position and dimensions.
    
    The Rectangle class represents a four-sided polygon with right angles
    at each corner, aligned with the coordinate axes. It's one of the most
    versatile and commonly used shapes in graphics programming.
    
    Coordinate system:
    - Position (x, y) represents the top-left corner
    - Width extends to the right (positive x direction)
    - Height extends downward (positive y direction)
    - All four corners are at right angles (90 degrees)
    
    Key properties:
    - **Axis-aligned**: Edges parallel to x and y axes (no rotation)
    - **Simple definition**: Just position and size needed
    - **Efficient operations**: Fast hit testing and clipping
    - **Versatile**: Can represent UI elements, bounds, frames
    
    Common use cases:
    - UI components (buttons, panels, text boxes)
    - Image and video frames
    - Bounding boxes for collision detection
    - Chart bars and histogram bins
    - Layout containers and grids
    
    Attributes:
        x (float): X coordinate of the top-left corner
        y (float): Y coordinate of the top-left corner
        width (float): Horizontal size (must be positive)
        height (float): Vertical size (must be positive)
    
    Computed properties:
        - center: The geometric center point
        - corners: top_left, top_right, bottom_left, bottom_right
        - area: width * height
        - perimeter: 2 * (width + height)
        - is_square: True if width equals height
    
    Example:
        >>> # Create a blue rectangle
        >>> rect = Rectangle(
        ...     x=10, y=20,
        ...     width=100, height=50,
        ...     fill=Color(r=0, g=0, b=255)
        ... )
        >>> 
        >>> # Get the center point
        >>> center = rect.center  # Point2D(x=60, y=45)
        >>> 
        >>> # Create a square
        >>> square = Rectangle(x=0, y=0, width=50, height=50)
        >>> assert square.is_square()
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
    """A general ellipse shape defined by center and two radii.
    
    The Ellipse class represents a closed curve where the sum of distances
    from any point on the curve to two fixed points (foci) is constant.
    It's a generalization of a circle, allowing different radii in the
    horizontal and vertical directions.
    
    Mathematical definition:
        All points (x, y) where: ((x-cx)/rx)² + ((y-cy)/ry)² = 1
        Where (cx, cy) is the center, rx is horizontal radius, ry is vertical radius
    
    Key properties:
    - **Generality**: Includes circles as special case (rx = ry)
    - **Dual radii**: Independent control of width and height
    - **Smooth curve**: No corners or discontinuities
    - **Symmetry**: Symmetric about both axes through center
    
    Common use cases:
    - Data visualization (confidence ellipses, orbital paths)
    - UI design (oval buttons, pill-shaped elements)
    - Technical diagrams (cylinders in perspective)
    - Artistic elements (organic shapes, shadows)
    
    Attributes:
        center (Point2D): The center point of the ellipse
        rx (float): Horizontal radius (must be positive)
        ry (float): Vertical radius (must be positive)
    
    Special characteristics:
    - When rx = ry, the ellipse is a circle
    - Area = π * rx * ry
    - No simple formula for perimeter (uses approximation)
    - Foci located at c * center ± sqrt(|rx² - ry²|)
    
    Example:
        >>> # Create a horizontal ellipse
        >>> ellipse = Ellipse(
        ...     center=Point2D(x=100, y=100),
        ...     rx=50,  # Wide
        ...     ry=30,  # Short
        ...     fill=Color.from_hex("#00FF00")
        ... )
        >>> 
        >>> # Check if it's actually a circle
        >>> ellipse.is_circle()  # False
        >>> 
        >>> # Create a circle using Ellipse
        >>> circle_as_ellipse = Ellipse(
        ...     center=Point2D(x=0, y=0),
        ...     rx=40,
        ...     ry=40
        ... )
        >>> circle_as_ellipse.is_circle()  # True
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
    """A straight line segment between two points.
    
    The Line class represents a straight line segment in 2D space, defined
    by its start and end points. Unlike infinite lines in mathematics, this
    represents a finite segment with specific endpoints.
    
    Mathematical properties:
    - Direction vector: (end - start)
    - Length: |end - start| (Euclidean distance)
    - Slope: (end.y - start.y) / (end.x - start.x)
    - Angle: atan2(end.y - start.y, end.x - start.x)
    
    Key characteristics:
    - **Simplicity**: Just two points needed
    - **Directionality**: Has a defined start and end
    - **No area**: Infinitesimally thin (styled with stroke only)
    - **Degenerate case**: Can be a point if start = end
    
    Common use cases:
    - Connectors in diagrams (arrows, relationships)
    - Axes and grid lines in charts
    - Geometric constructions and technical drawings  
    - Path segments and polylines (when combined)
    - Dividers and separators in UI
    
    Attributes:
        start (Point2D): The starting point of the line segment
        end (Point2D): The ending point of the line segment
    
    Computed properties:
        - length: Euclidean distance between endpoints
        - midpoint: Point halfway between start and end
        - slope: Rise over run (undefined for vertical lines)
        - angle: Direction angle in radians
        - is_horizontal/is_vertical: Axis alignment checks
    
    Styling notes:
        - Lines are typically styled with stroke only (no fill)
        - Stroke width affects visual thickness
        - End caps and join styles may apply (implementation-specific)
    
    Example:
        >>> # Create a diagonal line
        >>> line = Line(
        ...     start=Point2D(x=0, y=0),
        ...     end=Point2D(x=100, y=100),
        ...     stroke=Color(r=0, g=0, b=0),
        ...     stroke_width=2
        ... )
        >>> 
        >>> # Get line properties
        >>> line.length()  # ~141.42 (sqrt(2) * 100)
        >>> line.angle()   # ~0.785 radians (45 degrees)
        >>> 
        >>> # Create a horizontal line
        >>> h_line = Line(
        ...     start=Point2D(x=0, y=50),
        ...     end=Point2D(x=100, y=50)
        ... )
        >>> h_line.is_horizontal()  # True
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