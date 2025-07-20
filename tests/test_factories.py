"""Tests for shape factory functions."""

import pytest
import math
from claude_draw.factories import (
    create_circle, create_rectangle, create_square, create_ellipse, create_line,
    create_horizontal_line, create_vertical_line, create_circle_from_diameter,
    create_rectangle_from_corners, create_rectangle_from_center, create_ellipse_from_circle
)
from claude_draw.shapes import Circle, Rectangle, Ellipse, Line
from claude_draw.models.point import Point2D
from claude_draw.models.color import Color


class TestBasicFactories:
    """Test basic factory functions."""
    
    def test_create_circle(self):
        """Test create_circle factory."""
        circle = create_circle(10, 20, 5)
        
        assert isinstance(circle, Circle)
        assert circle.center == Point2D(x=10, y=20)
        assert circle.radius == 5
        assert circle.fill is None
        assert circle.stroke is None
        assert circle.stroke_width == 1.0
    
    def test_create_circle_with_style(self):
        """Test create_circle with styling."""
        fill = Color.from_hex("#FF0000")
        stroke = Color.from_hex("#000000")
        
        circle = create_circle(0, 0, 10, fill=fill, stroke=stroke, stroke_width=2.0)
        
        assert circle.fill == fill
        assert circle.stroke == stroke
        assert circle.stroke_width == 2.0
    
    def test_create_circle_validation(self):
        """Test create_circle validation."""
        # Valid circle
        circle = create_circle(0, 0, 5)
        assert circle.radius == 5
        
        # Invalid radius
        with pytest.raises(ValueError, match="Circle radius must be positive"):
            create_circle(0, 0, -5)
        
        with pytest.raises(ValueError, match="Circle radius must be positive"):
            create_circle(0, 0, 0)
    
    def test_create_rectangle(self):
        """Test create_rectangle factory."""
        rect = create_rectangle(10, 20, 30, 40)
        
        assert isinstance(rect, Rectangle)
        assert rect.x == 10
        assert rect.y == 20
        assert rect.width == 30
        assert rect.height == 40
        assert rect.fill is None
        assert rect.stroke is None
        assert rect.stroke_width == 1.0
    
    def test_create_rectangle_with_style(self):
        """Test create_rectangle with styling."""
        fill = Color.from_hex("#00FF00")
        stroke = Color.from_hex("#000000")
        
        rect = create_rectangle(0, 0, 100, 200, fill=fill, stroke=stroke, stroke_width=3.0)
        
        assert rect.fill == fill
        assert rect.stroke == stroke
        assert rect.stroke_width == 3.0
    
    def test_create_rectangle_validation(self):
        """Test create_rectangle validation."""
        # Valid rectangle
        rect = create_rectangle(0, 0, 10, 20)
        assert rect.width == 10
        assert rect.height == 20
        
        # Invalid width
        with pytest.raises(ValueError, match="Rectangle width must be positive"):
            create_rectangle(0, 0, -10, 20)
        
        with pytest.raises(ValueError, match="Rectangle width must be positive"):
            create_rectangle(0, 0, 0, 20)
        
        # Invalid height
        with pytest.raises(ValueError, match="Rectangle height must be positive"):
            create_rectangle(0, 0, 10, -20)
        
        with pytest.raises(ValueError, match="Rectangle height must be positive"):
            create_rectangle(0, 0, 10, 0)
    
    def test_create_square(self):
        """Test create_square factory."""
        square = create_square(10, 20, 30)
        
        assert isinstance(square, Rectangle)
        assert square.x == 10
        assert square.y == 20
        assert square.width == 30
        assert square.height == 30
        assert square.is_square()
    
    def test_create_ellipse(self):
        """Test create_ellipse factory."""
        ellipse = create_ellipse(10, 20, 15, 25)
        
        assert isinstance(ellipse, Ellipse)
        assert ellipse.center == Point2D(x=10, y=20)
        assert ellipse.rx == 15
        assert ellipse.ry == 25
        assert ellipse.fill is None
        assert ellipse.stroke is None
        assert ellipse.stroke_width == 1.0
    
    def test_create_ellipse_validation(self):
        """Test create_ellipse validation."""
        # Valid ellipse
        ellipse = create_ellipse(0, 0, 10, 20)
        assert ellipse.rx == 10
        assert ellipse.ry == 20
        
        # Invalid rx
        with pytest.raises(ValueError, match="Ellipse horizontal radius must be positive"):
            create_ellipse(0, 0, -10, 20)
        
        with pytest.raises(ValueError, match="Ellipse horizontal radius must be positive"):
            create_ellipse(0, 0, 0, 20)
        
        # Invalid ry
        with pytest.raises(ValueError, match="Ellipse vertical radius must be positive"):
            create_ellipse(0, 0, 10, -20)
        
        with pytest.raises(ValueError, match="Ellipse vertical radius must be positive"):
            create_ellipse(0, 0, 10, 0)
    
    def test_create_line(self):
        """Test create_line factory."""
        line = create_line(10, 20, 30, 40)
        
        assert isinstance(line, Line)
        assert line.start == Point2D(x=10, y=20)
        assert line.end == Point2D(x=30, y=40)
        assert line.stroke is None
        assert line.stroke_width == 1.0
    
    def test_create_line_with_style(self):
        """Test create_line with styling."""
        stroke = Color.from_hex("#0000FF")
        
        line = create_line(0, 0, 100, 100, stroke=stroke, stroke_width=5.0)
        
        assert line.stroke == stroke
        assert line.stroke_width == 5.0


