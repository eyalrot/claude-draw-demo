"""Tests for shape primitives."""

import pytest
import math
from pydantic import ValidationError

from claude_draw.shapes import Circle, Rectangle, Ellipse, Line
from claude_draw.models.point import Point2D
from claude_draw.models.bounding_box import BoundingBox
from claude_draw.models.color import Color


class TestCircle:
    """Test cases for Circle shape."""
    
    def test_circle_creation(self):
        """Test basic circle creation."""
        center = Point2D(x=10, y=20)
        circle = Circle(center=center, radius=5.0)
        
        assert circle.center == center
        assert circle.radius == 5.0
        assert circle.fill is None
        assert circle.stroke is None
    
    def test_circle_creation_with_style(self):
        """Test circle creation with styling."""
        center = Point2D(x=0, y=0)
        fill = Color.from_hex("#FF0000")
        stroke = Color.from_hex("#000000")
        
        circle = Circle(
            center=center,
            radius=10.0,
            fill=fill,
            stroke=stroke,
            stroke_width=2.0
        )
        
        assert circle.fill == fill
        assert circle.stroke == stroke
        assert circle.stroke_width == 2.0
    
    def test_circle_validation(self):
        """Test circle validation."""
        center = Point2D(x=0, y=0)
        
        # Valid circle
        circle = Circle(center=center, radius=1.0)
        assert circle.radius == 1.0
        
        # Invalid radius (negative)
        with pytest.raises(ValidationError):
            Circle(center=center, radius=-1.0)
        
        # Invalid radius (zero)
        with pytest.raises(ValidationError):
            Circle(center=center, radius=0.0)
    
    def test_circle_bounds(self):
        """Test circle bounding box calculation."""
        center = Point2D(x=10, y=20)
        circle = Circle(center=center, radius=5.0)
        
        bounds = circle.get_bounds()
        expected = BoundingBox(x=5, y=15, width=10, height=10)
        
        assert bounds == expected
    
    def test_circle_with_center(self):
        """Test circle center modification."""
        circle = Circle(center=Point2D(x=0, y=0), radius=5.0)
        new_center = Point2D(x=10, y=20)
        
        new_circle = circle.with_center(new_center)
        
        # Original unchanged
        assert circle.center == Point2D(x=0, y=0)
        
        # New circle has new center
        assert new_circle.center == new_center
        assert new_circle.radius == 5.0
    
    def test_circle_with_radius(self):
        """Test circle radius modification."""
        circle = Circle(center=Point2D(x=0, y=0), radius=5.0)
        
        new_circle = circle.with_radius(10.0)
        
        # Original unchanged
        assert circle.radius == 5.0
        
        # New circle has new radius
        assert new_circle.radius == 10.0
        assert new_circle.center == Point2D(x=0, y=0)
    
    def test_circle_contains_point(self):
        """Test circle point containment."""
        circle = Circle(center=Point2D(x=0, y=0), radius=5.0)
        
        # Point inside
        assert circle.contains_point(Point2D(x=0, y=0))  # Center
        assert circle.contains_point(Point2D(x=3, y=4))  # 3-4-5 triangle
        
        # Point on boundary
        assert circle.contains_point(Point2D(x=5, y=0))  # On edge
        assert circle.contains_point(Point2D(x=0, y=5))  # On edge
        
        # Point outside
        assert not circle.contains_point(Point2D(x=10, y=0))
        assert not circle.contains_point(Point2D(x=0, y=10))
    
    def test_circle_area(self):
        """Test circle area calculation."""
        circle = Circle(center=Point2D(x=0, y=0), radius=5.0)
        
        expected_area = math.pi * 25  # π * r²
        assert abs(circle.area() - expected_area) < 1e-10
    
    def test_circle_circumference(self):
        """Test circle circumference calculation."""
        circle = Circle(center=Point2D(x=0, y=0), radius=5.0)
        
        expected_circumference = 2 * math.pi * 5  # 2 * π * r
        assert abs(circle.circumference() - expected_circumference) < 1e-10
    
    def test_circle_scaled(self):
        """Test circle scaling."""
        circle = Circle(center=Point2D(x=0, y=0), radius=5.0)
        
        scaled = circle.scaled(2.0)
        
        # Original unchanged
        assert circle.radius == 5.0
        
        # Scaled circle has new radius
        assert scaled.radius == 10.0
        assert scaled.center == Point2D(x=0, y=0)
        
        # Invalid scale factor
        with pytest.raises(ValueError):
            circle.scaled(-1.0)
    
    def test_circle_visitor_pattern(self):
        """Test circle visitor pattern."""
        circle = Circle(center=Point2D(x=0, y=0), radius=5.0)
        
        class MockVisitor:
            def visit_circle(self, circle):
                return f"visited_circle_{circle.radius}"
        
        visitor = MockVisitor()
        result = circle.accept(visitor)
        
        assert result == "visited_circle_5.0"


