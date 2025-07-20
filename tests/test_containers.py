"""Tests for container implementations."""

import pytest
from claude_draw.containers import Group, Layer, Drawing, BlendMode
from claude_draw.shapes import Circle, Rectangle
from claude_draw.models.point import Point2D
from claude_draw.models.color import Color
from claude_draw.models.bounding_box import BoundingBox


class TestGroup:
    """Test cases for Group container."""
    
    def test_group_creation(self):
        """Test basic group creation."""
        group = Group()
        assert group.children == []
        assert group.name is None
        assert group.z_index == 0
        assert group.id is not None
    
    def test_group_with_name(self):
        """Test group creation with name."""
        group = Group(name="test_group")
        assert group.name == "test_group"
    
    def test_add_child(self):
        """Test adding children to group."""
        group = Group()
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        
        new_group = group.add_child(circle)
        assert len(new_group.children) == 1
        assert new_group.children[0] == circle
        assert len(group.children) == 0  # Original unchanged
    
    def test_add_multiple_children(self):
        """Test adding multiple children."""
        group = Group()
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        rect = Rectangle(x=0, y=0, width=20, height=15)
        
        group = group.add_child(circle)
        group = group.add_child(rect)
        
        assert len(group.children) == 2
        assert circle in group.children
        assert rect in group.children
    
    def test_remove_child(self):
        """Test removing child from group."""
        group = Group()
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        rect = Rectangle(x=0, y=0, width=20, height=15)
        
        group = group.add_child(circle).add_child(rect)
        new_group = group.remove_child(circle.id)
        
        assert len(new_group.children) == 1
        assert new_group.children[0] == rect
        assert len(group.children) == 2  # Original unchanged
    
    def test_remove_nonexistent_child(self):
        """Test removing non-existent child."""
        group = Group()
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        group = group.add_child(circle)
        
        new_group = group.remove_child("nonexistent")
        assert len(new_group.children) == 1  # No change
    
    def test_with_name(self):
        """Test updating group name."""
        group = Group()
        new_group = group.with_name("new_name")
        
        assert new_group.name == "new_name"
        assert group.name is None  # Original unchanged
    
    def test_with_z_index(self):
        """Test updating z-index."""
        group = Group()
        new_group = group.with_z_index(5)
        
        assert new_group.z_index == 5
        assert group.z_index == 0  # Original unchanged
    
    def test_get_children_sorted(self):
        """Test getting children sorted by z-index."""
        group = Group()
        # Note: Basic shapes don't have z_index, so this tests the fallback to 0
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        rect = Rectangle(x=0, y=0, width=20, height=15)
        
        group = group.add_child(circle).add_child(rect)
        sorted_children = group.get_children_sorted()
        
        assert len(sorted_children) == 2
        # Since both have z_index=0 (default), order should be maintained
        assert sorted_children == group.children
    
    def test_get_bounds_empty(self):
        """Test bounds calculation for empty group."""
        group = Group()
        bounds = group.get_bounds()
        
        assert bounds.x == 0
        assert bounds.y == 0
        assert bounds.width == 0
        assert bounds.height == 0
    
    def test_get_bounds_with_children(self):
        """Test bounds calculation with children."""
        group = Group()
        circle = Circle(center=Point2D(x=10, y=10), radius=5)  # bounds: (5,5) to (15,15)
        rect = Rectangle(x=20, y=20, width=10, height=10)  # bounds: (20,20) to (30,30)
        
        group = group.add_child(circle).add_child(rect)
        bounds = group.get_bounds()
        
        # Should encompass both shapes
        assert bounds.x == 5  # min x
        assert bounds.y == 5  # min y
        assert bounds.width == 25  # max_x - min_x = 30 - 5
        assert bounds.height == 25  # max_y - min_y = 30 - 5


