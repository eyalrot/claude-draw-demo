"""Tests for Point2D model."""

import math
import pytest
from pydantic import ValidationError

from claude_draw.models.point import Point2D


class TestPoint2D:
    """Test Point2D model."""
    
    def test_creation(self):
        """Test creating a Point2D."""
        p = Point2D(x=3.0, y=4.0)
        assert p.x == 3.0
        assert p.y == 4.0
    
    def test_integer_coordinates(self):
        """Test that integer coordinates are accepted."""
        p = Point2D(x=3, y=4)
        assert p.x == 3.0
        assert p.y == 4.0
    
    def test_negative_coordinates(self):
        """Test that negative coordinates are allowed."""
        p = Point2D(x=-3.5, y=-4.5)
        assert p.x == -3.5
        assert p.y == -4.5
    
    def test_zero_coordinates(self):
        """Test that zero coordinates are allowed."""
        p = Point2D(x=0, y=0)
        assert p.x == 0.0
        assert p.y == 0.0
    
    def test_invalid_coordinates(self):
        """Test that non-finite coordinates are rejected."""
        with pytest.raises(ValidationError) as exc_info:
            Point2D(x=float('inf'), y=0)
        assert "must be a finite number" in str(exc_info.value)
        
        with pytest.raises(ValidationError) as exc_info:
            Point2D(x=0, y=float('-inf'))
        assert "must be a finite number" in str(exc_info.value)
        
        with pytest.raises(ValidationError) as exc_info:
            Point2D(x=float('nan'), y=0)
        assert "must be a finite number" in str(exc_info.value)
    
    def test_origin_factory(self):
        """Test origin factory method."""
        p = Point2D.origin()
        assert p.x == 0.0
        assert p.y == 0.0
    
    def test_string_representation(self):
        """Test string representations."""
        p = Point2D(x=3.0, y=4.0)
        assert str(p) == "Point2D(3.0, 4.0)"
        assert repr(p) == "Point2D(x=3.0, y=4.0)"
    
    def test_equality(self):
        """Test point equality."""
        p1 = Point2D(x=3.0, y=4.0)
        p2 = Point2D(x=3.0, y=4.0)
        p3 = Point2D(x=3.0, y=5.0)
        
        assert p1 == p2
        assert p1 != p3
        assert p1 != "not a point"
    
    def test_hash(self):
        """Test point hashing."""
        p1 = Point2D(x=3.0, y=4.0)
        p2 = Point2D(x=3.0, y=4.0)
        p3 = Point2D(x=3.0, y=5.0)
        
        assert hash(p1) == hash(p2)
        assert hash(p1) != hash(p3)
        
        # Test that points can be used in sets
        point_set = {p1, p2, p3}
        assert len(point_set) == 2
    
    def test_addition(self):
        """Test point addition."""
        p1 = Point2D(x=3.0, y=4.0)
        p2 = Point2D(x=1.0, y=2.0)
        result = p1 + p2
        
        assert result.x == 4.0
        assert result.y == 6.0
    
    def test_subtraction(self):
        """Test point subtraction."""
        p1 = Point2D(x=3.0, y=4.0)
        p2 = Point2D(x=1.0, y=2.0)
        result = p1 - p2
        
        assert result.x == 2.0
        assert result.y == 2.0
    
    def test_scalar_multiplication(self):
        """Test scalar multiplication."""
        p = Point2D(x=3.0, y=4.0)
        
        # Left multiplication
        result1 = p * 2.0
        assert result1.x == 6.0
        assert result1.y == 8.0
        
        # Right multiplication
        result2 = 2.0 * p
        assert result2.x == 6.0
        assert result2.y == 8.0
        
        # Multiplication by zero
        result3 = p * 0
        assert result3.x == 0.0
        assert result3.y == 0.0
        
        # Multiplication by negative
        result4 = p * -1
        assert result4.x == -3.0
        assert result4.y == -4.0
    
    def test_scalar_division(self):
        """Test scalar division."""
        p = Point2D(x=6.0, y=8.0)
        
        result = p / 2.0
        assert result.x == 3.0
        assert result.y == 4.0
        
        # Division by zero should raise
        with pytest.raises(ZeroDivisionError):
            p / 0
    
    def test_negation(self):
        """Test point negation."""
        p = Point2D(x=3.0, y=-4.0)
        result = -p
        
        assert result.x == -3.0
        assert result.y == 4.0
    
    def test_as_tuple(self):
        """Test converting to tuple."""
        p = Point2D(x=3.0, y=4.0)
        t = p.as_tuple()
        
        assert t == (3.0, 4.0)
        assert isinstance(t, tuple)
    
    def test_serialization(self):
        """Test JSON serialization."""
        p = Point2D(x=3.0, y=4.0)
        
        # To dict
        data = p.to_dict()
        assert data == {"x": 3.0, "y": 4.0}
        
        # From dict
        p2 = Point2D.from_dict(data)
        assert p2 == p
        
        # To JSON
        json_str = p.to_json()
        assert '"x":3.0' in json_str
        assert '"y":4.0' in json_str
        
        # From JSON
        p3 = Point2D.from_json(json_str)
        assert p3 == p
    
    def test_distance_to(self):
        """Test distance calculations."""
        p1 = Point2D(x=0, y=0)
        p2 = Point2D(x=3, y=4)
        
        assert p1.distance_to(p2) == 5.0
        assert p2.distance_to(p1) == 5.0
        
        # Distance to self is zero
        assert p1.distance_to(p1) == 0.0
    
    def test_manhattan_distance(self):
        """Test Manhattan distance calculation."""
        p1 = Point2D(x=0, y=0)
        p2 = Point2D(x=3, y=4)
        
        assert p1.manhattan_distance_to(p2) == 7.0
        assert p2.manhattan_distance_to(p1) == 7.0
        
        # Test with negative coordinates
        p3 = Point2D(x=-2, y=-3)
        assert p1.manhattan_distance_to(p3) == 5.0
    
    def test_magnitude(self):
        """Test magnitude calculation."""
        assert Point2D(x=3, y=4).magnitude() == 5.0
        assert Point2D(x=0, y=0).magnitude() == 0.0
        assert Point2D(x=1, y=0).magnitude() == 1.0
    
    def test_normalize(self):
        """Test normalization."""
        p = Point2D(x=3, y=4)
        normalized = p.normalize()
        
        assert abs(normalized.x - 0.6) < 1e-10
        assert abs(normalized.y - 0.8) < 1e-10
        assert abs(normalized.magnitude() - 1.0) < 1e-10
        
        # Zero vector cannot be normalized
        with pytest.raises(ZeroDivisionError):
            Point2D(x=0, y=0).normalize()
    
    def test_dot_product(self):
        """Test dot product."""
        p1 = Point2D(x=3, y=4)
        p2 = Point2D(x=2, y=1)
        
        assert p1.dot(p2) == 10.0
        assert p2.dot(p1) == 10.0
        
        # Perpendicular vectors
        p3 = Point2D(x=1, y=0)
        p4 = Point2D(x=0, y=1)
        assert p3.dot(p4) == 0.0
    
    def test_cross_product(self):
        """Test 2D cross product."""
        p1 = Point2D(x=3, y=4)
        p2 = Point2D(x=2, y=1)
        
        assert p1.cross(p2) == -5.0
        assert p2.cross(p1) == 5.0
        
        # Parallel vectors
        p3 = Point2D(x=2, y=4)
        p4 = Point2D(x=1, y=2)
        assert p3.cross(p4) == 0.0
    
    def test_angle_to(self):
        """Test angle calculation."""
        p1 = Point2D(x=1, y=0)
        p2 = Point2D(x=0, y=1)
        
        # 90 degrees
        angle = p1.angle_to(p2)
        assert abs(angle - math.pi/2) < 1e-10
        
        # 0 degrees (same direction)
        p3 = Point2D(x=2, y=0)
        assert abs(p1.angle_to(p3)) < 1e-10
        
        # 180 degrees (opposite direction)
        p4 = Point2D(x=-1, y=0)
        assert abs(p1.angle_to(p4) - math.pi) < 1e-10
    
    def test_rotate(self):
        """Test rotation."""
        p = Point2D(x=1, y=0)
        
        # Rotate 90 degrees counterclockwise
        rotated = p.rotate(math.pi / 2)
        assert abs(rotated.x) < 1e-10
        assert abs(rotated.y - 1) < 1e-10
        
        # Rotate around different center
        center = Point2D(x=1, y=1)
        rotated2 = p.rotate(math.pi / 2, center)
        assert abs(rotated2.x - 2) < 1e-10
        assert abs(rotated2.y - 1) < 1e-10
    
    def test_translate(self):
        """Test translation."""
        p = Point2D(x=1, y=2)
        translated = p.translate(3, 4)
        
        assert translated.x == 4.0
        assert translated.y == 6.0
    
    def test_midpoint(self):
        """Test midpoint calculation."""
        p1 = Point2D(x=0, y=0)
        p2 = Point2D(x=4, y=6)
        
        mid = p1.midpoint(p2)
        assert mid.x == 2.0
        assert mid.y == 3.0
    
    def test_lerp(self):
        """Test linear interpolation."""
        p1 = Point2D(x=0, y=0)
        p2 = Point2D(x=10, y=20)
        
        # t=0 returns p1
        assert p1.lerp(p2, 0) == p1
        
        # t=1 returns p2
        assert p1.lerp(p2, 1) == p2
        
        # t=0.5 returns midpoint
        mid = p1.lerp(p2, 0.5)
        assert mid.x == 5.0
        assert mid.y == 10.0
        
        # t=0.25
        quarter = p1.lerp(p2, 0.25)
        assert quarter.x == 2.5
        assert quarter.y == 5.0
    
    def test_reflect(self):
        """Test reflection across a line."""
        # Point to reflect
        p = Point2D(x=2, y=3)
        
        # Reflect across x-axis (y=0)
        line_point = Point2D(x=0, y=0)
        line_direction = Point2D(x=1, y=0)
        reflected = p.reflect(line_point, line_direction)
        
        assert reflected.x == 2.0
        assert reflected.y == -3.0
        
        # Reflect across y-axis (x=0)
        line_direction2 = Point2D(x=0, y=1)
        reflected2 = p.reflect(line_point, line_direction2)
        
        assert reflected2.x == -2.0
        assert reflected2.y == 3.0