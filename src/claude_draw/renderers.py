"""Concrete renderer implementations for Claude Draw."""

import math
from typing import Any, TYPE_CHECKING
from claude_draw.visitors import BaseRenderer
from claude_draw.models.point import Point2D

if TYPE_CHECKING:
    from claude_draw.shapes import Circle, Rectangle, Line, Ellipse
    from claude_draw.containers import Group, Layer, Drawing


class SVGRenderer(BaseRenderer):
    """SVG renderer that demonstrates the visitor pattern.
    
    This renderer generates SVG markup by visiting drawable objects
    and converting them to their SVG equivalents.
    """
    
    def __init__(self, width: float = 800, height: float = 600):
        """Initialize the SVG renderer.
        
        Args:
            width: Canvas width in pixels
            height: Canvas height in pixels
        """
        super().__init__()
        self.width = width
        self.height = height
        self._defs_content = []
        self._gradient_counter = 0
    
    def begin_render(self) -> None:
        """Initialize the SVG document."""
        self.emit(f'<?xml version="1.0" encoding="UTF-8"?>\n')
        self.emit(f'<svg width="{self.width}" height="{self.height}" ')
        self.emit(f'xmlns="http://www.w3.org/2000/svg" ')
        self.emit(f'viewBox="0 0 {self.width} {self.height}">\n')
        
        # Add definitions section for gradients and patterns
        self.emit('<defs>\n')
    
    def end_render(self) -> None:
        """Finalize the SVG document."""
        # Close defs section
        for def_content in self._defs_content:
            self.emit(def_content)
        self.emit('</defs>\n')
        
        # Close SVG tag
        self.emit('</svg>\n')
    
    def pre_visit(self, drawable) -> None:
        """Push drawable's transform to context before visiting."""
        if hasattr(drawable, 'transform') and not drawable.transform.is_identity():
            self.context.push_transform(drawable.transform)
    
    def post_visit(self, drawable) -> None:
        """Pop drawable's transform from context after visiting."""
        if hasattr(drawable, 'transform') and not drawable.transform.is_identity():
            self.context.pop_transform()
    
    def _format_transform(self) -> str:
        """Format the current transformation matrix as SVG transform attribute.
        
        Returns:
            str: SVG transform attribute or empty string if identity
        """
        transform = self.context.current_transform
        
        # Check if it's an identity matrix
        if transform.is_identity():
            return ""
        
        # Format as SVG matrix transform
        return f'transform="matrix({transform.a},{transform.b},{transform.c},{transform.d},{transform.tx},{transform.ty})"'
    
    def _format_style_attributes(self, drawable) -> str:
        """Format style attributes for an SVG element.
        
        Args:
            drawable: The drawable object with style properties
            
        Returns:
            str: SVG style attributes
        """
        attrs = []
        state = self.context.current_state
        
        # Fill color
        if hasattr(drawable, 'fill') and drawable.fill is not None:
            fill_color = drawable.fill.to_hex()
            attrs.append(f'fill="{fill_color}"')
        elif state.fill is not None:
            fill_color = state.fill.to_hex()
            attrs.append(f'fill="{fill_color}"')
        else:
            attrs.append('fill="none"')
        
        # Stroke properties
        if hasattr(drawable, 'stroke') and drawable.stroke is not None:
            stroke_color = drawable.stroke.to_hex()
            attrs.append(f'stroke="{stroke_color}"')
            
            stroke_width = getattr(drawable, 'stroke_width', 1.0)
            attrs.append(f'stroke-width="{stroke_width}"')
        elif state.stroke is not None:
            stroke_color = state.stroke.to_hex()
            attrs.append(f'stroke="{stroke_color}"')
            attrs.append(f'stroke-width="{state.stroke_width}"')
        else:
            attrs.append('stroke="none"')
        
        # Opacity
        effective_opacity = self.context.get_effective_opacity()
        if hasattr(drawable, 'opacity'):
            effective_opacity *= drawable.opacity
        
        if effective_opacity < 1.0:
            attrs.append(f'opacity="{effective_opacity:.3f}"')
        
        return " ".join(attrs)
    
    def visit_circle(self, circle: "Circle") -> Any:
        """Render a circle as SVG <circle> element."""
        self.pre_visit(circle)
        
        if not self.context.is_visible():
            self.post_visit(circle)
            return
        
        transform_attr = self._format_transform()
        style_attrs = self._format_style_attributes(circle)
        
        self.emit(f'<circle cx="{circle.center.x:.3f}" cy="{circle.center.y:.3f}" ')
        self.emit(f'r="{circle.radius:.3f}" ')
        if transform_attr:
            self.emit(f'{transform_attr} ')
        self.emit(f'{style_attrs}/>\n')
        
        self.post_visit(circle)
    
    def visit_rectangle(self, rectangle: "Rectangle") -> Any:
        """Render a rectangle as SVG <rect> element."""
        self.pre_visit(rectangle)
        
        if not self.context.is_visible():
            return
        
        transform_attr = self._format_transform()
        style_attrs = self._format_style_attributes(rectangle)
        
        self.emit(f'<rect x="{rectangle.x:.3f}" y="{rectangle.y:.3f}" ')
        self.emit(f'width="{rectangle.width:.3f}" height="{rectangle.height:.3f}" ')
        if transform_attr:
            self.emit(f'{transform_attr} ')
        self.emit(f'{style_attrs}/>\n')
        
        self.post_visit(rectangle)
    
    def visit_ellipse(self, ellipse: "Ellipse") -> Any:
        """Render an ellipse as SVG <ellipse> element."""
        self.pre_visit(ellipse)
        
        if not self.context.is_visible():
            return
        
        # Apply transformation to center point
        center = self.context.current_transform.transform_point(ellipse.center)
        
        transform_attr = self._format_transform()
        style_attrs = self._format_style_attributes(ellipse)
        
        self.emit(f'<ellipse cx="{center.x:.3f}" cy="{center.y:.3f}" ')
        self.emit(f'rx="{ellipse.rx:.3f}" ry="{ellipse.ry:.3f}" ')
        if transform_attr:
            self.emit(f'{transform_attr} ')
        self.emit(f'{style_attrs}/>\n')
        
        self.post_visit(ellipse)
    
    def visit_line(self, line: "Line") -> Any:
        """Render a line as SVG <line> element."""
        self.pre_visit(line)
        
        if not self.context.is_visible():
            return
        
        # Apply transformation to endpoints
        start = self.context.current_transform.transform_point(line.start)
        end = self.context.current_transform.transform_point(line.end)
        
        transform_attr = self._format_transform()
        style_attrs = self._format_style_attributes(line)
        
        self.emit(f'<line x1="{start.x:.3f}" y1="{start.y:.3f}" ')
        self.emit(f'x2="{end.x:.3f}" y2="{end.y:.3f}" ')
        if transform_attr:
            self.emit(f'{transform_attr} ')
        self.emit(f'{style_attrs}/>\n')
        
        self.post_visit(line)
    
    def visit_group(self, group: "Group") -> Any:
        """Render a group as SVG <g> element."""
        self.pre_visit(group)
        
        if not self.context.is_visible():
            return
        
        # Start group element
        transform_attr = self._format_transform()
        
        self.emit('<g ')
        if transform_attr:
            self.emit(f'{transform_attr} ')
        
        # Add group name as id if available
        if group.name:
            self.emit(f'id="{group.name}" ')
        
        self.emit('>\n')
        
        # Use the parent class implementation for children traversal
        super().visit_group(group)
        
        # Close group element
        self.emit('</g>\n')
        
        self.post_visit(group)
    
    def visit_layer(self, layer: "Layer") -> Any:
        """Render a layer as SVG <g> element with layer properties."""
        self.pre_visit(layer)
        
        if not layer.visible or not self.context.is_visible():
            return
        
        # Start layer group element
        transform_attr = self._format_transform()
        
        self.emit('<g ')
        if transform_attr:
            self.emit(f'{transform_attr} ')
        
        # Add layer name as id if available
        if layer.name:
            self.emit(f'id="layer_{layer.name}" ')
        
        # Add opacity if less than 1.0
        if layer.opacity < 1.0:
            self.emit(f'opacity="{layer.opacity:.3f}" ')
        
        # Add blend mode if not normal
        if layer.blend_mode.value != "normal":
            self.emit(f'style="mix-blend-mode:{layer.blend_mode.value}" ')
        
        self.emit('>\n')
        
        # Use the parent class implementation for children traversal
        super().visit_layer(layer)
        
        # Close layer group element
        self.emit('</g>\n')
        
        self.post_visit(layer)
    
    def visit_drawing(self, drawing: "Drawing") -> Any:
        """Render a drawing - set canvas dimensions and render children."""
        self.pre_visit(drawing)
        
        # Update canvas dimensions from drawing and restart SVG with correct dimensions
        self.width = drawing.width
        self.height = drawing.height
        
        # Clear previous output and restart with correct dimensions
        self._output.clear()
        self.begin_render()
        
        # Add background if specified
        if drawing.background_color:
            self.emit(f'<rect x="0" y="0" width="{self.width}" ')
            self.emit(f'height="{self.height}" fill="{drawing.background_color}"/>\n')
        
        # Use the parent class implementation for children traversal
        super().visit_drawing(drawing)
        
        self.post_visit(drawing)