class TestLayer:
    """Test cases for Layer container."""
    
    def test_layer_creation(self):
        """Test basic layer creation."""
        layer = Layer()
        assert layer.children == []
        assert layer.name is None
        assert layer.opacity == 1.0
        assert layer.blend_mode == BlendMode.NORMAL
        assert layer.visible is True
        assert layer.locked is False
        assert layer.z_index == 0
    
    def test_layer_with_properties(self):
        """Test layer creation with properties."""
        layer = Layer(
            name="test_layer",
            opacity=0.5,
            blend_mode=BlendMode.MULTIPLY,
            visible=False,
            locked=True,
            z_index=10
        )
        assert layer.name == "test_layer"
        assert layer.opacity == 0.5
        assert layer.blend_mode == BlendMode.MULTIPLY
        assert layer.visible is False
        assert layer.locked is True
        assert layer.z_index == 10
    
    def test_with_opacity(self):
        """Test updating layer opacity."""
        layer = Layer()
        new_layer = layer.with_opacity(0.7)
        
        assert new_layer.opacity == 0.7
        assert layer.opacity == 1.0  # Original unchanged
    
    def test_with_blend_mode(self):
        """Test updating blend mode."""
        layer = Layer()
        new_layer = layer.with_blend_mode(BlendMode.SCREEN)
        
        assert new_layer.blend_mode == BlendMode.SCREEN
        assert layer.blend_mode == BlendMode.NORMAL  # Original unchanged
    
    def test_with_visibility(self):
        """Test updating visibility."""
        layer = Layer()
        new_layer = layer.with_visibility(False)
        
        assert new_layer.visible is False
        assert layer.visible is True  # Original unchanged
    
    def test_with_locked(self):
        """Test updating lock state."""
        layer = Layer()
        new_layer = layer.with_locked(True)
        
        assert new_layer.locked is True
        assert layer.locked is False  # Original unchanged
    
    def test_is_editable(self):
        """Test editability check."""
        layer = Layer()
        assert layer.is_editable() is True  # Not locked and visible
        
        layer = layer.with_locked(True)
        assert layer.is_editable() is False  # Locked
        
        layer = layer.with_locked(False).with_visibility(False)
        assert layer.is_editable() is False  # Not visible
        
        layer = layer.with_locked(True).with_visibility(False)
        assert layer.is_editable() is False  # Both locked and not visible
    
    def test_opacity_validation(self):
        """Test opacity validation."""
        with pytest.raises(ValueError):
            Layer(opacity=-0.1)  # Below 0
        
        with pytest.raises(ValueError):
            Layer(opacity=1.1)  # Above 1


class TestDrawing:
    """Test cases for Drawing container."""
    
    def test_drawing_creation(self):
        """Test basic drawing creation."""
        drawing = Drawing()
        assert drawing.children == []
        assert drawing.width == 800.0
        assert drawing.height == 600.0
        assert drawing.title is None
        assert drawing.description is None
        assert drawing.background_color is None
    
    def test_drawing_with_properties(self):
        """Test drawing creation with properties."""
        drawing = Drawing(
            width=1024,
            height=768,
            title="Test Drawing",
            description="A test drawing",
            background_color="#ffffff"
        )
        assert drawing.width == 1024
        assert drawing.height == 768
        assert drawing.title == "Test Drawing"
        assert drawing.description == "A test drawing"
        assert drawing.background_color == "#ffffff"
    
    def test_with_dimensions(self):
        """Test updating drawing dimensions."""
        drawing = Drawing()
        new_drawing = drawing.with_dimensions(1200, 900)
        
        assert new_drawing.width == 1200
        assert new_drawing.height == 900
        assert drawing.width == 800.0  # Original unchanged
        assert drawing.height == 600.0
    
    def test_with_title(self):
        """Test updating title."""
        drawing = Drawing()
        new_drawing = drawing.with_title("New Title")
        
        assert new_drawing.title == "New Title"
        assert drawing.title is None  # Original unchanged
    
    def test_with_description(self):
        """Test updating description."""
        drawing = Drawing()
        new_drawing = drawing.with_description("New description")
        
        assert new_drawing.description == "New description"
        assert drawing.description is None  # Original unchanged
    
    def test_with_background_color(self):
        """Test updating background color."""
        drawing = Drawing()
        new_drawing = drawing.with_background_color("#ff0000")
        
        assert new_drawing.background_color == "#ff0000"
        assert drawing.background_color is None  # Original unchanged
    
    def test_get_canvas_bounds(self):
        """Test canvas bounds calculation."""
        drawing = Drawing(width=1024, height=768)
        bounds = drawing.get_canvas_bounds()
        
        assert bounds.x == 0
        assert bounds.y == 0
        assert bounds.width == 1024
        assert bounds.height == 768
    
    def test_get_layers(self):
        """Test getting child layers."""
        drawing = Drawing()
        layer1 = Layer(name="layer1")
        layer2 = Layer(name="layer2")
        group = Group(name="group1")
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        
        drawing = drawing.add_child(layer1).add_child(group).add_child(layer2).add_child(circle)
        layers = drawing.get_layers()
        
        assert len(layers) == 2
        assert layer1 in layers
        assert layer2 in layers
        assert group not in layers
        assert circle not in layers
    
    def test_get_groups(self):
        """Test getting child groups."""
        drawing = Drawing()
        layer = Layer(name="layer1")
        group1 = Group(name="group1")
        group2 = Group(name="group2")
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        
        drawing = drawing.add_child(layer).add_child(group1).add_child(group2).add_child(circle)
        groups = drawing.get_groups()
        
        assert len(groups) == 2
        assert group1 in groups
        assert group2 in groups
        assert layer not in groups
        assert circle not in groups
    
    def test_get_layers_sorted(self):
        """Test getting layers sorted by z-index."""
        drawing = Drawing()
        layer1 = Layer(name="layer1", z_index=5)
        layer2 = Layer(name="layer2", z_index=1)
        layer3 = Layer(name="layer3", z_index=10)
        
        drawing = drawing.add_child(layer1).add_child(layer2).add_child(layer3)
        sorted_layers = drawing.get_layers_sorted()
        
        assert len(sorted_layers) == 3
        assert sorted_layers[0] == layer2  # z_index=1
        assert sorted_layers[1] == layer1  # z_index=5
        assert sorted_layers[2] == layer3  # z_index=10
    
    def test_dimensions_validation(self):
        """Test dimension validation."""
        with pytest.raises(ValueError):
            Drawing(width=0)  # Zero width
        
        with pytest.raises(ValueError):
            Drawing(height=-10)  # Negative height


