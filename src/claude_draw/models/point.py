"""Point2D model for representing 2D coordinates."""

import math
from typing import Self
from pydantic import field_validator

from claude_draw.models.base import DrawModel
from claude_draw.models.validators import validate_finite_number


class Point2D(DrawModel):
    """A point in 2D space with x and y coordinates.
    
    Attributes:
        x: X coordinate
        y: Y coordinate
    """
    
    x: float
    y: float
    
    @field_validator('x', 'y')
    @classmethod
    def validate_coordinate(cls, value: float, info) -> float:
        """Validate that coordinates are finite numbers.
        
        Args:
            value: The coordinate value
            info: Field validation info
            
        Returns:
            The validated coordinate
            
        Raises:
            ValueError: If the coordinate is not finite
        """
        return validate_finite_number(value, info.field_name)
    
    @classmethod
    def origin(cls) -> "Point2D":
        """Create a point at the origin (0, 0).
        
        Returns:
            Point at origin
        """
        return cls(x=0.0, y=0.0)
    
    def __str__(self) -> str:
        """String representation of the point.
        
        Returns:
            String in format "Point2D(x, y)"
        """
        return f"Point2D({self.x}, {self.y})"
    
    def __repr__(self) -> str:
        """Detailed string representation.
        
        Returns:
            String representation with field names
        """
        return f"Point2D(x={self.x}, y={self.y})"
    
    def __eq__(self, other: object) -> bool:
        """Check equality with another point.
        
        Args:
            other: Another object to compare with
            
        Returns:
            True if points are equal
        """
        if not isinstance(other, Point2D):
            return False
        return self.x == other.x and self.y == other.y
    
    def __hash__(self) -> int:
        """Get hash of the point.
        
        Returns:
            Hash value
        """
        return hash((self.x, self.y))
    
    def __add__(self, other: "Point2D") -> "Point2D":
        """Add two points (vector addition).
        
        Args:
            other: Another point
            
        Returns:
            New point with summed coordinates
        """
        return Point2D(x=self.x + other.x, y=self.y + other.y)
    
    def __sub__(self, other: "Point2D") -> "Point2D":
        """Subtract two points (vector subtraction).
        
        Args:
            other: Another point
            
        Returns:
            New point with difference of coordinates
        """
        return Point2D(x=self.x - other.x, y=self.y - other.y)
    
    def __mul__(self, scalar: float) -> "Point2D":
        """Multiply point by scalar.
        
        Args:
            scalar: Scalar value
            
        Returns:
            New point with scaled coordinates
        """
        return Point2D(x=self.x * scalar, y=self.y * scalar)
    
    def __rmul__(self, scalar: float) -> "Point2D":
        """Right multiply point by scalar.
        
        Args:
            scalar: Scalar value
            
        Returns:
            New point with scaled coordinates
        """
        return self.__mul__(scalar)
    
    def __truediv__(self, scalar: float) -> "Point2D":
        """Divide point by scalar.
        
        Args:
            scalar: Scalar value
            
        Returns:
            New point with divided coordinates
            
        Raises:
            ZeroDivisionError: If scalar is zero
        """
        if scalar == 0:
            raise ZeroDivisionError("Cannot divide point by zero")
        return Point2D(x=self.x / scalar, y=self.y / scalar)
    
    def __neg__(self) -> "Point2D":
        """Negate the point.
        
        Returns:
            New point with negated coordinates
        """
        return Point2D(x=-self.x, y=-self.y)
    
    def as_tuple(self) -> tuple[float, float]:
        """Get point as a tuple.
        
        Returns:
            Tuple of (x, y)
        """
        return (self.x, self.y)
    
    def distance_to(self, other: "Point2D") -> float:
        """Calculate Euclidean distance to another point.
        
        Args:
            other: Another point
            
        Returns:
            Distance between the points
        """
        dx = self.x - other.x
        dy = self.y - other.y
        return math.sqrt(dx * dx + dy * dy)
    
    def manhattan_distance_to(self, other: "Point2D") -> float:
        """Calculate Manhattan distance to another point.
        
        Args:
            other: Another point
            
        Returns:
            Manhattan distance between the points
        """
        return abs(self.x - other.x) + abs(self.y - other.y)
    
    def magnitude(self) -> float:
        """Get the magnitude (length) of the point vector from origin.
        
        Returns:
            Magnitude of the vector
        """
        return math.sqrt(self.x * self.x + self.y * self.y)
    
    def normalize(self) -> "Point2D":
        """Normalize the point to unit length.
        
        Returns:
            Normalized point (unit vector)
            
        Raises:
            ZeroDivisionError: If the point is at origin
        """
        mag = self.magnitude()
        if mag == 0:
            raise ZeroDivisionError("Cannot normalize zero vector")
        return self / mag
    
    def dot(self, other: "Point2D") -> float:
        """Calculate dot product with another point.
        
        Args:
            other: Another point
            
        Returns:
            Dot product
        """
        return self.x * other.x + self.y * other.y
    
    def cross(self, other: "Point2D") -> float:
        """Calculate 2D cross product with another point.
        
        The 2D cross product returns the z-component of the 3D cross product.
        
        Args:
            other: Another point
            
        Returns:
            Cross product (scalar)
        """
        return self.x * other.y - self.y * other.x
    
    def angle_to(self, other: "Point2D") -> float:
        """Calculate angle to another point in radians.
        
        Args:
            other: Another point
            
        Returns:
            Angle in radians [0, π]
        """
        dot_product = self.dot(other)
        mag_product = self.magnitude() * other.magnitude()
        if mag_product == 0:
            return 0.0
        cos_angle = dot_product / mag_product
        # Clamp to [-1, 1] to handle floating point errors
        cos_angle = max(-1.0, min(1.0, cos_angle))
        return math.acos(cos_angle)
    
    def rotate(self, angle: float, center: "Point2D | None" = None) -> "Point2D":
        """Rotate the point around a center.
        
        Args:
            angle: Angle in radians (counterclockwise)
            center: Center of rotation (default: origin)
            
        Returns:
            Rotated point
        """
        if center is None:
            center = Point2D.origin()
        
        # Translate to origin
        translated = self - center
        
        # Rotate
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)
        x = translated.x * cos_a - translated.y * sin_a
        y = translated.x * sin_a + translated.y * cos_a
        
        # Translate back
        return Point2D(x=x + center.x, y=y + center.y)
    
    def translate(self, dx: float, dy: float) -> "Point2D":
        """Translate the point by given amounts.
        
        Args:
            dx: X translation
            dy: Y translation
            
        Returns:
            Translated point
        """
        return Point2D(x=self.x + dx, y=self.y + dy)
    
    def midpoint(self, other: "Point2D") -> "Point2D":
        """Find the midpoint between this point and another.
        
        Args:
            other: Another point
            
        Returns:
            Midpoint
        """
        return Point2D(
            x=(self.x + other.x) / 2,
            y=(self.y + other.y) / 2
        )
    
    def lerp(self, other: "Point2D", t: float) -> "Point2D":
        """Linear interpolation between this point and another.
        
        Args:
            other: Target point
            t: Interpolation parameter [0, 1]
               0 returns self, 1 returns other
            
        Returns:
            Interpolated point
        """
        return Point2D(
            x=self.x + (other.x - self.x) * t,
            y=self.y + (other.y - self.y) * t
        )
    
    def reflect(self, line_point: "Point2D", line_direction: "Point2D") -> "Point2D":
        """Reflect the point across a line.
        
        Args:
            line_point: A point on the line
            line_direction: Direction vector of the line
            
        Returns:
            Reflected point
        """
        # Normalize line direction
        line_dir = line_direction.normalize()
        
        # Vector from line point to this point
        v = self - line_point
        
        # Project v onto line direction
        proj_length = v.dot(line_dir)
        projection = line_dir * proj_length
        
        # Perpendicular component
        perpendicular = v - projection
        
        # Reflect
        return line_point + projection - perpendicular