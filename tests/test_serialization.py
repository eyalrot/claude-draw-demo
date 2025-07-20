"""Tests for serialization functionality."""

import json
import pytest
import tempfile
import os
from typing import Dict, Any

from claude_draw.serialization import (
    serialize_drawable, deserialize_drawable, deserialize_with_references,
    load_drawable, save_drawable, register_drawable_type, 
    get_drawable_class, get_type_discriminator,
    EnhancedJSONEncoder, SerializationRegistry
)
from claude_draw.shapes import Circle, Rectangle, Line, Ellipse
from claude_draw.containers import Group, Layer, Drawing
from claude_draw.models.point import Point2D
from claude_draw.models.color import Color
from claude_draw.models.transform import Transform2D


class TestSerializationRegistry:
    """Test cases for SerializationRegistry."""
    
    def test_register_and_get_class(self):
        """Test registering and retrieving classes."""
        registry = SerializationRegistry()
        
        registry.register("TestCircle", Circle)
        assert registry.get_class("TestCircle") is Circle
        assert registry.get_type_name(Circle) == "TestCircle"
    
    def test_get_nonexistent_class(self):
        """Test getting non-existent class returns None."""
        registry = SerializationRegistry()
        assert registry.get_class("NonExistent") is None
    
    def test_get_all_types(self):
        """Test getting all registered types."""
        registry = SerializationRegistry()
        registry.register("TestCircle", Circle)
        registry.register("TestRect", Rectangle)
        
        all_types = registry.get_all_types()
        assert "TestCircle" in all_types
        assert "TestRect" in all_types
        assert all_types["TestCircle"] is Circle
        assert all_types["TestRect"] is Rectangle


class TestEnhancedJSONEncoder:
    """Test cases for EnhancedJSONEncoder."""
    
    def test_serialize_circle(self):
        """Test serializing a circle with type discriminator."""
        circle = Circle(center=Point2D(x=50, y=50), radius=25)
        encoder = EnhancedJSONEncoder()
        
        serialized = encoder._serialize_draw_model(circle)
        
        assert "__type__" in serialized
        assert serialized["__type__"] == "Circle"
        assert "__version__" in serialized
        assert serialized["center"]["x"] == 50
        assert serialized["center"]["y"] == 50
        assert serialized["radius"] == 25
    
    def test_serialize_without_version(self):
        """Test serializing without version information."""
        circle = Circle(center=Point2D(x=0, y=0), radius=10)
        encoder = EnhancedJSONEncoder(include_version=False)
        
        serialized = encoder._serialize_draw_model(circle)
        
        assert "__version__" not in serialized
        assert "__type__" in serialized
    
    def test_serialize_with_style(self):
        """Test serializing object with style properties."""
        circle = Circle(
            center=Point2D(x=0, y=0), 
            radius=10,
            fill=Color.from_hex("#FF0000"),
            stroke=Color.from_hex("#000000"),
            stroke_width=2.0,
            opacity=0.8
        )
        encoder = EnhancedJSONEncoder()
        
        serialized = encoder._serialize_draw_model(circle)
        
        assert serialized["fill"]["r"] == 255
        assert serialized["fill"]["g"] == 0
        assert serialized["fill"]["b"] == 0
        assert serialized["stroke"]["r"] == 0
        assert serialized["stroke"]["g"] == 0
        assert serialized["stroke"]["b"] == 0 
        assert serialized["stroke_width"] == 2.0
        assert serialized["opacity"] == 0.8


class TestBasicSerialization:
    """Test cases for basic serialization/deserialization."""
    
    def test_circle_round_trip(self):
        """Test circle serialization round trip."""
        original = Circle(
            center=Point2D(x=100, y=200), 
            radius=50,
            fill=Color.from_hex("#FF0000")
        )
        
        # Serialize
        json_str = serialize_drawable(original)
        assert isinstance(json_str, str)
        
        # Deserialize
        restored = deserialize_drawable(json_str)
        
        # Verify
        assert isinstance(restored, Circle)
        assert restored.center.x == 100
        assert restored.center.y == 200
        assert restored.radius == 50
        assert restored.fill.to_hex() == "#FF0000"
    
    def test_rectangle_round_trip(self):
        """Test rectangle serialization round trip."""
        original = Rectangle(
            x=10, y=20, width=30, height=40,
            stroke=Color.from_hex("#000000"),
            stroke_width=3.0
        )
        
        json_str = serialize_drawable(original)
        restored = deserialize_drawable(json_str)
        
        assert isinstance(restored, Rectangle)
        assert restored.x == 10
        assert restored.y == 20
        assert restored.width == 30
        assert restored.height == 40
        assert restored.stroke.to_hex() == "#000000"
        assert restored.stroke_width == 3.0
    
    def test_line_round_trip(self):
        """Test line serialization round trip."""
        original = Line(
            start=Point2D(x=0, y=0),
            end=Point2D(x=100, y=100),
            stroke=Color.from_hex("#0000FF")
        )
        
        json_str = serialize_drawable(original)
        restored = deserialize_drawable(json_str)
        
        assert isinstance(restored, Line)
        assert restored.start.x == 0
        assert restored.start.y == 0
        assert restored.end.x == 100
        assert restored.end.y == 100
        assert restored.stroke.to_hex() == "#0000FF"
    
    def test_ellipse_round_trip(self):
        """Test ellipse serialization round trip."""
        original = Ellipse(
            center=Point2D(x=50, y=75),
            rx=30, ry=20,
            fill=Color.from_hex("#00FF00"),
            opacity=0.6
        )
        
        json_str = serialize_drawable(original)
        restored = deserialize_drawable(json_str)
        
        assert isinstance(restored, Ellipse)
        assert restored.center.x == 50
        assert restored.center.y == 75
        assert restored.rx == 30
        assert restored.ry == 20
        assert restored.fill.to_hex() == "#00FF00"
        assert restored.opacity == 0.6


