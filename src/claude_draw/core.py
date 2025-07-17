"""Core components for Claude Draw."""

from dataclasses import dataclass, field
from typing import Optional, Union


@dataclass
class Circle:
    """A circle shape."""

    x: float
    y: float
    radius: float
    fill: Optional[str] = None
    stroke: Optional[str] = None
    stroke_width: float = 1.0

    def to_svg(self) -> str:
        """Convert to SVG element."""
        attrs = [f'cx="{self.x}"', f'cy="{self.y}"', f'r="{self.radius}"']
        if self.fill:
            attrs.append(f'fill="{self.fill}"')
        else:
            attrs.append('fill="none"')
        if self.stroke:
            attrs.append(f'stroke="{self.stroke}"')
            attrs.append(f'stroke-width="{self.stroke_width}"')
        return f"<circle {' '.join(attrs)} />"


@dataclass
class Rectangle:
    """A rectangle shape."""

    x: float
    y: float
    width: float
    height: float
    fill: Optional[str] = None
    stroke: Optional[str] = None
    stroke_width: float = 1.0

    def to_svg(self) -> str:
        """Convert to SVG element."""
        attrs = [
            f'x="{self.x}"',
            f'y="{self.y}"',
            f'width="{self.width}"',
            f'height="{self.height}"',
        ]
        if self.fill:
            attrs.append(f'fill="{self.fill}"')
        else:
            attrs.append('fill="none"')
        if self.stroke:
            attrs.append(f'stroke="{self.stroke}"')
            attrs.append(f'stroke-width="{self.stroke_width}"')
        return f"<rect {' '.join(attrs)} />"


@dataclass
class Canvas:
    """A drawing canvas that contains shapes."""

    width: int = 800
    height: int = 600
    background: Optional[str] = "white"
    shapes: list[Union[Circle, Rectangle]] = field(default_factory=list)

    def add(self, shape: Union[Circle, Rectangle]) -> None:
        """Add a shape to the canvas."""
        self.shapes.append(shape)

    def to_svg(self) -> str:
        """Convert the canvas to SVG format."""
        svg_shapes = [shape.to_svg() for shape in self.shapes]

        svg_content = [
            f'<svg width="{self.width}" height="{self.height}" xmlns="http://www.w3.org/2000/svg">',
        ]

        if self.background:
            svg_content.append(
                f'<rect width="{self.width}" height="{self.height}" fill="{self.background}" />'
            )

        svg_content.extend(svg_shapes)
        svg_content.append("</svg>")

        return "\n".join(svg_content)

    def save(self, filename: str) -> None:
        """Save the canvas to a file."""
        if not filename.endswith(".svg"):
            filename += ".svg"

        with open(filename, "w") as f:
            f.write(self.to_svg())
