"""Factory functions for creating shape objects."""

from typing import Optional
from claude_draw.shapes import Circle, Rectangle, Ellipse, Line
from claude_draw.models.point import Point2D
from claude_draw.models.color import Color


def create_circle(x: float, y: float, radius: float, 
                  fill: Optional[Color] = None, 
                  stroke: Optional[Color] = None,
                  stroke_width: float = 1.0) -> Circle:
    """Create a circle with the specified parameters.
    
    Args:
        x: X coordinate of the center
        y: Y coordinate of the center
        radius: Radius of the circle (must be positive)
        fill: Optional fill color
        stroke: Optional stroke color
        stroke_width: Stroke width (default: 1.0)
        
    Returns:
        Circle: A new circle instance
        
    Raises:
        ValueError: If radius is not positive
    """
    if radius <= 0:
        raise ValueError("Circle radius must be positive")
    
    center = Point2D(x=x, y=y)
    return Circle(
        center=center,
        radius=radius,
        fill=fill,
        stroke=stroke,
        stroke_width=stroke_width
    )


def create_rectangle(x: float, y: float, width: float, height: float,
                     fill: Optional[Color] = None,
                     stroke: Optional[Color] = None,
                     stroke_width: float = 1.0) -> Rectangle:
    """Create a rectangle with the specified parameters.
    
    Args:
        x: X coordinate of the top-left corner
        y: Y coordinate of the top-left corner
        width: Width of the rectangle (must be positive)
        height: Height of the rectangle (must be positive)
        fill: Optional fill color
        stroke: Optional stroke color
        stroke_width: Stroke width (default: 1.0)
        
    Returns:
        Rectangle: A new rectangle instance
        
    Raises:
        ValueError: If width or height is not positive
    """
    if width <= 0:
        raise ValueError("Rectangle width must be positive")
    if height <= 0:
        raise ValueError("Rectangle height must be positive")
    
    return Rectangle(
        x=x,
        y=y,
        width=width,
        height=height,
        fill=fill,
        stroke=stroke,
        stroke_width=stroke_width
    )


def create_square(x: float, y: float, size: float,
                  fill: Optional[Color] = None,
                  stroke: Optional[Color] = None,
                  stroke_width: float = 1.0) -> Rectangle:
    """Create a square (rectangle with equal width and height).
    
    Args:
        x: X coordinate of the top-left corner
        y: Y coordinate of the top-left corner
        size: Size of the square (must be positive)
        fill: Optional fill color
        stroke: Optional stroke color
        stroke_width: Stroke width (default: 1.0)
        
    Returns:
        Rectangle: A new square rectangle instance
        
    Raises:
        ValueError: If size is not positive
    """
    return create_rectangle(x, y, size, size, fill, stroke, stroke_width)


def create_ellipse(x: float, y: float, rx: float, ry: float,
                   fill: Optional[Color] = None,
                   stroke: Optional[Color] = None,
                   stroke_width: float = 1.0) -> Ellipse:
    """Create an ellipse with the specified parameters.
    
    Args:
        x: X coordinate of the center
        y: Y coordinate of the center
        rx: Horizontal radius (must be positive)
        ry: Vertical radius (must be positive)
        fill: Optional fill color
        stroke: Optional stroke color
        stroke_width: Stroke width (default: 1.0)
        
    Returns:
        Ellipse: A new ellipse instance
        
    Raises:
        ValueError: If rx or ry is not positive
    """
    if rx <= 0:
        raise ValueError("Ellipse horizontal radius must be positive")
    if ry <= 0:
        raise ValueError("Ellipse vertical radius must be positive")
    
    center = Point2D(x=x, y=y)
    return Ellipse(
        center=center,
        rx=rx,
        ry=ry,
        fill=fill,
        stroke=stroke,
        stroke_width=stroke_width
    )


def create_line(x1: float, y1: float, x2: float, y2: float,
                stroke: Optional[Color] = None,
                stroke_width: float = 1.0) -> Line:
    """Create a line with the specified endpoints.
    
    Args:
        x1: X coordinate of the start point
        y1: Y coordinate of the start point
        x2: X coordinate of the end point
        y2: Y coordinate of the end point
        stroke: Optional stroke color
        stroke_width: Stroke width (default: 1.0)
        
    Returns:
        Line: A new line instance
    """
    start = Point2D(x=x1, y=y1)
    end = Point2D(x=x2, y=y2)
    return Line(
        start=start,
        end=end,
        stroke=stroke,
        stroke_width=stroke_width
    )


