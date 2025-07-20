"""Tests for visitor pattern implementation."""

import pytest
from claude_draw.visitors import RenderContext, RenderState, BaseRenderer
from claude_draw.renderers import SVGRenderer, BoundingBoxCalculator
from claude_draw.shapes import Circle, Rectangle, Line, Ellipse
from claude_draw.containers import Group, Layer, Drawing
from claude_draw.models.point import Point2D
from claude_draw.models.color import Color
from claude_draw.models.transform import Transform2D


class TestRenderContext:
    """Test cases for RenderContext."""
    
    def test_initial_state(self):
        """Test initial render context state."""
        context = RenderContext()
        
        assert context.current_transform.is_identity()
        assert context.current_state.opacity == 1.0
        assert context.current_state.visible is True
        assert context.get_effective_opacity() == 1.0
        assert context.is_visible() is True
    
    def test_transform_stack(self):
        """Test transformation matrix stack operations."""
        context = RenderContext()
        
        # Push a translation
        translate = Transform2D().translate(10, 20)
        context.push_transform(translate)
        
        current = context.current_transform
        assert current.tx == 10
        assert current.ty == 20
        
        # Push another transformation
        scale = Transform2D().scale(2.0, 2.0)
        context.push_transform(scale)
        
        current = context.current_transform
        # Should be composed: scale after translate
        # The existing translation is preserved, scale affects the matrix coefficients
        assert current.tx == 10  # Translation preserved
        assert current.ty == 20  # Translation preserved  
        assert current.a == 2.0  # Scale applied
        assert current.d == 2.0  # Scale applied
        
        # Pop transformations
        popped = context.pop_transform()
        assert context.current_transform.tx == 10
        assert context.current_transform.ty == 20
        
        context.pop_transform()
        assert context.current_transform.is_identity()
    
    def test_state_stack(self):
        """Test rendering state stack operations."""
        context = RenderContext()
        
        # Push state with updates
        context.push_state(opacity=0.5, visible=True)
        assert context.current_state.opacity == 0.5
        assert context.get_effective_opacity() == 0.5
        
        # Push another state
        context.push_state(opacity=0.8)
        assert context.current_state.opacity == 0.8
        assert context.get_effective_opacity() == 0.4  # 0.5 * 0.8
        
        # Pop states
        context.pop_state()
        assert context.current_state.opacity == 0.5
        assert context.get_effective_opacity() == 0.5
        
        context.pop_state()
        assert context.current_state.opacity == 1.0
        assert context.get_effective_opacity() == 1.0
    
    def test_combined_push_pop(self):
        """Test combined transform and state push/pop."""
        context = RenderContext()
        
        transform = Transform2D().translate(5, 5)
        context.push(transform, opacity=0.7)
        
        assert context.current_transform.tx == 5
        assert context.current_state.opacity == 0.7
        
        popped_transform, popped_state = context.pop()
        assert context.current_transform.is_identity()
        assert context.current_state.opacity == 1.0
        assert popped_transform.tx == 5
        assert popped_state.opacity == 0.7
    
    def test_cannot_pop_base_state(self):
        """Test that base states cannot be popped."""
        context = RenderContext()
        
        with pytest.raises(ValueError, match="Cannot pop base"):
            context.pop_transform()
        
        with pytest.raises(ValueError, match="Cannot pop base"):
            context.pop_state()


class TestSVGRenderer:
    """Test cases for SVG renderer."""
    
    def test_basic_svg_structure(self):
        """Test basic SVG document structure."""
        renderer = SVGRenderer(100, 200)
        circle = Circle(center=Point2D(x=50, y=50), radius=25)
        
        output = renderer.render(circle)
        
        assert output.startswith('<?xml version="1.0" encoding="UTF-8"?>')
        assert 'width="100"' in output
        assert 'height="200"' in output
        assert 'xmlns="http://www.w3.org/2000/svg"' in output
        assert output.endswith('</svg>\n')
    
    def test_circle_rendering(self):
        """Test circle SVG generation."""
        renderer = SVGRenderer()
        circle = Circle(
            center=Point2D(x=50, y=50), 
            radius=25,
            fill=Color.from_hex("#FF0000"),
            stroke=Color.from_hex("#000000"),
            stroke_width=2.0
        )
        
        output = renderer.render(circle)
        
        assert '<circle' in output
        assert 'cx="50.000" cy="50.000"' in output
        assert 'r="25.000"' in output
        assert 'fill="#FF0000"' in output
        assert 'stroke="#000000"' in output
        assert 'stroke-width="2.0"' in output
    
    def test_rectangle_rendering(self):
        """Test rectangle SVG generation."""
        renderer = SVGRenderer()
        rect = Rectangle(
            x=10, y=20, width=30, height=40,
            fill=Color.from_hex("#00FF00")
        )
        
        output = renderer.render(rect)
        
        assert '<rect' in output
        assert 'x="10.000" y="20.000"' in output
        assert 'width="30.000" height="40.000"' in output
        assert 'fill="#00FF00"' in output
    
    def test_group_rendering(self):
        """Test group SVG generation with children."""
        renderer = SVGRenderer()
        
        circle = Circle(center=Point2D(x=25, y=25), radius=10)
        rect = Rectangle(x=0, y=0, width=50, height=50)
        
        group = Group(name="test_group").add_child(rect).add_child(circle)
        
        output = renderer.render(group)
        
        assert '<g id="test_group"' in output
        assert '</g>' in output
        assert '<rect' in output
        assert '<circle' in output
    
    def test_layer_rendering(self):
        """Test layer SVG generation with opacity."""
        renderer = SVGRenderer()
        
        circle = Circle(center=Point2D(x=25, y=25), radius=10)
        layer = Layer(name="test_layer", opacity=0.5).add_child(circle)
        
        output = renderer.render(layer)
        
        assert '<g id="layer_test_layer"' in output
        assert 'opacity="0.500"' in output
        assert '<circle' in output
    
    def test_drawing_rendering(self):
        """Test complete drawing SVG generation."""
        renderer = SVGRenderer()
        
        circle = Circle(center=Point2D(x=50, y=50), radius=25)
        drawing = Drawing(
            width=200, height=150,
            background_color="#F0F0F0"
        ).add_child(circle)
        
        output = renderer.render(drawing)
        
        assert 'width="200' in output
        assert 'height="150' in output
        assert 'fill="#F0F0F0"' in output  # Background
        assert '<circle' in output
    
    def test_transformation_handling(self):
        """Test SVG transformation matrix generation."""
        renderer = SVGRenderer()
        
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        transformed_circle = circle.translate(20, 30).scale(2.0)
        
        output = renderer.render(transformed_circle)
        
        # Should include transform attribute
        assert 'transform="matrix(' in output