class TestRectangle:
    """Test cases for Rectangle shape."""
    
    def test_rectangle_creation(self):
        """Test basic rectangle creation."""
        rect = Rectangle(x=10, y=20, width=30, height=40)
        
        assert rect.x == 10
        assert rect.y == 20
        assert rect.width == 30
        assert rect.height == 40
    
    def test_rectangle_validation(self):
        """Test rectangle validation."""
        # Valid rectangle
        rect = Rectangle(x=0, y=0, width=10, height=20)
        assert rect.width == 10
        assert rect.height == 20
        
        # Invalid width (negative)
        with pytest.raises(ValidationError):
            Rectangle(x=0, y=0, width=-10, height=20)
        
        # Invalid width (zero)
        with pytest.raises(ValidationError):
            Rectangle(x=0, y=0, width=0, height=20)
        
        # Invalid height (negative)
        with pytest.raises(ValidationError):
            Rectangle(x=0, y=0, width=10, height=-20)
        
        # Invalid height (zero)
        with pytest.raises(ValidationError):
            Rectangle(x=0, y=0, width=10, height=0)
    
    def test_rectangle_bounds(self):
        """Test rectangle bounding box calculation."""
        rect = Rectangle(x=10, y=20, width=30, height=40)
        
        bounds = rect.get_bounds()
        expected = BoundingBox(x=10, y=20, width=30, height=40)
        
        assert bounds == expected
    
    def test_rectangle_properties(self):
        """Test rectangle property calculations."""
        rect = Rectangle(x=10, y=20, width=30, height=40)
        
        assert rect.position == Point2D(x=10, y=20)
        assert rect.center == Point2D(x=25, y=40)
        assert rect.top_left == Point2D(x=10, y=20)
        assert rect.top_right == Point2D(x=40, y=20)
        assert rect.bottom_left == Point2D(x=10, y=60)
        assert rect.bottom_right == Point2D(x=40, y=60)
    
    def test_rectangle_with_position(self):
        """Test rectangle position modification."""
        rect = Rectangle(x=10, y=20, width=30, height=40)
        
        new_rect = rect.with_position(50, 60)
        
        # Original unchanged
        assert rect.x == 10
        assert rect.y == 20
        
        # New rectangle has new position
        assert new_rect.x == 50
        assert new_rect.y == 60
        assert new_rect.width == 30
        assert new_rect.height == 40
    
    def test_rectangle_with_size(self):
        """Test rectangle size modification."""
        rect = Rectangle(x=10, y=20, width=30, height=40)
        
        new_rect = rect.with_size(100, 200)
        
        # Original unchanged
        assert rect.width == 30
        assert rect.height == 40
        
        # New rectangle has new size
        assert new_rect.width == 100
        assert new_rect.height == 200
        assert new_rect.x == 10
        assert new_rect.y == 20
    
    def test_rectangle_contains_point(self):
        """Test rectangle point containment."""
        rect = Rectangle(x=10, y=20, width=30, height=40)
        
        # Points inside
        assert rect.contains_point(Point2D(x=25, y=40))  # Center
        assert rect.contains_point(Point2D(x=15, y=30))  # Inside
        
        # Points on boundary
        assert rect.contains_point(Point2D(x=10, y=20))  # Top-left corner
        assert rect.contains_point(Point2D(x=40, y=60))  # Bottom-right corner
        
        # Points outside
        assert not rect.contains_point(Point2D(x=5, y=15))  # Outside top-left
        assert not rect.contains_point(Point2D(x=45, y=65))  # Outside bottom-right
    
    def test_rectangle_area(self):
        """Test rectangle area calculation."""
        rect = Rectangle(x=10, y=20, width=30, height=40)
        
        assert rect.area() == 1200  # 30 * 40
    
    def test_rectangle_perimeter(self):
        """Test rectangle perimeter calculation."""
        rect = Rectangle(x=10, y=20, width=30, height=40)
        
        assert rect.perimeter() == 140  # 2 * (30 + 40)
    
    def test_rectangle_is_square(self):
        """Test rectangle square detection."""
        rect = Rectangle(x=0, y=0, width=30, height=40)
        assert not rect.is_square()
        
        square = Rectangle(x=0, y=0, width=30, height=30)
        assert square.is_square()
    
    def test_rectangle_visitor_pattern(self):
        """Test rectangle visitor pattern."""
        rect = Rectangle(x=10, y=20, width=30, height=40)
        
        class MockVisitor:
            def visit_rectangle(self, rectangle):
                return f"visited_rectangle_{rectangle.width}x{rectangle.height}"
        
        visitor = MockVisitor()
        result = rect.accept(visitor)
        
        assert result == "visited_rectangle_30.0x40.0"