class BoundingBoxCalculator(BaseRenderer):
    """Example visitor that calculates bounding boxes instead of rendering.
    
    This demonstrates how the visitor pattern enables different operations
    on the same drawable object hierarchy.
    """
    
    def __init__(self):
        """Initialize the bounding box calculator."""
        super().__init__()
        self.min_x = float('inf')
        self.min_y = float('inf')
        self.max_x = float('-inf')
        self.max_y = float('-inf')
    
    def begin_render(self) -> None:
        """Reset bounding box calculation."""
        self.min_x = float('inf')
        self.min_y = float('inf')
        self.max_x = float('-inf')
        self.max_y = float('-inf')
    
    def end_render(self) -> None:
        """Finalize bounding box calculation."""
        pass
    
    def get_bounding_box(self):
        """Get the calculated bounding box.
        
        Returns:
            tuple: (min_x, min_y, width, height) or None if no bounds
        """
        if self.min_x == float('inf'):
            return None
        
        return (
            self.min_x,
            self.min_y,
            self.max_x - self.min_x,
            self.max_y - self.min_y
        )
    
    def _update_bounds(self, x: float, y: float) -> None:
        """Update the overall bounding box with a point.
        
        Args:
            x: X coordinate
            y: Y coordinate
        """
        self.min_x = min(self.min_x, x)
        self.min_y = min(self.min_y, y)
        self.max_x = max(self.max_x, x)
        self.max_y = max(self.max_y, y)
    
    def visit_circle(self, circle: "Circle") -> Any:
        """Calculate bounds for a circle."""
        center = self.context.current_transform.transform_point(circle.center)
        
        # For simplicity, assume uniform scaling
        self._update_bounds(center.x - circle.radius, center.y - circle.radius)
        self._update_bounds(center.x + circle.radius, center.y + circle.radius)
    
    def visit_rectangle(self, rectangle: "Rectangle") -> Any:
        """Calculate bounds for a rectangle."""
        # Transform all four corners
        corners = [
            Point2D(x=rectangle.x, y=rectangle.y),
            Point2D(x=rectangle.x + rectangle.width, y=rectangle.y),
            Point2D(x=rectangle.x + rectangle.width, y=rectangle.y + rectangle.height),
            Point2D(x=rectangle.x, y=rectangle.y + rectangle.height)
        ]
        
        for corner in corners:
            transformed = self.context.current_transform.transform_point(corner)
            self._update_bounds(transformed.x, transformed.y)
    
    def visit_ellipse(self, ellipse: "Ellipse") -> Any:
        """Calculate bounds for an ellipse."""
        center = self.context.current_transform.transform_point(ellipse.center)
        
        # For simplicity, use axis-aligned bounding box
        self._update_bounds(center.x - ellipse.rx, center.y - ellipse.ry)
        self._update_bounds(center.x + ellipse.rx, center.y + ellipse.ry)
    
    def visit_line(self, line: "Line") -> Any:
        """Calculate bounds for a line."""
        start = self.context.current_transform.transform_point(line.start)
        end = self.context.current_transform.transform_point(line.end)
        
        self._update_bounds(start.x, start.y)
        self._update_bounds(end.x, end.y)