class TestBoundingBoxCalculator:
    """Test cases for bounding box calculator."""
    
    def test_circle_bounds(self):
        """Test bounding box calculation for circle."""
        calculator = BoundingBoxCalculator()
        circle = Circle(center=Point2D(x=50, y=50), radius=25)
        
        calculator.render(circle)
        bounds = calculator.get_bounding_box()
        
        assert bounds is not None
        x, y, width, height = bounds
        assert x == 25  # 50 - 25
        assert y == 25  # 50 - 25
        assert width == 50  # 2 * 25
        assert height == 50  # 2 * 25
    
    def test_rectangle_bounds(self):
        """Test bounding box calculation for rectangle."""
        calculator = BoundingBoxCalculator()
        rect = Rectangle(x=10, y=20, width=30, height=40)
        
        calculator.render(rect)
        bounds = calculator.get_bounding_box()
        
        assert bounds is not None
        x, y, width, height = bounds
        assert x == 10
        assert y == 20
        assert width == 30
        assert height == 40
    
    def test_group_bounds(self):
        """Test bounding box calculation for group of shapes."""
        calculator = BoundingBoxCalculator()
        
        circle = Circle(center=Point2D(x=0, y=0), radius=10)  # bounds: -10 to 10
        rect = Rectangle(x=20, y=30, width=40, height=50)  # bounds: 20-60, 30-80
        
        group = Group().add_child(circle).add_child(rect)
        
        calculator.render(group)
        bounds = calculator.get_bounding_box()
        
        assert bounds is not None
        x, y, width, height = bounds
        assert x == -10  # min of circle and rect
        assert y == -10  # min of circle and rect
        assert width == 70  # from -10 to 60
        assert height == 90  # from -10 to 80
    
    def test_empty_bounds(self):
        """Test bounding box calculation with no shapes."""
        calculator = BoundingBoxCalculator()
        empty_group = Group()
        
        calculator.render(empty_group)
        bounds = calculator.get_bounding_box()
        
        assert bounds is None


class MockRenderer(BaseRenderer):
    """Mock renderer for testing base functionality."""
    
    def __init__(self):
        super().__init__()
        self.visit_calls = []
    
    def begin_render(self):
        self.visit_calls.append("begin")
    
    def end_render(self):
        self.visit_calls.append("end")
    
    def visit_circle(self, circle):
        self.visit_calls.append(f"circle_{int(circle.radius)}")
    
    def visit_rectangle(self, rectangle):
        self.visit_calls.append(f"rect_{int(rectangle.width)}x{int(rectangle.height)}")
    
    def visit_line(self, line):
        self.visit_calls.append("line")
    
    def visit_ellipse(self, ellipse):
        self.visit_calls.append(f"ellipse_{int(ellipse.rx)}x{int(ellipse.ry)}")


class TestBaseRenderer:
    """Test cases for BaseRenderer functionality."""
    
    def test_render_lifecycle(self):
        """Test that render lifecycle methods are called."""
        renderer = MockRenderer()
        circle = Circle(center=Point2D(x=0, y=0), radius=5)
        
        renderer.render(circle)
        
        assert "begin" in renderer.visit_calls
        assert "circle_5" in renderer.visit_calls
        assert "end" in renderer.visit_calls
    
    def test_group_traversal_order(self):
        """Test that group children are visited in correct order."""
        renderer = MockRenderer()
        
        circle = Circle(center=Point2D(x=0, y=0), radius=5)
        rect = Rectangle(x=0, y=0, width=10, height=20)
        
        # Add in specific order
        group = Group().add_child(circle).add_child(rect)
        
        renderer.render(group)
        
        # Should visit children in order they were added
        circle_idx = next(i for i, call in enumerate(renderer.visit_calls) if "circle" in call)
        rect_idx = next(i for i, call in enumerate(renderer.visit_calls) if "rect" in call)
        assert circle_idx < rect_idx
    
    def test_visitor_pattern_dispatch(self):
        """Test that visitor pattern correctly dispatches to right methods."""
        renderer = MockRenderer()
        
        shapes = [
            Circle(center=Point2D(x=0, y=0), radius=1),
            Rectangle(x=0, y=0, width=2, height=3),
            Line(start=Point2D(x=0, y=0), end=Point2D(x=1, y=1)),
            Ellipse(center=Point2D(x=0, y=0), rx=4, ry=5)
        ]
        
        all_calls = []
        for shape in shapes:
            renderer.visit_calls.clear()
            renderer.render(shape)
            all_calls.extend(renderer.visit_calls)
            
        # Check that correct visit methods were called
        assert any("circle_1" in call for call in all_calls)
        assert any("rect_2x3" in call for call in all_calls)
        assert any("line" in call for call in all_calls)
        assert any("ellipse_4x5" in call for call in all_calls)