"""Tests for protocol definitions."""

import pytest
from typing import Any
from claude_draw.protocols import DrawableVisitor, Renderer
from claude_draw.base import Drawable


class MockDrawableVisitor:
    """Mock implementation of DrawableVisitor for testing."""
    
    def __init__(self):
        self.visited = []
    
    def visit_circle(self, circle) -> str:
        self.visited.append(('circle', circle))
        return "visited_circle"
    
    def visit_rectangle(self, rectangle) -> str:
        self.visited.append(('rectangle', rectangle))
        return "visited_rectangle"
    
    def visit_line(self, line) -> str:
        self.visited.append(('line', line))
        return "visited_line"
    
    def visit_path(self, path) -> str:
        self.visited.append(('path', path))
        return "visited_path"
    
    def visit_group(self, group) -> str:
        self.visited.append(('group', group))
        return "visited_group"
    
    def visit_layer(self, layer) -> str:
        self.visited.append(('layer', layer))
        return "visited_layer"
    
    def visit_canvas(self, canvas) -> str:
        self.visited.append(('canvas', canvas))
        return "visited_canvas"


class MockRenderer:
    """Mock implementation of Renderer for testing."""
    
    def __init__(self):
        self.rendered_objects = []
        self.begun = False
        self.ended = False
    
    def render(self, drawable: Drawable) -> str:
        self.rendered_objects.append(drawable)
        return f"rendered_{drawable.__class__.__name__}"
    
    def begin_render(self) -> None:
        self.begun = True
    
    def end_render(self) -> str:
        self.ended = True
        return "complete_render"
    
    def get_mime_type(self) -> str:
        return "image/svg+xml"
    
    def get_file_extension(self) -> str:
        return ".svg"


class TestDrawableVisitor:
    """Test cases for DrawableVisitor protocol."""
    
    def test_visitor_protocol_implementation(self):
        """Test that a class can implement the DrawableVisitor protocol."""
        visitor = MockDrawableVisitor()
        
        # Check that the visitor implements the protocol
        assert hasattr(visitor, 'visit_circle')
        assert hasattr(visitor, 'visit_rectangle')
        assert hasattr(visitor, 'visit_line')
        assert hasattr(visitor, 'visit_path')
        assert hasattr(visitor, 'visit_group')
        assert hasattr(visitor, 'visit_layer')
        assert hasattr(visitor, 'visit_canvas')
    
    def test_visitor_can_be_used_as_protocol(self):
        """Test that a visitor can be used where the protocol is expected."""
        def process_with_visitor(visitor: DrawableVisitor, obj: Any) -> str:
            # This would normally be called by the drawable object
            return visitor.visit_circle(obj)
        
        visitor = MockDrawableVisitor()
        result = process_with_visitor(visitor, "fake_circle")
        
        assert result == "visited_circle"
        assert visitor.visited == [('circle', "fake_circle")]
    
    def test_visitor_methods_return_values(self):
        """Test that visitor methods can return different types."""
        visitor = MockDrawableVisitor()
        
        # Test different visitor methods
        assert visitor.visit_circle("circle") == "visited_circle"
        assert visitor.visit_rectangle("rectangle") == "visited_rectangle"
        assert visitor.visit_line("line") == "visited_line"
        assert visitor.visit_path("path") == "visited_path"
        assert visitor.visit_group("group") == "visited_group"
        assert visitor.visit_layer("layer") == "visited_layer"
        assert visitor.visit_canvas("canvas") == "visited_canvas"
    
    def test_visitor_tracks_visited_objects(self):
        """Test that visitor can track which objects were visited."""
        visitor = MockDrawableVisitor()
        
        visitor.visit_circle("circle1")
        visitor.visit_rectangle("rect1")
        visitor.visit_circle("circle2")
        
        expected = [
            ('circle', "circle1"),
            ('rectangle', "rect1"),
            ('circle', "circle2")
        ]
        assert visitor.visited == expected


class TestRenderer:
    """Test cases for Renderer protocol."""
    
    def test_renderer_protocol_implementation(self):
        """Test that a class can implement the Renderer protocol."""
        renderer = MockRenderer()
        
        # Check that the renderer implements the protocol
        assert hasattr(renderer, 'render')
        assert hasattr(renderer, 'begin_render')
        assert hasattr(renderer, 'end_render')
        assert hasattr(renderer, 'get_mime_type')
        assert hasattr(renderer, 'get_file_extension')
    
    def test_renderer_can_be_used_as_protocol(self):
        """Test that a renderer can be used where the protocol is expected."""
        def render_with_renderer(renderer: Renderer, obj: Any) -> str:
            renderer.begin_render()
            result = renderer.render(obj)
            renderer.end_render()
            return result
        
        renderer = MockRenderer()
        result = render_with_renderer(renderer, "fake_drawable")
        
        assert result == "rendered_str"
        assert renderer.begun is True
        assert renderer.ended is True
    
    def test_renderer_lifecycle(self):
        """Test the renderer lifecycle methods."""
        renderer = MockRenderer()
        
        # Initially not begun or ended
        assert renderer.begun is False
        assert renderer.ended is False
        
        # Begin rendering
        renderer.begin_render()
        assert renderer.begun is True
        assert renderer.ended is False
        
        # End rendering
        result = renderer.end_render()
        assert renderer.begun is True
        assert renderer.ended is True
        assert result == "complete_render"
    
    def test_renderer_metadata(self):
        """Test renderer metadata methods."""
        renderer = MockRenderer()
        
        assert renderer.get_mime_type() == "image/svg+xml"
        assert renderer.get_file_extension() == ".svg"
    
    def test_renderer_tracks_rendered_objects(self):
        """Test that renderer can track rendered objects."""
        renderer = MockRenderer()
        
        # Mock drawable objects
        obj1 = type('MockDrawable', (), {'__class__': type('MockDrawable', (), {})})()
        obj2 = type('MockDrawable', (), {'__class__': type('MockDrawable', (), {})})()
        
        renderer.render(obj1)
        renderer.render(obj2)
        
        assert len(renderer.rendered_objects) == 2
        assert renderer.rendered_objects[0] is obj1
        assert renderer.rendered_objects[1] is obj2


class TestProtocolIntegration:
    """Test cases for protocol integration."""
    
    def test_protocols_can_be_used_together(self):
        """Test that visitor and renderer protocols can work together."""
        class VisitorRenderer(MockRenderer, MockDrawableVisitor):
            """Combined visitor and renderer."""
            
            def __init__(self):
                MockRenderer.__init__(self)
                MockDrawableVisitor.__init__(self)
            
            def render_via_visitor(self, drawable: Any) -> str:
                # Use visitor pattern to render
                return self.visit_circle(drawable)
        
        combined = VisitorRenderer()
        
        # Test both interfaces work
        assert combined.get_mime_type() == "image/svg+xml"
        assert combined.visit_circle("circle") == "visited_circle"
        assert combined.render_via_visitor("circle") == "visited_circle"
    
    def test_protocol_type_hints(self):
        """Test that protocol type hints work correctly."""
        def process_drawable(visitor: DrawableVisitor, renderer: Renderer, obj: Any) -> tuple:
            visitor_result = visitor.visit_circle(obj)
            renderer_result = renderer.render(obj)
            return visitor_result, renderer_result
        
        visitor = MockDrawableVisitor()
        renderer = MockRenderer()
        
        result = process_drawable(visitor, renderer, "test_obj")
        
        assert result == ("visited_circle", "rendered_str")
        assert visitor.visited == [('circle', "test_obj")]
        assert len(renderer.rendered_objects) == 1