class TestEllipse:
    """Test cases for Ellipse shape."""
    
    def test_ellipse_creation(self):
        """Test basic ellipse creation."""
        center = Point2D(x=10, y=20)
        ellipse = Ellipse(center=center, rx=5.0, ry=3.0)
        
        assert ellipse.center == center
        assert ellipse.rx == 5.0
        assert ellipse.ry == 3.0
    
    def test_ellipse_validation(self):
        """Test ellipse validation."""
        center = Point2D(x=0, y=0)
        
        # Valid ellipse
        ellipse = Ellipse(center=center, rx=5.0, ry=3.0)
        assert ellipse.rx == 5.0
        assert ellipse.ry == 3.0
        
        # Invalid rx (negative)
        with pytest.raises(ValidationError):
            Ellipse(center=center, rx=-5.0, ry=3.0)
        
        # Invalid rx (zero)
        with pytest.raises(ValidationError):
            Ellipse(center=center, rx=0.0, ry=3.0)
        
        # Invalid ry (negative)
        with pytest.raises(ValidationError):
            Ellipse(center=center, rx=5.0, ry=-3.0)
        
        # Invalid ry (zero)
        with pytest.raises(ValidationError):
            Ellipse(center=center, rx=5.0, ry=0.0)
    
    def test_ellipse_bounds(self):
        """Test ellipse bounding box calculation."""
        center = Point2D(x=10, y=20)
        ellipse = Ellipse(center=center, rx=5.0, ry=3.0)
        
        bounds = ellipse.get_bounds()
        expected = BoundingBox(x=5, y=17, width=10, height=6)
        
        assert bounds == expected
    
    def test_ellipse_with_center(self):
        """Test ellipse center modification."""
        ellipse = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=3.0)
        new_center = Point2D(x=10, y=20)
        
        new_ellipse = ellipse.with_center(new_center)
        
        # Original unchanged
        assert ellipse.center == Point2D(x=0, y=0)
        
        # New ellipse has new center
        assert new_ellipse.center == new_center
        assert new_ellipse.rx == 5.0
        assert new_ellipse.ry == 3.0
    
    def test_ellipse_with_radii(self):
        """Test ellipse radii modification."""
        ellipse = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=3.0)
        
        new_ellipse = ellipse.with_radii(10.0, 8.0)
        
        # Original unchanged
        assert ellipse.rx == 5.0
        assert ellipse.ry == 3.0
        
        # New ellipse has new radii
        assert new_ellipse.rx == 10.0
        assert new_ellipse.ry == 8.0
        assert new_ellipse.center == Point2D(x=0, y=0)
    
    def test_ellipse_contains_point(self):
        """Test ellipse point containment."""
        ellipse = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=3.0)
        
        # Point at center
        assert ellipse.contains_point(Point2D(x=0, y=0))
        
        # Points on axes
        assert ellipse.contains_point(Point2D(x=5, y=0))  # Right edge
        assert ellipse.contains_point(Point2D(x=-5, y=0))  # Left edge
        assert ellipse.contains_point(Point2D(x=0, y=3))  # Top edge
        assert ellipse.contains_point(Point2D(x=0, y=-3))  # Bottom edge
        
        # Points outside
        assert not ellipse.contains_point(Point2D(x=6, y=0))
        assert not ellipse.contains_point(Point2D(x=0, y=4))
    
    def test_ellipse_area(self):
        """Test ellipse area calculation."""
        ellipse = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=3.0)
        
        expected_area = math.pi * 5.0 * 3.0  # π * rx * ry
        assert abs(ellipse.area() - expected_area) < 1e-10
    
    def test_ellipse_perimeter(self):
        """Test ellipse perimeter calculation."""
        ellipse = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=3.0)
        
        # Ramanujan's approximation
        perimeter = ellipse.perimeter()
        assert perimeter > 0
        
        # For a circle (rx = ry), should be close to 2πr
        circle_ellipse = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=5.0)
        expected_circle_perimeter = 2 * math.pi * 5.0
        assert abs(circle_ellipse.perimeter() - expected_circle_perimeter) < 0.1
    
    def test_ellipse_is_circle(self):
        """Test ellipse circle detection."""
        ellipse = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=3.0)
        assert not ellipse.is_circle()
        
        circle = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=5.0)
        assert circle.is_circle()
    
    def test_ellipse_scaled(self):
        """Test ellipse scaling."""
        ellipse = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=3.0)
        
        # Uniform scaling
        scaled = ellipse.scaled(2.0)
        assert scaled.rx == 10.0
        assert scaled.ry == 6.0
        
        # Non-uniform scaling
        scaled_non_uniform = ellipse.scaled(2.0, 3.0)
        assert scaled_non_uniform.rx == 10.0
        assert scaled_non_uniform.ry == 9.0
        
        # Invalid scale factors
        with pytest.raises(ValueError):
            ellipse.scaled(-1.0)
        
        with pytest.raises(ValueError):
            ellipse.scaled(2.0, -1.0)
    
    def test_ellipse_visitor_pattern(self):
        """Test ellipse visitor pattern."""
        ellipse = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=3.0)
        
        class MockVisitor:
            def visit_ellipse(self, ellipse):
                return f"visited_ellipse_{ellipse.rx}x{ellipse.ry}"
        
        visitor = MockVisitor()
        result = ellipse.accept(visitor)
        
        assert result == "visited_ellipse_5.0x3.0"