class TestSpecializedFactories:
    """Test specialized factory functions."""
    
    def test_create_horizontal_line(self):
        """Test create_horizontal_line factory."""
        line = create_horizontal_line(10, 20, 50)
        
        assert isinstance(line, Line)
        assert line.start == Point2D(x=10, y=20)
        assert line.end == Point2D(x=60, y=20)
        assert line.is_horizontal()
    
    def test_create_horizontal_line_negative_length(self):
        """Test create_horizontal_line with negative length."""
        line = create_horizontal_line(10, 20, -30)
        
        assert line.start == Point2D(x=10, y=20)
        assert line.end == Point2D(x=-20, y=20)
        assert line.is_horizontal()
    
    def test_create_vertical_line(self):
        """Test create_vertical_line factory."""
        line = create_vertical_line(10, 20, 50)
        
        assert isinstance(line, Line)
        assert line.start == Point2D(x=10, y=20)
        assert line.end == Point2D(x=10, y=70)
        assert line.is_vertical()
    
    def test_create_vertical_line_negative_length(self):
        """Test create_vertical_line with negative length."""
        line = create_vertical_line(10, 20, -30)
        
        assert line.start == Point2D(x=10, y=20)
        assert line.end == Point2D(x=10, y=-10)
        assert line.is_vertical()
    
    def test_create_circle_from_diameter(self):
        """Test create_circle_from_diameter factory."""
        circle = create_circle_from_diameter(10, 20, 30)
        
        assert isinstance(circle, Circle)
        assert circle.center == Point2D(x=10, y=20)
        assert circle.radius == 15  # diameter / 2
    
    def test_create_circle_from_diameter_validation(self):
        """Test create_circle_from_diameter validation."""
        # Valid circle
        circle = create_circle_from_diameter(0, 0, 10)
        assert circle.radius == 5
        
        # Invalid diameter
        with pytest.raises(ValueError, match="Circle diameter must be positive"):
            create_circle_from_diameter(0, 0, -10)
        
        with pytest.raises(ValueError, match="Circle diameter must be positive"):
            create_circle_from_diameter(0, 0, 0)
    
    def test_create_rectangle_from_corners(self):
        """Test create_rectangle_from_corners factory."""
        rect = create_rectangle_from_corners(10, 20, 50, 80)
        
        assert isinstance(rect, Rectangle)
        assert rect.x == 10
        assert rect.y == 20
        assert rect.width == 40
        assert rect.height == 60
    
    def test_create_rectangle_from_corners_reversed(self):
        """Test create_rectangle_from_corners with reversed coordinates."""
        rect = create_rectangle_from_corners(50, 80, 10, 20)
        
        assert rect.x == 10  # min x
        assert rect.y == 20  # min y
        assert rect.width == 40
        assert rect.height == 60
    
    def test_create_rectangle_from_corners_validation(self):
        """Test create_rectangle_from_corners validation."""
        # Valid rectangle
        rect = create_rectangle_from_corners(0, 0, 10, 20)
        assert rect.width == 10
        assert rect.height == 20
        
        # Degenerate rectangle (same x coordinates)
        with pytest.raises(ValueError, match="Rectangle corners must define a valid rectangle"):
            create_rectangle_from_corners(10, 0, 10, 20)
        
        # Degenerate rectangle (same y coordinates)
        with pytest.raises(ValueError, match="Rectangle corners must define a valid rectangle"):
            create_rectangle_from_corners(0, 10, 20, 10)
    
    def test_create_rectangle_from_center(self):
        """Test create_rectangle_from_center factory."""
        rect = create_rectangle_from_center(50, 60, 40, 30)
        
        assert isinstance(rect, Rectangle)
        assert rect.x == 30  # center_x - width/2
        assert rect.y == 45  # center_y - height/2
        assert rect.width == 40
        assert rect.height == 30
        assert rect.center == Point2D(x=50, y=60)
    
    def test_create_rectangle_from_center_validation(self):
        """Test create_rectangle_from_center validation."""
        # Valid rectangle
        rect = create_rectangle_from_center(0, 0, 10, 20)
        assert rect.width == 10
        assert rect.height == 20
        
        # Invalid width
        with pytest.raises(ValueError, match="Rectangle width must be positive"):
            create_rectangle_from_center(0, 0, -10, 20)
        
        # Invalid height
        with pytest.raises(ValueError, match="Rectangle height must be positive"):
            create_rectangle_from_center(0, 0, 10, -20)
    
    def test_create_ellipse_from_circle(self):
        """Test create_ellipse_from_circle factory."""
        ellipse = create_ellipse_from_circle(10, 20, 15)
        
        assert isinstance(ellipse, Ellipse)
        assert ellipse.center == Point2D(x=10, y=20)
        assert ellipse.rx == 15
        assert ellipse.ry == 15
        assert ellipse.is_circle()
    
    def test_create_ellipse_from_circle_validation(self):
        """Test create_ellipse_from_circle validation."""
        # Valid ellipse
        ellipse = create_ellipse_from_circle(0, 0, 10)
        assert ellipse.rx == 10
        assert ellipse.ry == 10
        
        # Invalid radius
        with pytest.raises(ValueError, match="Ellipse radius must be positive"):
            create_ellipse_from_circle(0, 0, -10)
        
        with pytest.raises(ValueError, match="Ellipse radius must be positive"):
            create_ellipse_from_circle(0, 0, 0)


