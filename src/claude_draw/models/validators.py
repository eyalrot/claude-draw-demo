"""Common validators and validation utilities for Claude Draw models."""

import math
from typing import Union, Tuple, Optional
from pydantic import field_validator


def validate_finite_number(value: float, field_name: str = "value") -> float:
    """Validate that a number is finite (not NaN or infinity).
    
    Args:
        value: Number to validate
        field_name: Name of the field for error messages
        
    Returns:
        The validated number
        
    Raises:
        ValueError: If the number is not finite
    """
    if not math.isfinite(value):
        raise ValueError(f"{field_name} must be a finite number, got {value}")
    return value


def validate_color_channel(value: int, channel_name: str = "channel") -> int:
    """Validate that a color channel value is in the range 0-255.
    
    Args:
        value: Channel value to validate
        channel_name: Name of the channel for error messages
        
    Returns:
        The validated channel value
        
    Raises:
        ValueError: If the value is not in range 0-255
    """
    if not isinstance(value, int):
        raise ValueError(f"{channel_name} must be an integer, got {type(value).__name__}")
    if not 0 <= value <= 255:
        raise ValueError(f"{channel_name} must be in range 0-255, got {value}")
    return value


def validate_alpha_channel(value: float) -> float:
    """Validate that an alpha channel value is in the range 0.0-1.0.
    
    Args:
        value: Alpha value to validate
        
    Returns:
        The validated alpha value
        
    Raises:
        ValueError: If the value is not in range 0.0-1.0
    """
    if not 0.0 <= value <= 1.0:
        raise ValueError(f"Alpha must be in range 0.0-1.0, got {value}")
    return value


def validate_angle_radians(value: float) -> float:
    """Validate and normalize an angle in radians to the range [0, 2π).
    
    Args:
        value: Angle in radians
        
    Returns:
        Normalized angle in range [0, 2π)
    """
    value = validate_finite_number(value, "angle")
    # Normalize to [0, 2π)
    return value % (2 * math.pi)


def validate_angle_degrees(value: float) -> float:
    """Validate and normalize an angle in degrees to the range [0, 360).
    
    Args:
        value: Angle in degrees
        
    Returns:
        Normalized angle in range [0, 360)
    """
    value = validate_finite_number(value, "angle")
    # Normalize to [0, 360)
    return value % 360


def validate_non_negative(value: float, field_name: str = "value") -> float:
    """Validate that a number is non-negative.
    
    Args:
        value: Number to validate
        field_name: Name of the field for error messages
        
    Returns:
        The validated number
        
    Raises:
        ValueError: If the number is negative
    """
    if value < 0:
        raise ValueError(f"{field_name} must be non-negative, got {value}")
    return value


def validate_positive(value: float, field_name: str = "value") -> float:
    """Validate that a number is positive.
    
    Args:
        value: Number to validate
        field_name: Name of the field for error messages
        
    Returns:
        The validated number
        
    Raises:
        ValueError: If the number is not positive
    """
    if value <= 0:
        raise ValueError(f"{field_name} must be positive, got {value}")
    return value


def validate_hex_color(value: str) -> str:
    """Validate that a string is a valid hex color.
    
    Args:
        value: String to validate
        
    Returns:
        The validated hex color (normalized to uppercase)
        
    Raises:
        ValueError: If the string is not a valid hex color
    """
    if not isinstance(value, str):
        raise ValueError(f"Hex color must be a string, got {type(value).__name__}")
    
    # Remove leading # if present
    if value.startswith("#"):
        value = value[1:]
    
    # Check length (3 or 6 characters)
    if len(value) not in (3, 6):
        raise ValueError(f"Hex color must be 3 or 6 characters, got {len(value)}")
    
    # Check all characters are valid hex
    try:
        int(value, 16)
    except ValueError:
        raise ValueError(f"Invalid hex color: {value}")
    
    # Normalize to 6 characters
    if len(value) == 3:
        value = "".join(c * 2 for c in value)
    
    return value.upper()


def clamp(value: float, min_value: float, max_value: float) -> float:
    """Clamp a value between min and max.
    
    Args:
        value: Value to clamp
        min_value: Minimum allowed value
        max_value: Maximum allowed value
        
    Returns:
        Clamped value
    """
    return max(min_value, min(max_value, value))