class TestLine:
    """Test cases for Line shape."""
    
    def test_line_creation(self):
        """Test basic line creation."""
        start = Point2D(x=10, y=20)
        end = Point2D(x=30, y=40)
        line = Line(start=start, end=end)
        
        assert line.start == start
        assert line.end == end
    
    def test_line_bounds(self):
        """Test line bounding box calculation."""
        start = Point2D(x=10, y=20)
        end = Point2D(x=30, y=40)
        line = Line(start=start, end=end)
        
        bounds = line.get_bounds()
        expected = BoundingBox(x=10, y=20, width=20, height=20)
        
        assert bounds == expected
    
    def test_line_bounds_reversed(self):
        """Test line bounding box with reversed coordinates."""
        start = Point2D(x=30, y=40)
        end = Point2D(x=10, y=20)
        line = Line(start=start, end=end)
        
        bounds = line.get_bounds()
        expected = BoundingBox(x=10, y=20, width=20, height=20)
        
        assert bounds == expected
    
    def test_line_with_start(self):
        """Test line start point modification."""
        line = Line(start=Point2D(x=0, y=0), end=Point2D(x=10, y=10))
        new_start = Point2D(x=5, y=5)
        
        new_line = line.with_start(new_start)
        
        # Original unchanged
        assert line.start == Point2D(x=0, y=0)
        
        # New line has new start
        assert new_line.start == new_start
        assert new_line.end == Point2D(x=10, y=10)
    
    def test_line_with_end(self):
        """Test line end point modification."""
        line = Line(start=Point2D(x=0, y=0), end=Point2D(x=10, y=10))
        new_end = Point2D(x=20, y=20)
        
        new_line = line.with_end(new_end)
        
        # Original unchanged
        assert line.end == Point2D(x=10, y=10)
        
        # New line has new end
        assert new_line.end == new_end
        assert new_line.start == Point2D(x=0, y=0)
    
    def test_line_length(self):
        """Test line length calculation."""
        line = Line(start=Point2D(x=0, y=0), end=Point2D(x=3, y=4))
        
        assert line.length() == 5.0  # 3-4-5 triangle
    
    def test_line_midpoint(self):
        """Test line midpoint calculation."""
        line = Line(start=Point2D(x=0, y=0), end=Point2D(x=10, y=20))
        
        midpoint = line.midpoint()
        expected = Point2D(x=5, y=10)
        
        assert midpoint == expected
    
    def test_line_slope(self):
        """Test line slope calculation."""
        line = Line(start=Point2D(x=0, y=0), end=Point2D(x=10, y=20))
        
        assert line.slope() == 2.0  # rise/run = 20/10 = 2
        
        # Vertical line should raise error
        vertical_line = Line(start=Point2D(x=0, y=0), end=Point2D(x=0, y=10))
        with pytest.raises(ValueError):
            vertical_line.slope()
    
    def test_line_angle(self):
        """Test line angle calculation."""
        line = Line(start=Point2D(x=0, y=0), end=Point2D(x=1, y=1))
        
        expected_angle = math.pi / 4  # 45 degrees
        assert abs(line.angle() - expected_angle) < 1e-10
    
    def test_line_orientation_checks(self):
        """Test line orientation check methods."""
        horizontal = Line(start=Point2D(x=0, y=5), end=Point2D(x=10, y=5))
        assert horizontal.is_horizontal()
        assert not horizontal.is_vertical()
        
        vertical = Line(start=Point2D(x=5, y=0), end=Point2D(x=5, y=10))
        assert vertical.is_vertical()
        assert not vertical.is_horizontal()
        
        diagonal = Line(start=Point2D(x=0, y=0), end=Point2D(x=10, y=10))
        assert not diagonal.is_horizontal()
        assert not diagonal.is_vertical()
    
    def test_line_is_point(self):
        """Test line point detection."""
        line = Line(start=Point2D(x=0, y=0), end=Point2D(x=10, y=10))
        assert not line.is_point()
        
        point = Line(start=Point2D(x=5, y=5), end=Point2D(x=5, y=5))
        assert point.is_point()
    
    def test_line_visitor_pattern(self):
        """Test line visitor pattern."""
        line = Line(start=Point2D(x=0, y=0), end=Point2D(x=10, y=10))
        
        class MockVisitor:
            def visit_line(self, line):
                return f"visited_line_{line.length()}"
        
        visitor = MockVisitor()
        result = line.accept(visitor)
        
        expected_length = math.sqrt(200)  # sqrt(10² + 10²)
        assert result == f"visited_line_{expected_length}"


