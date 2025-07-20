#!/usr/bin/env python3
"""
Simple SVG export utility for testing the house with trees example.

This is a basic implementation that will be replaced by the proper
visitor pattern renderer once Task 9 is implemented.
"""

from typing import List
from claude_draw.containers import Drawing, Layer, Group
from claude_draw.shapes import Circle, Rectangle, Ellipse
from claude_draw.models.color import Color


def color_to_svg(color: Color) -> str:
    """Convert a Color object to SVG color string."""
    if color is None:
        return "none"
    # For now, just try to get hex representation
    if hasattr(color, 'hex'):
        return color.hex
    elif hasattr(color, 'r') and hasattr(color, 'g') and hasattr(color, 'b'):
        return f"rgb({color.r},{color.g},{color.b})"
    else:
        return "black"  # fallback


def export_circle_to_svg(circle: Circle) -> str:
    """Export a Circle to SVG."""
    cx = circle.center.x
    cy = circle.center.y
    r = circle.radius
    
    fill = color_to_svg(circle.fill) if circle.fill else "none"
    stroke = color_to_svg(circle.stroke) if circle.stroke else "none"
    stroke_width = circle.stroke_width if circle.stroke else 0
    opacity = circle.opacity
    
    return f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{fill}" stroke="{stroke}" stroke-width="{stroke_width}" opacity="{opacity}"/>'


def export_rectangle_to_svg(rect: Rectangle) -> str:
    """Export a Rectangle to SVG."""
    x = rect.x
    y = rect.y
    width = rect.width
    height = rect.height
    
    fill = color_to_svg(rect.fill) if rect.fill else "none"
    stroke = color_to_svg(rect.stroke) if rect.stroke else "none"
    stroke_width = rect.stroke_width if rect.stroke else 0
    opacity = rect.opacity
    
    return f'<rect x="{x}" y="{y}" width="{width}" height="{height}" fill="{fill}" stroke="{stroke}" stroke-width="{stroke_width}" opacity="{opacity}"/>'


def export_ellipse_to_svg(ellipse: Ellipse) -> str:
    """Export an Ellipse to SVG."""
    cx = ellipse.center.x
    cy = ellipse.center.y
    rx = ellipse.rx
    ry = ellipse.ry
    
    fill = color_to_svg(ellipse.fill) if ellipse.fill else "none"
    stroke = color_to_svg(ellipse.stroke) if ellipse.stroke else "none"
    stroke_width = ellipse.stroke_width if ellipse.stroke else 0
    opacity = ellipse.opacity
    
    return f'<ellipse cx="{cx}" cy="{cy}" rx="{rx}" ry="{ry}" fill="{fill}" stroke="{stroke}" stroke-width="{stroke_width}" opacity="{opacity}"/>'


def export_drawable_to_svg(drawable) -> List[str]:
    """Export a drawable object to SVG elements."""
    elements = []
    
    if isinstance(drawable, Circle):
        elements.append(export_circle_to_svg(drawable))
    elif isinstance(drawable, Rectangle):
        elements.append(export_rectangle_to_svg(drawable))
    elif isinstance(drawable, Ellipse):
        elements.append(export_ellipse_to_svg(drawable))
    elif isinstance(drawable, (Group, Layer)):
        # Handle containers by recursively processing children
        if hasattr(drawable, 'name') and drawable.name:
            elements.append(f'<!-- {type(drawable).__name__}: {drawable.name} -->')
        for child in drawable.children:
            elements.extend(export_drawable_to_svg(child))
    
    return elements


def export_drawing_to_svg(drawing: Drawing, filename: str) -> None:
    """Export a Drawing to an SVG file."""
    
    # SVG header
    svg_lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{drawing.width}" height="{drawing.height}" viewBox="0 0 {drawing.width} {drawing.height}">',
    ]
    
    # Add title if present
    if drawing.title:
        svg_lines.append(f'  <title>{drawing.title}</title>')
    
    # Add description if present
    if drawing.description:
        svg_lines.append(f'  <desc>{drawing.description}</desc>')
    
    # Add background if present
    if drawing.background_color:
        svg_lines.append(f'  <rect x="0" y="0" width="{drawing.width}" height="{drawing.height}" fill="{drawing.background_color}"/>')
    
    # Process all children (layers)
    for layer in drawing.children:
        layer_elements = export_drawable_to_svg(layer)
        for element in layer_elements:
            svg_lines.append(f'  {element}')
    
    # SVG footer
    svg_lines.append('</svg>')
    
    # Write to file
    with open(filename, 'w') as f:
        f.write('\n'.join(svg_lines))
    
    print(f"SVG exported to: {filename}")


def main():
    """Test the SVG export with the house drawing."""
    from house_with_trees import create_house_with_trees
    
    print("Creating house with trees drawing...")
    drawing = create_house_with_trees()
    
    print("Exporting to SVG...")
    export_drawing_to_svg(drawing, "house_with_trees.svg")
    
    print("✅ SVG export complete!")
    

if __name__ == "__main__":
    main()