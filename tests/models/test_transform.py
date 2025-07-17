"""Tests for Transform2D model."""

import math
import pytest
from pydantic import ValidationError

from claude_draw.models.transform import Transform2D
from claude_draw.models.point import Point2D


class TestTransform2D:
    """Test Transform2D model."""
    
    def test_identity_creation(self):
        """Test creating identity transform."""
        transform = Transform2D.identity()
        assert transform.a == 1.0
        assert transform.b == 0.0
        assert transform.c == 0.0
        assert transform.d == 1.0
        assert transform.tx == 0.0
        assert transform.ty == 0.0
        assert transform.is_identity()
    
    def test_custom_creation(self):
        """Test creating custom transform."""
        transform = Transform2D(a=2.0, b=0.5, c=1.0, d=1.5, tx=10.0, ty=20.0)
        assert transform.a == 2.0
        assert transform.b == 0.5
        assert transform.c == 1.0
        assert transform.d == 1.5
        assert transform.tx == 10.0
        assert transform.ty == 20.0
    
    def test_invalid_values(self):
        """Test that invalid values are rejected."""
        with pytest.raises(ValidationError):
            Transform2D(a=float('inf'))
        
        with pytest.raises(ValidationError):
            Transform2D(b=float('nan'))
    
    def test_from_matrix(self):
        """Test creating transform from matrix."""
        matrix = [
            [2.0, 1.0, 10.0],
            [0.5, 1.5, 20.0],
            [0.0, 0.0, 1.0]
        ]
        transform = Transform2D.from_matrix(matrix)
        assert transform.a == 2.0
        assert transform.b == 0.5
        assert transform.c == 1.0
        assert transform.d == 1.5
        assert transform.tx == 10.0
        assert transform.ty == 20.0
    
    def test_from_invalid_matrix(self):
        """Test creating transform from invalid matrix."""
        # Wrong size
        with pytest.raises(ValueError, match="Matrix must be 3x3"):
            Transform2D.from_matrix([[1, 2], [3, 4]])
        
        # Wrong bottom row
        with pytest.raises(ValueError, match="Bottom row must be"):
            Transform2D.from_matrix([
                [1, 0, 0],
                [0, 1, 0],
                [1, 0, 1]  # Should be [0, 0, 1]
            ])
    
    def test_to_matrix(self):
        """Test converting to matrix."""
        transform = Transform2D(a=2.0, b=0.5, c=1.0, d=1.5, tx=10.0, ty=20.0)
        matrix = transform.to_matrix()
        expected = [
            [2.0, 1.0, 10.0],
            [0.5, 1.5, 20.0],
            [0.0, 0.0, 1.0]
        ]
        assert matrix == expected
    
    def test_determinant(self):
        """Test determinant calculation."""
        # Identity transform
        identity = Transform2D.identity()
        assert identity.determinant() == 1.0
        
        # Custom transform
        transform = Transform2D(a=2.0, b=0.0, c=0.0, d=3.0)
        assert transform.determinant() == 6.0
        
        # Non-invertible transform
        singular = Transform2D(a=1.0, b=2.0, c=2.0, d=4.0)
        assert abs(singular.determinant()) < 1e-10
    
    def test_is_invertible(self):
        """Test invertibility check."""
        # Identity is invertible
        identity = Transform2D.identity()
        assert identity.is_invertible()
        
        # Regular transform is invertible
        transform = Transform2D(a=2.0, b=0.0, c=0.0, d=3.0)
        assert transform.is_invertible()
        
        # Singular transform is not invertible
        singular = Transform2D(a=1.0, b=2.0, c=2.0, d=4.0)
        assert not singular.is_invertible()
    
    def test_inverse(self):
        """Test inverse calculation."""
        # Identity inverse is identity
        identity = Transform2D.identity()
        inverse = identity.inverse()
        assert inverse.is_identity()
        
        # Test with translation
        translate = Transform2D(tx=10.0, ty=20.0)
        inverse = translate.inverse()
        assert abs(inverse.tx - (-10.0)) < 1e-10
        assert abs(inverse.ty - (-20.0)) < 1e-10
        
        # Test with scaling
        scale = Transform2D(a=2.0, d=3.0)
        inverse = scale.inverse()
        assert abs(inverse.a - 0.5) < 1e-10
        assert abs(inverse.d - 1.0/3.0) < 1e-10
        
        # Test non-invertible transform
        singular = Transform2D(a=1.0, b=2.0, c=2.0, d=4.0)
        with pytest.raises(ValueError, match="not invertible"):
            singular.inverse()
    
    def test_multiplication(self):
        """Test matrix multiplication."""
        # Identity * transform = transform
        identity = Transform2D.identity()
        transform = Transform2D(a=2.0, tx=10.0)
        result = identity * transform
        assert result == transform
        
        # Transform * identity = transform
        result = transform * identity
        assert result == transform
        
        # Two translations
        t1 = Transform2D(tx=10.0, ty=20.0)
        t2 = Transform2D(tx=5.0, ty=15.0)
        result = t1 * t2
        assert result.tx == 15.0
        assert result.ty == 35.0
        
        # Scaling and translation
        scale = Transform2D(a=2.0, d=3.0)
        translate = Transform2D(tx=10.0, ty=20.0)
        result = scale * translate
        assert result.tx == 20.0  # 2 * 10
        assert result.ty == 60.0  # 3 * 20
    
    def test_transform_point(self):
        """Test point transformation."""
        # Identity transform
        identity = Transform2D.identity()
        point = Point2D(x=10.0, y=20.0)
        result = identity.transform_point(point)
        assert result == point
        
        # Translation
        translate = Transform2D(tx=5.0, ty=15.0)
        result = translate.transform_point(point)
        assert result.x == 15.0
        assert result.y == 35.0
        
        # Scaling
        scale = Transform2D(a=2.0, d=3.0)
        result = scale.transform_point(point)
        assert result.x == 20.0
        assert result.y == 60.0
    
    def test_transform_vector(self):
        """Test vector transformation (no translation)."""
        # Translation should not affect vectors
        translate = Transform2D(tx=5.0, ty=15.0)
        vector = Point2D(x=10.0, y=20.0)
        result = translate.transform_vector(vector)
        assert result == vector
        
        # Scaling should affect vectors
        scale = Transform2D(a=2.0, d=3.0)
        result = scale.transform_vector(vector)
        assert result.x == 20.0
        assert result.y == 60.0
    
    def test_equality(self):
        """Test transform equality."""
        t1 = Transform2D(a=2.0, b=0.5, c=1.0, d=1.5, tx=10.0, ty=20.0)
        t2 = Transform2D(a=2.0, b=0.5, c=1.0, d=1.5, tx=10.0, ty=20.0)
        t3 = Transform2D(a=2.0, b=0.5, c=1.0, d=1.5, tx=10.0, ty=21.0)
        
        assert t1 == t2
        assert t1 != t3
        assert t1 != "not a transform"
        
        # Test with small differences (within epsilon)
        t4 = Transform2D(a=2.0 + 1e-11, b=0.5, c=1.0, d=1.5, tx=10.0, ty=20.0)
        assert t1 == t4
    
    def test_hash(self):
        """Test transform hashing."""
        t1 = Transform2D(a=2.0, b=0.5, c=1.0, d=1.5, tx=10.0, ty=20.0)
        t2 = Transform2D(a=2.0, b=0.5, c=1.0, d=1.5, tx=10.0, ty=20.0)
        t3 = Transform2D(a=2.0, b=0.5, c=1.0, d=1.5, tx=10.0, ty=21.0)
        
        assert hash(t1) == hash(t2)
        assert hash(t1) != hash(t3)
        
        # Can be used in sets
        transform_set = {t1, t2, t3}
        assert len(transform_set) == 2
    
    def test_string_representation(self):
        """Test string representations."""
        transform = Transform2D(a=2.0, b=0.5, c=1.0, d=1.5, tx=10.0, ty=20.0)
        str_repr = str(transform)
        assert "Transform2D" in str_repr
        assert "2.000" in str_repr
        
        repr_str = repr(transform)
        assert "Transform2D(a=2.0" in repr_str
    
    def test_is_identity(self):
        """Test identity check."""
        identity = Transform2D.identity()
        assert identity.is_identity()
        
        not_identity = Transform2D(a=2.0)
        assert not not_identity.is_identity()
    
    def test_is_translation(self):
        """Test translation check."""
        translation = Transform2D(tx=10.0, ty=20.0)
        assert translation.is_translation()
        
        not_translation = Transform2D(a=2.0, tx=10.0)
        assert not not_translation.is_translation()
        
        identity = Transform2D.identity()
        assert not identity.is_translation()
    
    def test_is_scaling(self):
        """Test scaling check."""
        scaling = Transform2D(a=2.0, d=3.0)
        assert scaling.is_scaling()
        
        not_scaling = Transform2D(a=2.0, tx=10.0)
        assert not not_scaling.is_scaling()
        
        identity = Transform2D.identity()
        assert not identity.is_scaling()
    
    def test_is_rotation(self):
        """Test rotation check."""
        # 90 degree rotation
        angle = math.pi / 2
        rotation = Transform2D(a=math.cos(angle), b=math.sin(angle), 
                             c=-math.sin(angle), d=math.cos(angle))
        assert rotation.is_rotation()
        
        not_rotation = Transform2D(a=2.0, d=3.0)
        assert not not_rotation.is_rotation()
        
        identity = Transform2D.identity()
        assert not identity.is_rotation()  # Identity is not considered a rotation
    
    def test_get_scale_factors(self):
        """Test scale factor extraction."""
        # Pure scaling
        scale = Transform2D(a=2.0, d=3.0)
        sx, sy = scale.get_scale_factors()
        assert abs(sx - 2.0) < 1e-10
        assert abs(sy - 3.0) < 1e-10
        
        # Identity
        identity = Transform2D.identity()
        sx, sy = identity.get_scale_factors()
        assert abs(sx - 1.0) < 1e-10
        assert abs(sy - 1.0) < 1e-10
    
    def test_get_rotation_angle(self):
        """Test rotation angle extraction."""
        # 90 degree rotation
        angle = math.pi / 2
        rotation = Transform2D(a=math.cos(angle), b=math.sin(angle), 
                             c=-math.sin(angle), d=math.cos(angle))
        extracted_angle = rotation.get_rotation_angle()
        assert abs(extracted_angle - angle) < 1e-10
        
        # Identity (no rotation)
        identity = Transform2D.identity()
        assert abs(identity.get_rotation_angle()) < 1e-10
    
    def test_get_translation(self):
        """Test translation extraction."""
        translation = Transform2D(tx=10.0, ty=20.0)
        t = translation.get_translation()
        assert t.x == 10.0
        assert t.y == 20.0
        
        # Identity (no translation)
        identity = Transform2D.identity()
        t = identity.get_translation()
        assert t.x == 0.0
        assert t.y == 0.0
    
    def test_serialization(self):
        """Test JSON serialization."""
        transform = Transform2D(a=2.0, b=0.5, c=1.0, d=1.5, tx=10.0, ty=20.0)
        
        # To dict
        data = transform.to_dict()
        expected = {"a": 2.0, "b": 0.5, "c": 1.0, "d": 1.5, "tx": 10.0, "ty": 20.0}
        assert data == expected
        
        # From dict
        transform2 = Transform2D.from_dict(data)
        assert transform2 == transform
        
        # To JSON
        json_str = transform.to_json()
        assert '"a":2.0' in json_str
        
        # From JSON
        transform3 = Transform2D.from_json(json_str)
        assert transform3 == transform
    
    def test_translate_factory(self):
        """Test translate factory method."""
        translation = Transform2D.translate(10.0, 20.0)
        assert translation.tx == 10.0
        assert translation.ty == 20.0
        assert translation.a == 1.0
        assert translation.d == 1.0
        assert translation.b == 0.0
        assert translation.c == 0.0
        assert translation.is_translation()
    
    def test_scale_factory(self):
        """Test scale factory method."""
        # Uniform scaling
        scale = Transform2D.scale(2.0)
        assert scale.a == 2.0
        assert scale.d == 2.0
        assert scale.tx == 0.0
        assert scale.ty == 0.0
        assert scale.is_scaling()
        
        # Non-uniform scaling
        scale2 = Transform2D.scale(2.0, 3.0)
        assert scale2.a == 2.0
        assert scale2.d == 3.0
        assert scale2.is_scaling()
    
    def test_rotate_factory(self):
        """Test rotate factory method."""
        # 90 degree rotation
        angle = math.pi / 2
        rotation = Transform2D.rotate(angle)
        assert abs(rotation.a - math.cos(angle)) < 1e-10
        assert abs(rotation.b - math.sin(angle)) < 1e-10
        assert abs(rotation.c - (-math.sin(angle))) < 1e-10
        assert abs(rotation.d - math.cos(angle)) < 1e-10
        assert rotation.tx == 0.0
        assert rotation.ty == 0.0
        assert rotation.is_rotation()
        
        # 180 degree rotation
        angle2 = math.pi
        rotation2 = Transform2D.rotate(angle2)
        assert abs(rotation2.a - (-1.0)) < 1e-10
        assert abs(rotation2.b) < 1e-10
        assert abs(rotation2.c) < 1e-10
        assert abs(rotation2.d - (-1.0)) < 1e-10
    
    def test_skew_factory(self):
        """Test skew factory method."""
        # X skew only
        skew = Transform2D.skew(math.pi / 4)  # 45 degrees
        assert skew.a == 1.0
        assert skew.b == 0.0
        assert abs(skew.c - math.tan(math.pi / 4)) < 1e-10
        assert skew.d == 1.0
        assert skew.tx == 0.0
        assert skew.ty == 0.0
        
        # Both X and Y skew
        skew2 = Transform2D.skew(math.pi / 6, math.pi / 4)  # 30 and 45 degrees
        assert abs(skew2.b - math.tan(math.pi / 4)) < 1e-10
        assert abs(skew2.c - math.tan(math.pi / 6)) < 1e-10
    
    def test_translate_by(self):
        """Test translate_by method."""
        # Start with identity
        transform = Transform2D.identity()
        translated = transform.translate_by(10.0, 20.0)
        assert translated.tx == 10.0
        assert translated.ty == 20.0
        
        # Cumulative translation
        translated2 = translated.translate_by(5.0, 15.0)
        assert translated2.tx == 15.0
        assert translated2.ty == 35.0
        
        # With existing transform
        existing = Transform2D(a=2.0, d=3.0)
        translated3 = existing.translate_by(10.0, 20.0)
        assert translated3.a == 2.0
        assert translated3.d == 3.0
        assert translated3.tx == 10.0
        assert translated3.ty == 20.0
    
    def test_scale_by(self):
        """Test scale_by method."""
        # Start with identity
        transform = Transform2D.identity()
        scaled = transform.scale_by(2.0)
        assert scaled.a == 2.0
        assert scaled.d == 2.0
        
        # Non-uniform scaling
        scaled2 = transform.scale_by(2.0, 3.0)
        assert scaled2.a == 2.0
        assert scaled2.d == 3.0
        
        # Cumulative scaling
        scaled3 = scaled.scale_by(1.5)
        assert scaled3.a == 3.0
        assert scaled3.d == 3.0
        
        # With existing transform
        existing = Transform2D(tx=10.0, ty=20.0)
        scaled4 = existing.scale_by(2.0)
        assert scaled4.a == 2.0
        assert scaled4.d == 2.0
        assert scaled4.tx == 10.0
        assert scaled4.ty == 20.0
    
    def test_rotate_by(self):
        """Test rotate_by method."""
        # Start with identity
        transform = Transform2D.identity()
        rotated = transform.rotate_by(math.pi / 2)
        assert rotated.is_rotation()
        
        # Cumulative rotation
        rotated2 = rotated.rotate_by(math.pi / 2)
        # Should be 180 degrees total
        assert abs(rotated2.a - (-1.0)) < 1e-10
        assert abs(rotated2.d - (-1.0)) < 1e-10
        
        # With existing transform
        existing = Transform2D(tx=10.0, ty=20.0)
        rotated3 = existing.rotate_by(math.pi / 2)
        # Translation should remain
        assert rotated3.tx == 10.0
        assert rotated3.ty == 20.0
        # But rotation should be applied
        assert abs(rotated3.a) < 1e-10
        assert abs(rotated3.b - 1.0) < 1e-10
        assert abs(rotated3.c - (-1.0)) < 1e-10
        assert abs(rotated3.d) < 1e-10
    
    def test_skew_by(self):
        """Test skew_by method."""
        # Start with identity
        transform = Transform2D.identity()
        skewed = transform.skew_by(math.pi / 4)
        assert abs(skewed.c - math.tan(math.pi / 4)) < 1e-10
        
        # With existing transform
        existing = Transform2D(a=2.0, d=3.0)
        skewed2 = existing.skew_by(math.pi / 6, math.pi / 4)
        # Original scaling should remain
        assert skewed2.a == 2.0
        assert skewed2.d == 3.0
        # And skew should be applied
        assert abs(skewed2.b - math.tan(math.pi / 4)) < 1e-10
        assert abs(skewed2.c - math.tan(math.pi / 6)) < 1e-10
    
    def test_transformation_composition(self):
        """Test combining multiple transformations."""
        # Create a complex transformation: translate, scale, rotate
        transform = (Transform2D.identity()
                    .translate_by(10.0, 20.0)
                    .scale_by(2.0, 3.0)
                    .rotate_by(math.pi / 4))
        
        # Test that the transformation can be applied to a point
        point = Point2D(x=1.0, y=1.0)
        transformed = transform.transform_point(point)
        
        # The exact values would be complex to calculate, but we can verify
        # that the transformation produces a valid point
        assert isinstance(transformed, Point2D)
        assert math.isfinite(transformed.x)
        assert math.isfinite(transformed.y)
        
        # Test that the transformation is invertible
        assert transform.is_invertible()
        inverse = transform.inverse()
        
        # Applying transform then its inverse should return original point
        round_trip = inverse.transform_point(transformed)
        assert abs(round_trip.x - point.x) < 1e-10
        assert abs(round_trip.y - point.y) < 1e-10