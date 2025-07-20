"""Transform2D model for 2D affine transformations."""

import math
from typing import List, Optional, Self, Union
from pydantic import field_validator, model_validator

from claude_draw.models.base import DrawModel
from claude_draw.models.point import Point2D
from claude_draw.models.validators import validate_finite_number


class Transform2D(DrawModel):
    """A 2D affine transformation represented as a 3x3 matrix.
    
    The transformation matrix is stored in column-major order:
    [a c tx]
    [b d ty]
    [0 0 1 ]
    
    Where:
    - a, d: scaling factors
    - b, c: skew/rotation components
    - tx, ty: translation components
    
    Attributes:
        a: X-axis scaling factor
        b: Y-axis skew component
        c: X-axis skew component
        d: Y-axis scaling factor
        tx: X-axis translation
        ty: Y-axis translation
    """
    
    a: float = 1.0
    b: float = 0.0
    c: float = 0.0
    d: float = 1.0
    tx: float = 0.0
    ty: float = 0.0
    
    @field_validator('a', 'b', 'c', 'd', 'tx', 'ty')
    @classmethod
    def validate_matrix_component(cls, value: float, info) -> float:
        """Validate that matrix components are finite numbers."""
        return validate_finite_number(value, info.field_name)
    
    @classmethod
    def identity(cls) -> "Transform2D":
        """Create an identity transform.
        
        Returns:
            Identity transformation matrix
        """
        return cls()
    
    @classmethod
    def from_matrix(cls, matrix: List[List[float]]) -> "Transform2D":
        """Create transform from a 3x3 matrix.
        
        Args:
            matrix: 3x3 matrix as list of lists
            
        Returns:
            Transform2D instance
            
        Raises:
            ValueError: If matrix is not 3x3 or has invalid values
        """
        if len(matrix) != 3 or any(len(row) != 3 for row in matrix):
            raise ValueError("Matrix must be 3x3")
        
        # Check that bottom row is [0, 0, 1]
        if matrix[2] != [0, 0, 1]:
            raise ValueError("Bottom row must be [0, 0, 1] for affine transform")
        
        return cls(
            a=matrix[0][0],
            b=matrix[1][0],
            c=matrix[0][1],
            d=matrix[1][1],
            tx=matrix[0][2],
            ty=matrix[1][2]
        )
    
    def to_matrix(self) -> List[List[float]]:
        """Convert to 3x3 matrix representation.
        
        Returns:
            3x3 matrix as list of lists
        """
        return [
            [self.a, self.c, self.tx],
            [self.b, self.d, self.ty],
            [0.0, 0.0, 1.0]
        ]
    
    def determinant(self) -> float:
        """Calculate the determinant of the transformation matrix.
        
        Returns:
            Determinant value
        """
        return self.a * self.d - self.b * self.c
    
    def is_invertible(self) -> bool:
        """Check if the transformation is invertible.
        
        Returns:
            True if the transformation is invertible
        """
        return abs(self.determinant()) > 1e-10
    
    def inverse(self) -> "Transform2D":
        """Get the inverse transformation.
        
        Returns:
            Inverse transformation
            
        Raises:
            ValueError: If transformation is not invertible
        """
        det = self.determinant()
        if abs(det) < 1e-10:
            raise ValueError("Transform is not invertible (determinant is zero)")
        
        inv_det = 1.0 / det
        
        return Transform2D(
            a=self.d * inv_det,
            b=-self.b * inv_det,
            c=-self.c * inv_det,
            d=self.a * inv_det,
            tx=(self.c * self.ty - self.d * self.tx) * inv_det,
            ty=(self.b * self.tx - self.a * self.ty) * inv_det
        )
    
    def __mul__(self, other: "Transform2D") -> "Transform2D":
        """Multiply (compose) two transformations.
        
        Args:
            other: Another transformation
            
        Returns:
            Composed transformation (self * other)
        """
        return Transform2D(
            a=self.a * other.a + self.c * other.b,
            b=self.b * other.a + self.d * other.b,
            c=self.a * other.c + self.c * other.d,
            d=self.b * other.c + self.d * other.d,
            tx=self.a * other.tx + self.c * other.ty + self.tx,
            ty=self.b * other.tx + self.d * other.ty + self.ty
        )
    
    def transform_point(self, point: Point2D) -> Point2D:
        """Transform a point using this transformation.
        
        Args:
            point: Point to transform
            
        Returns:
            Transformed point
        """
        return Point2D(
            x=self.a * point.x + self.c * point.y + self.tx,
            y=self.b * point.x + self.d * point.y + self.ty
        )
    
    def transform_vector(self, vector: Point2D) -> Point2D:
        """Transform a vector (ignoring translation).
        
        Args:
            vector: Vector to transform
            
        Returns:
            Transformed vector
        """
        return Point2D(
            x=self.a * vector.x + self.c * vector.y,
            y=self.b * vector.x + self.d * vector.y
        )
    
    def __eq__(self, other: object) -> bool:
        """Check equality with another transformation.
        
        Args:
            other: Another object to compare with
            
        Returns:
            True if transformations are equal
        """
        if not isinstance(other, Transform2D):
            return False
        
        epsilon = 1e-10
        return (abs(self.a - other.a) < epsilon and
                abs(self.b - other.b) < epsilon and
                abs(self.c - other.c) < epsilon and
                abs(self.d - other.d) < epsilon and
                abs(self.tx - other.tx) < epsilon and
                abs(self.ty - other.ty) < epsilon)
    
    def __hash__(self) -> int:
        """Get hash of the transformation."""
        return hash((
            round(self.a, 10),
            round(self.b, 10),
            round(self.c, 10),
            round(self.d, 10),
            round(self.tx, 10),
            round(self.ty, 10)
        ))
    
    def __str__(self) -> str:
        """String representation of the transformation."""
        return f"Transform2D([{self.a:.3f}, {self.c:.3f}, {self.tx:.3f}], [{self.b:.3f}, {self.d:.3f}, {self.ty:.3f}])"
    
    def __repr__(self) -> str:
        """Detailed string representation."""
        return f"Transform2D(a={self.a}, b={self.b}, c={self.c}, d={self.d}, tx={self.tx}, ty={self.ty})"
    
    def is_identity(self) -> bool:
        """Check if this is an identity transformation.
        
        Returns:
            True if this is an identity transformation
        """
        epsilon = 1e-10
        return (abs(self.a - 1.0) < epsilon and
                abs(self.b) < epsilon and
                abs(self.c) < epsilon and
                abs(self.d - 1.0) < epsilon and
                abs(self.tx) < epsilon and
                abs(self.ty) < epsilon)
    
    def is_translation(self) -> bool:
        """Check if this is a pure translation.
        
        Returns:
            True if this is a pure translation
        """
        epsilon = 1e-10
        return (abs(self.a - 1.0) < epsilon and
                abs(self.b) < epsilon and
                abs(self.c) < epsilon and
                abs(self.d - 1.0) < epsilon and
                (abs(self.tx) > epsilon or abs(self.ty) > epsilon))
    
    def is_scaling(self) -> bool:
        """Check if this is a pure scaling transformation.
        
        Returns:
            True if this is a pure scaling transformation
        """
        epsilon = 1e-10
        return (abs(self.b) < epsilon and
                abs(self.c) < epsilon and
                abs(self.tx) < epsilon and
                abs(self.ty) < epsilon and
                (abs(self.a - 1.0) > epsilon or abs(self.d - 1.0) > epsilon))
    
    def is_rotation(self) -> bool:
        """Check if this is a pure rotation transformation.
        
        Returns:
            True if this is a pure rotation transformation (not identity)
        """
        epsilon = 1e-10
        # For rotation: a = d = cos(θ), b = -sin(θ), c = sin(θ)
        # But exclude identity matrix (rotation by 0 degrees)
        is_rotation_matrix = (abs(self.a - self.d) < epsilon and
                             abs(self.b + self.c) < epsilon and
                             abs(self.tx) < epsilon and
                             abs(self.ty) < epsilon and
                             abs(self.a * self.a + self.b * self.b - 1.0) < epsilon)
        
        # Exclude identity matrix (not considered a rotation)
        is_identity = (abs(self.a - 1.0) < epsilon and 
                      abs(self.b) < epsilon and
                      abs(self.c) < epsilon and
                      abs(self.d - 1.0) < epsilon)
        
        return is_rotation_matrix and not is_identity
    
    def get_scale_factors(self) -> tuple[float, float]:
        """Get the scale factors in X and Y directions.
        
        Returns:
            Tuple of (scale_x, scale_y)
        """
        # Calculate scale factors from the transformation matrix
        scale_x = math.sqrt(self.a * self.a + self.b * self.b)
        scale_y = math.sqrt(self.c * self.c + self.d * self.d)
        
        # Check for reflection (negative determinant)
        if self.determinant() < 0:
            scale_x = -scale_x
        
        return (scale_x, scale_y)
    
    def get_rotation_angle(self) -> float:
        """Get the rotation angle in radians.
        
        Returns:
            Rotation angle in radians
        """
        return math.atan2(self.b, self.a)
    
    def get_translation(self) -> Point2D:
        """Get the translation component.
        
        Returns:
            Translation as a Point2D
        """
        return Point2D(x=self.tx, y=self.ty)
    
    @classmethod
    def translate(cls, dx: float, dy: float) -> "Transform2D":
        """Create a translation transformation.
        
        Args:
            dx: Translation in X direction
            dy: Translation in Y direction
            
        Returns:
            Translation transformation
        """
        return cls(tx=dx, ty=dy)
    
    @classmethod
    def scale_transform(cls, sx: float, sy: Optional[float] = None) -> "Transform2D":
        """Create a scaling transformation.
        
        Args:
            sx: Scale factor in X direction
            sy: Scale factor in Y direction (defaults to sx for uniform scaling)
            
        Returns:
            Scaling transformation
        """
        if sy is None:
            sy = sx
        return cls(a=sx, d=sy)
    
    @classmethod
    def rotate(cls, angle: float) -> "Transform2D":
        """Create a rotation transformation.
        
        Args:
            angle: Rotation angle in radians
            
        Returns:
            Rotation transformation
        """
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)
        return cls(a=cos_a, b=sin_a, c=-sin_a, d=cos_a)
    
    @classmethod
    def skew(cls, skew_x: float, skew_y: float = 0.0) -> "Transform2D":
        """Create a skew transformation.
        
        Args:
            skew_x: Skew angle in X direction (radians)
            skew_y: Skew angle in Y direction (radians)
            
        Returns:
            Skew transformation
        """
        return cls(b=math.tan(skew_y), c=math.tan(skew_x))
    
    def translate_by(self, dx: float, dy: float) -> "Transform2D":
        """Apply translation to this transformation.
        
        This adds the translation directly to the existing translation values.
        
        Args:
            dx: Translation in X direction
            dy: Translation in Y direction
            
        Returns:
            New transformation with translation applied
        """
        return Transform2D(
            a=self.a, b=self.b, c=self.c, d=self.d,
            tx=self.tx + dx, ty=self.ty + dy
        )
    
    def scale(self, sx: float, sy: Optional[float] = None) -> "Transform2D":
        """Apply scaling to this transformation.
        
        Args:
            sx: Scale factor in X direction
            sy: Scale factor in Y direction (defaults to sx for uniform scaling)
            
        Returns:
            New transformation with scaling applied
        """
        return self * Transform2D.scale_transform(sx, sy)
    
    def scale_by(self, sx: float, sy: Optional[float] = None) -> "Transform2D":
        """Apply scaling to this transformation.
        
        Args:
            sx: Scale factor in X direction
            sy: Scale factor in Y direction (defaults to sx for uniform scaling)
            
        Returns:
            New transformation with scaling applied
        """
        return self * Transform2D.scale_transform(sx, sy)
    
    def rotate_by(self, angle: float) -> "Transform2D":
        """Apply rotation to this transformation.
        
        Args:
            angle: Rotation angle in radians
            
        Returns:
            New transformation with rotation applied
        """
        return self * Transform2D.rotate(angle)
    
    def skew_by(self, skew_x: float, skew_y: float = 0.0) -> "Transform2D":
        """Apply skew to this transformation.
        
        This adds the skew directly to the existing matrix values.
        
        Args:
            skew_x: Skew angle in X direction (radians)
            skew_y: Skew angle in Y direction (radians)
            
        Returns:
            New transformation with skew applied
        """
        return Transform2D(
            a=self.a, b=self.b + math.tan(skew_y), 
            c=self.c + math.tan(skew_x), d=self.d,
            tx=self.tx, ty=self.ty
        )