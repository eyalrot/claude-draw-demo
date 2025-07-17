"""Tests for validation utilities."""

import math
import pytest

from claude_draw.models.validators import (
    validate_finite_number,
    validate_color_channel,
    validate_alpha_channel,
    validate_angle_radians,
    validate_angle_degrees,
    validate_non_negative,
    validate_positive,
    validate_hex_color,
    clamp,
)


class TestValidateFiniteNumber:
    """Test validate_finite_number function."""
    
    def test_valid_numbers(self):
        """Test validation of valid finite numbers."""
        assert validate_finite_number(0.0) == 0.0
        assert validate_finite_number(42.5) == 42.5
        assert validate_finite_number(-100.0) == -100.0
    
    def test_invalid_numbers(self):
        """Test validation of invalid numbers."""
        with pytest.raises(ValueError, match="must be a finite number"):
            validate_finite_number(float('inf'))
        
        with pytest.raises(ValueError, match="must be a finite number"):
            validate_finite_number(float('-inf'))
        
        with pytest.raises(ValueError, match="must be a finite number"):
            validate_finite_number(float('nan'))


class TestValidateColorChannel:
    """Test validate_color_channel function."""
    
    def test_valid_values(self):
        """Test validation of valid color channel values."""
        assert validate_color_channel(0) == 0
        assert validate_color_channel(128) == 128
        assert validate_color_channel(255) == 255
    
    def test_invalid_values(self):
        """Test validation of invalid color channel values."""
        with pytest.raises(ValueError, match="must be an integer"):
            validate_color_channel(42.5)  # type: ignore
        
        with pytest.raises(ValueError, match="must be in range 0-255"):
            validate_color_channel(-1)
        
        with pytest.raises(ValueError, match="must be in range 0-255"):
            validate_color_channel(256)


class TestValidateAlphaChannel:
    """Test validate_alpha_channel function."""
    
    def test_valid_values(self):
        """Test validation of valid alpha values."""
        assert validate_alpha_channel(0.0) == 0.0
        assert validate_alpha_channel(0.5) == 0.5
        assert validate_alpha_channel(1.0) == 1.0
    
    def test_invalid_values(self):
        """Test validation of invalid alpha values."""
        with pytest.raises(ValueError, match="must be in range 0.0-1.0"):
            validate_alpha_channel(-0.1)
        
        with pytest.raises(ValueError, match="must be in range 0.0-1.0"):
            validate_alpha_channel(1.1)


class TestValidateAngleRadians:
    """Test validate_angle_radians function."""
    
    def test_normalization(self):
        """Test angle normalization to [0, 2π)."""
        assert validate_angle_radians(0) == 0
        assert validate_angle_radians(math.pi) == math.pi
        assert validate_angle_radians(2 * math.pi) == 0
        assert validate_angle_radians(3 * math.pi) == math.pi
        assert validate_angle_radians(-math.pi) == math.pi
    
    def test_invalid_angles(self):
        """Test validation of invalid angles."""
        with pytest.raises(ValueError, match="must be a finite number"):
            validate_angle_radians(float('inf'))


class TestValidateAngleDegrees:
    """Test validate_angle_degrees function."""
    
    def test_normalization(self):
        """Test angle normalization to [0, 360)."""
        assert validate_angle_degrees(0) == 0
        assert validate_angle_degrees(180) == 180
        assert validate_angle_degrees(360) == 0
        assert validate_angle_degrees(540) == 180
        assert validate_angle_degrees(-90) == 270
    
    def test_invalid_angles(self):
        """Test validation of invalid angles."""
        with pytest.raises(ValueError, match="must be a finite number"):
            validate_angle_degrees(float('inf'))


class TestValidateNonNegative:
    """Test validate_non_negative function."""
    
    def test_valid_values(self):
        """Test validation of valid non-negative values."""
        assert validate_non_negative(0) == 0
        assert validate_non_negative(42.5) == 42.5
    
    def test_invalid_values(self):
        """Test validation of negative values."""
        with pytest.raises(ValueError, match="must be non-negative"):
            validate_non_negative(-1)


class TestValidatePositive:
    """Test validate_positive function."""
    
    def test_valid_values(self):
        """Test validation of valid positive values."""
        assert validate_positive(1) == 1
        assert validate_positive(42.5) == 42.5
    
    def test_invalid_values(self):
        """Test validation of non-positive values."""
        with pytest.raises(ValueError, match="must be positive"):
            validate_positive(0)
        
        with pytest.raises(ValueError, match="must be positive"):
            validate_positive(-1)


class TestValidateHexColor:
    """Test validate_hex_color function."""
    
    def test_valid_colors(self):
        """Test validation of valid hex colors."""
        assert validate_hex_color("#FF0000") == "FF0000"
        assert validate_hex_color("FF0000") == "FF0000"
        assert validate_hex_color("#F00") == "FF0000"
        assert validate_hex_color("abc") == "AABBCC"
    
    def test_case_normalization(self):
        """Test that hex colors are normalized to uppercase."""
        assert validate_hex_color("ff0000") == "FF0000"
        assert validate_hex_color("#aAbBcC") == "AABBCC"
    
    def test_invalid_colors(self):
        """Test validation of invalid hex colors."""
        with pytest.raises(ValueError, match="must be a string"):
            validate_hex_color(123456)  # type: ignore
        
        with pytest.raises(ValueError, match="must be 3 or 6 characters"):
            validate_hex_color("FF")
        
        with pytest.raises(ValueError, match="must be 3 or 6 characters"):
            validate_hex_color("FF00000")
        
        with pytest.raises(ValueError, match="Invalid hex color"):
            validate_hex_color("GGGGGG")


class TestClamp:
    """Test clamp function."""
    
    def test_clamping(self):
        """Test value clamping."""
        assert clamp(5, 0, 10) == 5
        assert clamp(-5, 0, 10) == 0
        assert clamp(15, 0, 10) == 10
        assert clamp(0.5, 0.0, 1.0) == 0.5