def create_horizontal_line(x: float, y: float, length: float,
                           stroke: Optional[Color] = None,
                           stroke_width: float = 1.0) -> Line:
    """Create a horizontal line.
    
    Args:
        x: X coordinate of the start point
        y: Y coordinate of the line
        length: Length of the line (can be negative for leftward direction)
        stroke: Optional stroke color
        stroke_width: Stroke width (default: 1.0)
        
    Returns:
        Line: A new horizontal line instance
    """
    return create_line(x, y, x + length, y, stroke, stroke_width)


def create_vertical_line(x: float, y: float, length: float,
                         stroke: Optional[Color] = None,
                         stroke_width: float = 1.0) -> Line:
    """Create a vertical line.
    
    Args:
        x: X coordinate of the line
        y: Y coordinate of the start point
        length: Length of the line (can be negative for upward direction)
        stroke: Optional stroke color
        stroke_width: Stroke width (default: 1.0)
        
    Returns:
        Line: A new vertical line instance
    """
    return create_line(x, y, x, y + length, stroke, stroke_width)


# Convenience functions for common patterns
def create_circle_from_diameter(x: float, y: float, diameter: float,
                                fill: Optional[Color] = None,
                                stroke: Optional[Color] = None,
                                stroke_width: float = 1.0) -> Circle:
    """Create a circle from its diameter instead of radius.
    
    Args:
        x: X coordinate of the center
        y: Y coordinate of the center
        diameter: Diameter of the circle (must be positive)
        fill: Optional fill color
        stroke: Optional stroke color
        stroke_width: Stroke width (default: 1.0)
        
    Returns:
        Circle: A new circle instance
        
    Raises:
        ValueError: If diameter is not positive
    """
    if diameter <= 0:
        raise ValueError("Circle diameter must be positive")
    
    return create_circle(x, y, diameter / 2, fill, stroke, stroke_width)


def create_rectangle_from_corners(x1: float, y1: float, x2: float, y2: float,
                                  fill: Optional[Color] = None,
                                  stroke: Optional[Color] = None,
                                  stroke_width: float = 1.0) -> Rectangle:
    """Create a rectangle from two corner points.
    
    Args:
        x1: X coordinate of the first corner
        y1: Y coordinate of the first corner
        x2: X coordinate of the second corner
        y2: Y coordinate of the second corner
        fill: Optional fill color
        stroke: Optional stroke color
        stroke_width: Stroke width (default: 1.0)
        
    Returns:
        Rectangle: A new rectangle instance
        
    Raises:
        ValueError: If the corners define a degenerate rectangle
    """
    x = min(x1, x2)
    y = min(y1, y2)
    width = abs(x2 - x1)
    height = abs(y2 - y1)
    
    if width <= 0 or height <= 0:
        raise ValueError("Rectangle corners must define a valid rectangle")
    
    return create_rectangle(x, y, width, height, fill, stroke, stroke_width)


def create_rectangle_from_center(cx: float, cy: float, width: float, height: float,
                                 fill: Optional[Color] = None,
                                 stroke: Optional[Color] = None,
                                 stroke_width: float = 1.0) -> Rectangle:
    """Create a rectangle from its center point and dimensions.
    
    Args:
        cx: X coordinate of the center
        cy: Y coordinate of the center
        width: Width of the rectangle (must be positive)
        height: Height of the rectangle (must be positive)
        fill: Optional fill color
        stroke: Optional stroke color
        stroke_width: Stroke width (default: 1.0)
        
    Returns:
        Rectangle: A new rectangle instance
        
    Raises:
        ValueError: If width or height is not positive
    """
    if width <= 0:
        raise ValueError("Rectangle width must be positive")
    if height <= 0:
        raise ValueError("Rectangle height must be positive")
    
    x = cx - width / 2
    y = cy - height / 2
    
    return create_rectangle(x, y, width, height, fill, stroke, stroke_width)


def create_ellipse_from_circle(x: float, y: float, radius: float,
                               fill: Optional[Color] = None,
                               stroke: Optional[Color] = None,
                               stroke_width: float = 1.0) -> Ellipse:
    """Create a circular ellipse (both radii equal).
    
    Args:
        x: X coordinate of the center
        y: Y coordinate of the center
        radius: Radius for both axes (must be positive)
        fill: Optional fill color
        stroke: Optional stroke color
        stroke_width: Stroke width (default: 1.0)
        
    Returns:
        Ellipse: A new circular ellipse instance
        
    Raises:
        ValueError: If radius is not positive
    """
    if radius <= 0:
        raise ValueError("Ellipse radius must be positive")
    
    return create_ellipse(x, y, radius, radius, fill, stroke, stroke_width)