class TestContainerSerialization:
    """Test cases for container serialization."""
    
    def test_group_round_trip(self):
        """Test group serialization round trip."""
        circle = Circle(center=Point2D(x=25, y=25), radius=10)
        rect = Rectangle(x=0, y=0, width=50, height=50)
        
        original = Group(name="test_group").add_child(circle).add_child(rect)
        
        json_str = serialize_drawable(original)
        restored = deserialize_drawable(json_str)
        
        assert isinstance(restored, Group)
        assert restored.name == "test_group"
        assert len(restored.children) == 2
        
        # Check children
        child_types = [type(child).__name__ for child in restored.children]
        assert "Circle" in child_types
        assert "Rectangle" in child_types
    
    def test_layer_round_trip(self):
        """Test layer serialization round trip."""
        circle = Circle(center=Point2D(x=10, y=10), radius=5)
        
        original = Layer(
            name="test_layer",
            opacity=0.7,
            visible=True,
            z_index=5
        ).add_child(circle)
        
        json_str = serialize_drawable(original)
        restored = deserialize_drawable(json_str)
        
        assert isinstance(restored, Layer)
        assert restored.name == "test_layer"
        assert restored.opacity == 0.7
        assert restored.visible is True
        assert restored.z_index == 5
        assert len(restored.children) == 1
        assert isinstance(restored.children[0], Circle)
    
    def test_drawing_round_trip(self):
        """Test drawing serialization round trip."""
        circle = Circle(center=Point2D(x=50, y=50), radius=25)
        layer = Layer(name="layer1").add_child(circle)
        
        original = Drawing(
            width=200, height=150,
            title="Test Drawing",
            background_color="#F0F0F0"
        ).add_child(layer)
        
        json_str = serialize_drawable(original)
        restored = deserialize_drawable(json_str)
        
        assert isinstance(restored, Drawing)
        assert restored.width == 200
        assert restored.height == 150
        assert restored.title == "Test Drawing"
        assert restored.background_color == "#F0F0F0"
        assert len(restored.children) == 1
        assert isinstance(restored.children[0], Layer)


class TestTransformSerialization:
    """Test cases for transformation serialization."""
    
    def test_transformed_shape_round_trip(self):
        """Test serialization of transformed shapes."""
        transform = Transform2D().translate(10, 20).scale(2.0)
        
        original = Circle(
            center=Point2D(x=0, y=0),
            radius=10,
            transform=transform
        )
        
        json_str = serialize_drawable(original)
        restored = deserialize_drawable(json_str)
        
        assert isinstance(restored, Circle)
        assert restored.transform.tx == 10
        assert restored.transform.ty == 20
        assert restored.transform.a == 2.0
        assert restored.transform.d == 2.0


class TestErrorHandling:
    """Test cases for error handling."""
    
    def test_deserialize_missing_type(self):
        """Test deserializing data without type discriminator."""
        data = {"center": {"x": 0, "y": 0}, "radius": 10}
        
        with pytest.raises(ValueError, match="Missing type discriminator"):
            deserialize_drawable(data)
    
    def test_deserialize_unknown_type(self):
        """Test deserializing data with unknown type."""
        data = {
            "__type__": "UnknownShape",
            "center": {"x": 0, "y": 0},
            "radius": 10
        }
        
        with pytest.raises(ValueError, match="Unknown type discriminator"):
            deserialize_drawable(data)
    
    def test_deserialize_invalid_data_type(self):
        """Test deserializing invalid data type."""
        with pytest.raises((TypeError, json.JSONDecodeError)):
            deserialize_drawable("invalid")
    
    def test_deserialize_reference_without_context(self):
        """Test deserializing reference without proper context."""
        data = {"__ref__": "obj_1"}
        
        with pytest.raises(ValueError, match="Cannot deserialize reference without context"):
            deserialize_drawable(data)