class TestBlendMode:
    """Test cases for BlendMode enum."""
    
    def test_blend_mode_values(self):
        """Test blend mode enum values."""
        assert BlendMode.NORMAL == "normal"
        assert BlendMode.MULTIPLY == "multiply"
        assert BlendMode.SCREEN == "screen"
        assert BlendMode.OVERLAY == "overlay"
        assert BlendMode.DARKEN == "darken"
        assert BlendMode.LIGHTEN == "lighten"
        assert BlendMode.COLOR_DODGE == "color-dodge"
        assert BlendMode.COLOR_BURN == "color-burn"
        assert BlendMode.HARD_LIGHT == "hard-light"
        assert BlendMode.SOFT_LIGHT == "soft-light"
        assert BlendMode.DIFFERENCE == "difference"
        assert BlendMode.EXCLUSION == "exclusion"


class TestContainerHierarchy:
    """Test cases for container hierarchies and nesting."""
    
    def test_nested_groups(self):
        """Test nesting groups within groups."""
        parent_group = Group(name="parent")
        child_group = Group(name="child")
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        
        child_group = child_group.add_child(circle)
        parent_group = parent_group.add_child(child_group)
        
        assert len(parent_group.children) == 1
        assert parent_group.children[0] == child_group
        assert len(child_group.children) == 1
        assert child_group.children[0] == circle
    
    def test_layer_with_groups(self):
        """Test layer containing groups."""
        layer = Layer(name="main_layer")
        group = Group(name="group_in_layer")
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        
        group = group.add_child(circle)
        layer = layer.add_child(group)
        
        assert len(layer.children) == 1
        assert layer.children[0] == group
        assert len(group.children) == 1
    
    def test_drawing_with_layers_and_groups(self):
        """Test complete hierarchy: Drawing -> Layers -> Groups -> Shapes."""
        drawing = Drawing(title="Complex Drawing")
        
        # Create layer 1 with a group
        layer1 = Layer(name="background", z_index=1)
        bg_group = Group(name="background_shapes")
        bg_rect = Rectangle(x=0, y=0, width=800, height=600)
        bg_group = bg_group.add_child(bg_rect)
        layer1 = layer1.add_child(bg_group)
        
        # Create layer 2 with direct shapes
        layer2 = Layer(name="foreground", z_index=2)
        circle = Circle(center=Point2D(x=100, y=100), radius=50)
        layer2 = layer2.add_child(circle)
        
        # Add layers to drawing
        drawing = drawing.add_child(layer1).add_child(layer2)
        
        assert len(drawing.children) == 2
        sorted_layers = drawing.get_layers_sorted()
        assert sorted_layers[0].name == "background"
        assert sorted_layers[1].name == "foreground"
    
    def test_immutability(self):
        """Test that containers are immutable."""
        group = Group(name="original")
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        
        new_group = group.add_child(circle)
        
        # Original should be unchanged
        assert len(group.children) == 0
        assert group.name == "original"
        
        # New instance should have changes
        assert len(new_group.children) == 1
        assert new_group.name == "original"  # Name should be copied
        
        # Test name change doesn't affect original
        renamed_group = new_group.with_name("changed")
        assert new_group.name == "original"
        assert renamed_group.name == "changed"