class TestFactoryIntegration:
    """Test factory integration with shapes."""
    
    def test_factory_creates_proper_inheritance(self):
        """Test that factory-created shapes have proper inheritance."""
        from claude_draw.base import Primitive
        
        circle = create_circle(0, 0, 10)
        rect = create_rectangle(0, 0, 10, 20)
        ellipse = create_ellipse(0, 0, 10, 20)
        line = create_line(0, 0, 10, 20)
        
        assert isinstance(circle, Primitive)
        assert isinstance(rect, Primitive)
        assert isinstance(ellipse, Primitive)
        assert isinstance(line, Primitive)
    
    def test_factory_creates_immutable_objects(self):
        """Test that factory-created shapes are immutable."""
        from pydantic import ValidationError
        
        circle = create_circle(0, 0, 10)
        rect = create_rectangle(0, 0, 10, 20)
        
        with pytest.raises(ValidationError):
            circle.radius = 20
        
        with pytest.raises(ValidationError):
            rect.width = 30
    
    def test_factory_with_visitor_pattern(self):
        """Test that factory-created shapes work with visitor pattern."""
        class MockVisitor:
            def visit_circle(self, circle):
                return f"circle_{circle.radius}"
            
            def visit_rectangle(self, rectangle):
                return f"rectangle_{rectangle.width}x{rectangle.height}"
            
            def visit_ellipse(self, ellipse):
                return f"ellipse_{ellipse.rx}x{ellipse.ry}"
            
            def visit_line(self, line):
                return f"line_{line.length()}"
        
        visitor = MockVisitor()
        
        circle = create_circle(0, 0, 10)
        rect = create_rectangle(0, 0, 20, 30)
        ellipse = create_ellipse(0, 0, 15, 25)
        line = create_line(0, 0, 3, 4)  # 3-4-5 triangle
        
        assert circle.accept(visitor) == "circle_10.0"
        assert rect.accept(visitor) == "rectangle_20.0x30.0"
        assert ellipse.accept(visitor) == "ellipse_15.0x25.0"
        assert line.accept(visitor) == "line_5.0"
    
    def test_factory_with_transformations(self):
        """Test that factory-created shapes support transformations."""
        circle = create_circle(0, 0, 10)
        rect = create_rectangle(0, 0, 20, 30)
        
        # Test transformations return new instances
        translated_circle = circle.translate(5, 10)
        scaled_rect = rect.scale(2)
        
        assert translated_circle is not circle
        assert scaled_rect is not rect
        
        # Original objects unchanged
        assert circle.center == Point2D(x=0, y=0)
        assert rect.width == 20
        assert rect.height == 30
    
    def test_factory_with_styling_methods(self):
        """Test that factory-created shapes support styling methods."""
        circle = create_circle(0, 0, 10)
        red = Color.from_hex("#FF0000")
        
        styled_circle = circle.with_fill(red).with_opacity(0.5)
        
        assert styled_circle.fill == red
        assert styled_circle.opacity == 0.5
        assert circle.fill is None  # Original unchanged
    
    def test_factory_bounds_calculation(self):
        """Test that factory-created shapes calculate bounds correctly."""
        circle = create_circle(10, 20, 5)
        rect = create_rectangle(0, 0, 30, 40)
        ellipse = create_ellipse(10, 20, 15, 25)
        line = create_line(0, 0, 30, 40)
        
        # Circle bounds
        circle_bounds = circle.get_bounds()
        assert circle_bounds.x == 5
        assert circle_bounds.y == 15
        assert circle_bounds.width == 10
        assert circle_bounds.height == 10
        
        # Rectangle bounds
        rect_bounds = rect.get_bounds()
        assert rect_bounds.x == 0
        assert rect_bounds.y == 0
        assert rect_bounds.width == 30
        assert rect_bounds.height == 40
        
        # Ellipse bounds
        ellipse_bounds = ellipse.get_bounds()
        assert ellipse_bounds.x == -5  # 10 - 15
        assert ellipse_bounds.y == -5  # 20 - 25
        assert ellipse_bounds.width == 30  # 2 * 15
        assert ellipse_bounds.height == 50  # 2 * 25
        
        # Line bounds
        line_bounds = line.get_bounds()
        assert line_bounds.x == 0
        assert line_bounds.y == 0
        assert line_bounds.width == 30
        assert line_bounds.height == 40