class TestFileSerialization:
    """Test cases for file-based serialization."""
    
    def test_save_and_load_drawable(self):
        """Test saving and loading drawable from file."""
        original = Circle(
            center=Point2D(x=50, y=50),
            radius=25,
            fill=Color.from_hex("#FF0000")
        )
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            filename = f.name
        
        try:
            # Save
            save_drawable(original, filename)
            assert os.path.exists(filename)
            
            # Load
            restored = load_drawable(filename)
            
            # Verify
            assert isinstance(restored, Circle)
            assert restored.center.x == 50
            assert restored.center.y == 50
            assert restored.radius == 25
            assert restored.fill.to_hex() == "#FF0000"
            
        finally:
            # Cleanup
            if os.path.exists(filename):
                os.unlink(filename)
    
    def test_save_complex_drawing(self):
        """Test saving and loading complex drawing."""
        # Create a complex drawing
        circle = Circle(center=Point2D(x=25, y=25), radius=10, fill=Color.from_hex("#FF0000"))
        rect = Rectangle(x=10, y=10, width=30, height=20, stroke=Color.from_hex("#000000"))
        
        group = Group(name="shapes").add_child(circle).add_child(rect)
        layer = Layer(name="main_layer", opacity=0.8).add_child(group)
        
        drawing = Drawing(
            width=100, height=100,
            title="Complex Drawing"
        ).add_child(layer)
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            filename = f.name
        
        try:
            # Save
            save_drawable(drawing, filename)
            
            # Verify file contents
            with open(filename, 'r') as f:
                data = json.load(f)
            
            assert data["__type__"] == "Drawing"
            assert data["title"] == "Complex Drawing"
            assert len(data["children"]) == 1
            
            # Load
            restored = load_drawable(filename)
            
            # Verify structure
            assert isinstance(restored, Drawing)
            assert restored.title == "Complex Drawing"
            assert len(restored.children) == 1
            
            layer = restored.children[0]
            assert isinstance(layer, Layer)
            assert layer.name == "main_layer"
            assert layer.opacity == 0.8
            
            group = layer.children[0]
            assert isinstance(group, Group)
            assert group.name == "shapes"
            assert len(group.children) == 2
            
        finally:
            if os.path.exists(filename):
                os.unlink(filename)


class TestPolymorphicDeserialization:
    """Test cases for polymorphic deserialization."""
    
    def test_mixed_shape_collection(self):
        """Test deserializing collection with mixed shape types."""
        shapes_data = [
            {
                "__type__": "Circle",
                "center": {"x": 0, "y": 0},
                "radius": 10
            },
            {
                "__type__": "Rectangle", 
                "x": 10, "y": 20, "width": 30, "height": 40
            },
            {
                "__type__": "Line",
                "start": {"x": 0, "y": 0},
                "end": {"x": 100, "y": 100}
            }
        ]
        
        # Deserialize each shape
        shapes = [deserialize_drawable(data) for data in shapes_data]
        
        assert len(shapes) == 3
        assert isinstance(shapes[0], Circle)
        assert isinstance(shapes[1], Rectangle) 
        assert isinstance(shapes[2], Line)
        
        # Verify properties
        assert shapes[0].radius == 10
        assert shapes[1].width == 30
        assert shapes[2].end.x == 100


class TestEnhancedMethods:
    """Test cases for enhanced serialization methods on models."""
    
    def test_enhanced_json_method(self):
        """Test enhanced JSON serialization method."""
        circle = Circle(center=Point2D(x=10, y=20), radius=15)
        
        enhanced_json = circle.to_json_enhanced()
        data = json.loads(enhanced_json)
        
        assert "__type__" in data
        assert "__version__" in data
        assert data["__type__"] == "Circle"
    
    def test_enhanced_dict_method(self):
        """Test enhanced dictionary conversion method."""
        rect = Rectangle(x=5, y=10, width=20, height=25)
        
        enhanced_dict = rect.to_dict_enhanced()
        
        assert "__type__" in enhanced_dict
        assert "__version__" in enhanced_dict
        assert enhanced_dict["__type__"] == "Rectangle"
        assert enhanced_dict["x"] == 5
        assert enhanced_dict["width"] == 20


class TestRegistryIntegration:
    """Test cases for type registry integration."""
    
    def test_automatic_type_registration(self):
        """Test that core types are automatically registered."""
        assert get_drawable_class("Circle") is Circle
        assert get_drawable_class("Rectangle") is Rectangle
        assert get_drawable_class("Line") is Line
        assert get_drawable_class("Ellipse") is Ellipse
        assert get_drawable_class("Group") is Group
        assert get_drawable_class("Layer") is Layer
        assert get_drawable_class("Drawing") is Drawing
    
    def test_type_discriminator_lookup(self):
        """Test looking up type discriminators."""
        circle = Circle(center=Point2D(x=0, y=0), radius=5)
        assert get_type_discriminator(circle) == "Circle"
        
        rect = Rectangle(x=0, y=0, width=10, height=10)
        assert get_type_discriminator(rect) == "Rectangle"