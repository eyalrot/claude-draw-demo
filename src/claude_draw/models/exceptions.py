"""Custom exceptions for Claude Draw models."""


class DrawModelError(Exception):
    """Base exception for all Claude Draw model errors."""
    pass


class ValidationError(DrawModelError):
    """Raised when model validation fails."""
    pass


class TransformError(DrawModelError):
    """Raised when a transformation operation fails."""
    pass


class ColorError(DrawModelError):
    """Raised when a color operation fails."""
    pass


class GeometryError(DrawModelError):
    """Raised when a geometric operation fails."""
    pass