class TestShapeImmutability:
    """Test immutability of all shape classes."""
    
    def test_circle_immutability(self):
        """Test that Circle is immutable."""
        from pydantic import ValidationError
        
        circle = Circle(center=Point2D(x=0, y=0), radius=5.0)
        
        with pytest.raises(ValidationError):
            circle.radius = 10.0
        
        with pytest.raises(ValidationError):
            circle.center = Point2D(x=10, y=10)
    
    def test_rectangle_immutability(self):
        """Test that Rectangle is immutable."""
        from pydantic import ValidationError
        
        rect = Rectangle(x=0, y=0, width=10, height=20)
        
        with pytest.raises(ValidationError):
            rect.width = 30
        
        with pytest.raises(ValidationError):
            rect.x = 10
    
    def test_ellipse_immutability(self):
        """Test that Ellipse is immutable."""
        from pydantic import ValidationError
        
        ellipse = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=3.0)
        
        with pytest.raises(ValidationError):
            ellipse.rx = 10.0
        
        with pytest.raises(ValidationError):
            ellipse.center = Point2D(x=10, y=10)
    
    def test_line_immutability(self):
        """Test that Line is immutable."""
        from pydantic import ValidationError
        
        line = Line(start=Point2D(x=0, y=0), end=Point2D(x=10, y=10))
        
        with pytest.raises(ValidationError):
            line.start = Point2D(x=5, y=5)
        
        with pytest.raises(ValidationError):
            line.end = Point2D(x=20, y=20)


