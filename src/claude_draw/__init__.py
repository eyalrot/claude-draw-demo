"""Claude Draw - A Python library for creating 2D vector graphics."""

__version__ = "0.1.0"
__author__ = "Claude Draw Contributors"

from claude_draw.core import Canvas, Circle, Rectangle
from claude_draw.base import Drawable, StyleMixin, Primitive, Container
from claude_draw.protocols import DrawableVisitor, Renderer
from claude_draw.shapes import Circle as NewCircle, Rectangle as NewRectangle, Ellipse, Line
from claude_draw.containers import Group, Layer, Drawing, BlendMode
from claude_draw.visitors import RenderContext, RenderState, BaseRenderer
from claude_draw.renderers import SVGRenderer, BoundingBoxCalculator
from claude_draw.serialization import (
    serialize_drawable, deserialize_drawable, load_drawable, save_drawable,
    register_drawable_type, EnhancedJSONEncoder
)
from claude_draw.factories import (
    create_circle, create_rectangle, create_square, create_ellipse, create_line,
    create_horizontal_line, create_vertical_line, create_circle_from_diameter,
    create_rectangle_from_corners, create_rectangle_from_center, create_ellipse_from_circle
)

__all__ = [
    "Canvas", 
    "Circle", 
    "Rectangle", 
    "Drawable",
    "StyleMixin", 
    "Primitive", 
    "Container",
    "DrawableVisitor",
    "Renderer",
    "NewCircle",
    "NewRectangle", 
    "Ellipse",
    "Line",
    # Container classes
    "Group",
    "Layer", 
    "Drawing",
    "BlendMode",
    # Visitor pattern classes
    "RenderContext",
    "RenderState",
    "BaseRenderer",
    "SVGRenderer",
    "BoundingBoxCalculator",
    # Serialization functions
    "serialize_drawable",
    "deserialize_drawable", 
    "load_drawable",
    "save_drawable",
    "register_drawable_type",
    "EnhancedJSONEncoder",
    # Factory functions
    "create_circle",
    "create_rectangle", 
    "create_square",
    "create_ellipse",
    "create_line",
    "create_horizontal_line",
    "create_vertical_line",
    "create_circle_from_diameter",
    "create_rectangle_from_corners",
    "create_rectangle_from_center",
    "create_ellipse_from_circle",
    "__version__"
]
