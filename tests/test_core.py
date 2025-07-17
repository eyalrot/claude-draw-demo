"""Tests for core Claude Draw functionality."""

from claude_draw import Canvas, Circle, Rectangle, __version__


def test_version():
    """Test that version is set correctly."""
    assert __version__ == "0.1.0"


def test_circle_creation():
    """Test creating a circle."""
    circle = Circle(x=100, y=100, radius=50, fill="blue")
    assert circle.x == 100
    assert circle.y == 100
    assert circle.radius == 50
    assert circle.fill == "blue"


def test_rectangle_creation():
    """Test creating a rectangle."""
    rect = Rectangle(x=10, y=20, width=100, height=50, fill="red")
    assert rect.x == 10
    assert rect.y == 20
    assert rect.width == 100
    assert rect.height == 50
    assert rect.fill == "red"


def test_canvas_creation():
    """Test creating a canvas."""
    canvas = Canvas(width=400, height=300)
    assert canvas.width == 400
    assert canvas.height == 300
    assert canvas.background == "white"
    assert len(canvas.shapes) == 0


def test_canvas_add_shapes():
    """Test adding shapes to canvas."""
    canvas = Canvas()
    circle = Circle(x=200, y=150, radius=50, fill="blue")
    rect = Rectangle(x=50, y=50, width=100, height=80, fill="red")

    canvas.add(circle)
    canvas.add(rect)

    assert len(canvas.shapes) == 2
    assert canvas.shapes[0] == circle
    assert canvas.shapes[1] == rect


def test_circle_to_svg():
    """Test circle SVG generation."""
    circle = Circle(
        x=100, y=100, radius=50, fill="blue", stroke="black", stroke_width=2
    )
    svg = circle.to_svg()
    assert 'cx="100"' in svg
    assert 'cy="100"' in svg
    assert 'r="50"' in svg
    assert 'fill="blue"' in svg
    assert 'stroke="black"' in svg
    assert 'stroke-width="2"' in svg


def test_rectangle_to_svg():
    """Test rectangle SVG generation."""
    rect = Rectangle(x=10, y=20, width=100, height=50, fill="red")
    svg = rect.to_svg()
    assert 'x="10"' in svg
    assert 'y="20"' in svg
    assert 'width="100"' in svg
    assert 'height="50"' in svg
    assert 'fill="red"' in svg


def test_canvas_to_svg():
    """Test canvas SVG generation."""
    canvas = Canvas(width=400, height=300)
    circle = Circle(x=200, y=150, radius=50, fill="blue")
    canvas.add(circle)

    svg = canvas.to_svg()
    assert '<svg width="400" height="300"' in svg
    assert 'xmlns="http://www.w3.org/2000/svg"' in svg
    assert "<circle" in svg
    assert "</svg>" in svg
