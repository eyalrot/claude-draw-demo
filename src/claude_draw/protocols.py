"""Protocol definitions for Claude Draw."""

from typing import Protocol, Any, TYPE_CHECKING

if TYPE_CHECKING:
    from claude_draw.shapes import Circle, Rectangle, Line, Path
    from claude_draw.containers import Group, Layer, Canvas
    from claude_draw.base import Drawable


class DrawableVisitor(Protocol):
    """Protocol for visiting drawable objects.
    
    This protocol defines the interface that all visitor implementations must follow.
    Each visitor method corresponds to a specific type of drawable object and defines
    how that object should be processed.
    
    The visitor pattern allows for separation of concerns between the data structure
    (drawable objects) and the operations performed on them (rendering, analysis, etc.).
    """
    
    def visit_circle(self, circle: "Circle") -> Any:
        """Visit a circle object.
        
        Args:
            circle: The circle object to visit
            
        Returns:
            Any: Result of processing the circle (depends on visitor implementation)
        """
        ...
    
    def visit_rectangle(self, rectangle: "Rectangle") -> Any:
        """Visit a rectangle object.
        
        Args:
            rectangle: The rectangle object to visit
            
        Returns:
            Any: Result of processing the rectangle (depends on visitor implementation)
        """
        ...
    
    def visit_line(self, line: "Line") -> Any:
        """Visit a line object.
        
        Args:
            line: The line object to visit
            
        Returns:
            Any: Result of processing the line (depends on visitor implementation)
        """
        ...
    
    def visit_path(self, path: "Path") -> Any:
        """Visit a path object.
        
        Args:
            path: The path object to visit
            
        Returns:
            Any: Result of processing the path (depends on visitor implementation)
        """
        ...
    
    def visit_group(self, group: "Group") -> Any:
        """Visit a group object.
        
        Args:
            group: The group object to visit
            
        Returns:
            Any: Result of processing the group (depends on visitor implementation)
        """
        ...
    
    def visit_layer(self, layer: "Layer") -> Any:
        """Visit a layer object.
        
        Args:
            layer: The layer object to visit
            
        Returns:
            Any: Result of processing the layer (depends on visitor implementation)
        """
        ...
    
    def visit_canvas(self, canvas: "Canvas") -> Any:
        """Visit a canvas object.
        
        Args:
            canvas: The canvas object to visit
            
        Returns:
            Any: Result of processing the canvas (depends on visitor implementation)
        """
        ...


class Renderer(Protocol):
    """Protocol for rendering drawable objects to different output formats.
    
    This protocol defines the interface for rendering systems that can convert
    drawable objects into various output formats (SVG, PNG, PDF, etc.).
    
    Renderers typically implement the visitor pattern to traverse the drawable
    object hierarchy and generate appropriate output for each object type.
    """
    
    def render(self, drawable: "Drawable") -> str:
        """Render a drawable object to the target output format.
        
        Args:
            drawable: The drawable object to render
            
        Returns:
            str: The rendered output in the target format
        """
        ...
    
    def begin_render(self) -> None:
        """Initialize the rendering context.
        
        This method is called before rendering begins and allows the renderer
        to set up any necessary state, headers, or initialization code.
        """
        ...
    
    def end_render(self) -> str:
        """Finalize the rendering process and return the complete output.
        
        This method is called after all objects have been rendered and allows
        the renderer to add any necessary footers, cleanup, or post-processing.
        
        Returns:
            str: The complete rendered output
        """
        ...
    
    def get_mime_type(self) -> str:
        """Get the MIME type of the output format.
        
        Returns:
            str: The MIME type (e.g., 'image/svg+xml', 'image/png')
        """
        ...
    
    def get_file_extension(self) -> str:
        """Get the file extension for the output format.
        
        Returns:
            str: The file extension (e.g., '.svg', '.png', '.pdf')
        """
        ...