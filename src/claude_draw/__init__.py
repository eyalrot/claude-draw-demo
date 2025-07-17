"""Claude Draw - A Python library for creating 2D vector graphics."""

__version__ = "0.1.0"
__author__ = "Claude Draw Contributors"

from claude_draw.core import Canvas, Circle, Rectangle
from claude_draw.base import Drawable, StyleMixin, Primitive, Container
from claude_draw.protocols import DrawableVisitor, Renderer

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
    "__version__"
]