class TestShapeInheritance:
    """Test that all shapes properly inherit from base classes."""
    
    def test_shapes_inherit_from_primitive(self):
        """Test that all shapes inherit from Primitive."""
        from claude_draw.base import Primitive
        
        circle = Circle(center=Point2D(x=0, y=0), radius=5.0)
        rect = Rectangle(x=0, y=0, width=10, height=20)
        ellipse = Ellipse(center=Point2D(x=0, y=0), rx=5.0, ry=3.0)
        line = Line(start=Point2D(x=0, y=0), end=Point2D(x=10, y=10))
        
        assert isinstance(circle, Primitive)
        assert isinstance(rect, Primitive)
        assert isinstance(ellipse, Primitive)
        assert isinstance(line, Primitive)
    
    def test_shapes_have_styling_capabilities(self):
        """Test that shapes have styling capabilities from StyleMixin."""
        circle = Circle(center=Point2D(x=0, y=0), radius=5.0)
        
        # Should have styling properties
        assert hasattr(circle, 'fill')
        assert hasattr(circle, 'stroke')
        assert hasattr(circle, 'opacity')
        
        # Should have styling methods
        assert hasattr(circle, 'with_fill')
        assert hasattr(circle, 'with_stroke')
        assert hasattr(circle, 'with_opacity')
        
        # Test styling works
        red = Color.from_hex("#FF0000")
        styled_circle = circle.with_fill(red).with_opacity(0.5)
        
        assert styled_circle.fill == red
        assert styled_circle.opacity == 0.5
    
    def test_shapes_have_transformation_capabilities(self):
        """Test that shapes have transformation capabilities from Drawable."""
        circle = Circle(center=Point2D(x=0, y=0), radius=5.0)
        
        # Should have transformation methods
        assert hasattr(circle, 'translate')
        assert hasattr(circle, 'rotate')
        assert hasattr(circle, 'scale')
        
        # Test transformation works
        translated = circle.translate(10, 20)
        
        # Should be a new instance
        assert translated is not circle
        assert translated.center == Point2D(x=0, y=0)  # Center unchanged (transform applied separately)
        assert translated.transform != circle.transform