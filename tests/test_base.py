"""Tests for abstract base classes."""

import pytest
from typing import Any
from uuid import UUID
from claude_draw.base import Drawable, StyleMixin, Primitive, Container
from claude_draw.models.transform import Transform2D
from claude_draw.models.bounding_box import BoundingBox
from claude_draw.models.color import Color
from claude_draw.models.point import Point2D


class ConcreteDrawable(Drawable):
    """Concrete implementation of Drawable for testing."""
    
    def get_bounds(self) -> BoundingBox:
        return BoundingBox(x=0, y=0, width=100, height=100)
    
    def accept(self, visitor) -> str:
        return "accepted"


class ConcreteStyleMixin(StyleMixin):
    """Concrete implementation of StyleMixin for testing."""
    pass


class ConcretePrimitive(Primitive):
    """Concrete implementation of Primitive for testing."""
    
    def get_bounds(self) -> BoundingBox:
        return BoundingBox(x=0, y=0, width=50, height=50)
    
    def accept(self, visitor) -> str:
        return "primitive_accepted"


class ConcreteContainer(Container):
    """Concrete implementation of Container for testing."""
    
    def add_child(self, child: Drawable) -> "ConcreteContainer":
        new_children = self.children + [child]
        return self.model_copy(update={"children": new_children})
    
    def remove_child(self, child_id: str) -> "ConcreteContainer":
        new_children = [c for c in self.children if c.id != child_id]
        return self.model_copy(update={"children": new_children})
    
    def accept(self, visitor) -> str:
        return "container_accepted"


class TestDrawable:
    """Test cases for Drawable abstract base class."""
    
    def test_drawable_creation(self):
        """Test basic drawable creation."""
        drawable = ConcreteDrawable()
        
        # Check that ID is generated
        assert isinstance(drawable.id, str)
        assert len(drawable.id) > 0
        
        # Check that transform is default identity
        assert drawable.transform == Transform2D()
        
        # Check that it's immutable
        assert drawable.model_config['frozen'] is True
    
    def test_drawable_with_custom_id(self):
        """Test drawable creation with custom ID."""
        custom_id = "custom_id_123"
        drawable = ConcreteDrawable(id=custom_id)
        
        assert drawable.id == custom_id
    
    def test_drawable_with_custom_transform(self):
        """Test drawable creation with custom transform."""
        transform = Transform2D().translate(10, 20)
        drawable = ConcreteDrawable(transform=transform)
        
        assert drawable.transform == transform
    
    def test_drawable_immutability(self):
        """Test that drawable objects are immutable."""
        from pydantic import ValidationError
        
        drawable = ConcreteDrawable()
        
        # Should not be able to modify fields
        with pytest.raises(ValidationError):
            drawable.id = "new_id"
        
        with pytest.raises(ValidationError):
            drawable.transform = Transform2D()
    
    def test_drawable_with_transform(self):
        """Test with_transform method."""
        drawable = ConcreteDrawable()
        new_transform = Transform2D().scale(2, 2)
        
        new_drawable = drawable.with_transform(new_transform)
        
        # Original unchanged
        assert drawable.transform == Transform2D()
        
        # New instance has new transform
        assert new_drawable.transform == new_transform
        assert new_drawable.id == drawable.id  # ID should be preserved
    
    def test_drawable_with_id(self):
        """Test with_id method."""
        drawable = ConcreteDrawable()
        original_id = drawable.id
        new_id = "new_id_456"
        
        new_drawable = drawable.with_id(new_id)
        
        # Original unchanged
        assert drawable.id == original_id
        
        # New instance has new ID
        assert new_drawable.id == new_id
        assert new_drawable.transform == drawable.transform  # Transform preserved
    
    def test_drawable_translate(self):
        """Test translate method."""
        drawable = ConcreteDrawable()
        
        translated = drawable.translate(5, 10)
        
        # Original unchanged
        assert drawable.transform == Transform2D()
        
        # New instance is translated
        expected_transform = Transform2D().translate(5, 10)
        assert translated.transform == expected_transform
    
    def test_drawable_rotate(self):
        """Test rotate method."""
        drawable = ConcreteDrawable()
        angle = 1.57  # ~90 degrees
        
        rotated = drawable.rotate(angle)
        
        # Original unchanged
        assert drawable.transform == Transform2D()
        
        # New instance is rotated
        expected_transform = Transform2D().rotate(angle)
        assert rotated.transform == expected_transform
    
    def test_drawable_scale(self):
        """Test scale method."""
        drawable = ConcreteDrawable()
        
        # Uniform scaling
        scaled_uniform = drawable.scale(2)
        expected_uniform = Transform2D().scale(2, 2)
        assert scaled_uniform.transform == expected_uniform
        
        # Non-uniform scaling
        scaled_non_uniform = drawable.scale(2, 3)
        expected_non_uniform = Transform2D().scale(2, 3)
        assert scaled_non_uniform.transform == expected_non_uniform
    
    def test_drawable_chained_transforms(self):
        """Test chaining multiple transforms."""
        drawable = ConcreteDrawable()
        
        result = drawable.translate(10, 20).rotate(1.57).scale(2, 3)
        
        # Should create a compound transform
        expected = Transform2D().translate(10, 20).rotate(1.57).scale(2, 3)
        assert result.transform == expected
    
    def test_drawable_get_bounds(self):
        """Test get_bounds method."""
        drawable = ConcreteDrawable()
        bounds = drawable.get_bounds()
        
        assert bounds == BoundingBox(x=0, y=0, width=100, height=100)
    
    def test_drawable_accept_visitor(self):
        """Test accept method."""
        drawable = ConcreteDrawable()
        result = drawable.accept("mock_visitor")
        
        assert result == "accepted"


class TestStyleMixin:
    """Test cases for StyleMixin abstract base class."""
    
    def test_style_mixin_defaults(self):
        """Test StyleMixin default values."""
        style = ConcreteStyleMixin()
        
        assert style.fill is None
        assert style.stroke is None
        assert style.stroke_width == 1.0
        assert style.opacity == 1.0
        assert style.visible is True
    
    def test_style_mixin_custom_values(self):
        """Test StyleMixin with custom values."""
        fill = Color.from_hex("#FF0000")
        stroke = Color.from_hex("#000000")
        
        style = ConcreteStyleMixin(
            fill=fill,
            stroke=stroke,
            stroke_width=2.0,
            opacity=0.5,
            visible=False
        )
        
        assert style.fill == fill
        assert style.stroke == stroke
        assert style.stroke_width == 2.0
        assert style.opacity == 0.5
        assert style.visible is False
    
    def test_style_mixin_immutability(self):
        """Test that StyleMixin is immutable."""
        from pydantic import ValidationError
        
        style = ConcreteStyleMixin()
        
        with pytest.raises(ValidationError):
            style.fill = Color.from_hex("#FF0000")
    
    def test_style_mixin_with_fill(self):
        """Test with_fill method."""
        style = ConcreteStyleMixin()
        red = Color.from_hex("#FF0000")
        
        styled = style.with_fill(red)
        
        # Original unchanged
        assert style.fill is None
        
        # New instance has fill
        assert styled.fill == red
    
    def test_style_mixin_with_stroke(self):
        """Test with_stroke method."""
        style = ConcreteStyleMixin()
        blue = Color.from_hex("#0000FF")
        
        # With color only
        styled = style.with_stroke(blue)
        assert styled.stroke == blue
        assert styled.stroke_width == 1.0  # Default preserved
        
        # With color and width
        styled_with_width = style.with_stroke(blue, 3.0)
        assert styled_with_width.stroke == blue
        assert styled_with_width.stroke_width == 3.0
    
    def test_style_mixin_with_opacity(self):
        """Test with_opacity method."""
        style = ConcreteStyleMixin()
        
        transparent = style.with_opacity(0.3)
        
        assert style.opacity == 1.0  # Original unchanged
        assert transparent.opacity == 0.3
    
    def test_style_mixin_with_visibility(self):
        """Test with_visibility method."""
        style = ConcreteStyleMixin()
        
        hidden = style.with_visibility(False)
        
        assert style.visible is True  # Original unchanged
        assert hidden.visible is False
    
    def test_style_mixin_is_filled(self):
        """Test is_filled method."""
        style = ConcreteStyleMixin()
        assert style.is_filled() is False
        
        filled = style.with_fill(Color.from_hex("#FF0000"))
        assert filled.is_filled() is True
    
    def test_style_mixin_is_stroked(self):
        """Test is_stroked method."""
        style = ConcreteStyleMixin()
        assert style.is_stroked() is False
        
        # With stroke color but no width
        stroked_no_width = style.with_stroke(Color.from_hex("#000000"), 0)
        assert stroked_no_width.is_stroked() is False
        
        # With stroke color and width
        stroked = style.with_stroke(Color.from_hex("#000000"), 2.0)
        assert stroked.is_stroked() is True
    
    def test_style_mixin_validation(self):
        """Test StyleMixin validation."""
        # Invalid stroke width
        with pytest.raises(ValueError):
            ConcreteStyleMixin(stroke_width=-1.0)
        
        # Invalid opacity
        with pytest.raises(ValueError):
            ConcreteStyleMixin(opacity=-0.1)
        
        with pytest.raises(ValueError):
            ConcreteStyleMixin(opacity=1.1)


class TestPrimitive:
    """Test cases for Primitive abstract base class."""
    
    def test_primitive_combines_drawable_and_style(self):
        """Test that Primitive combines Drawable and StyleMixin."""
        primitive = ConcretePrimitive()
        
        # Should have Drawable properties
        assert hasattr(primitive, 'id')
        assert hasattr(primitive, 'transform')
        
        # Should have StyleMixin properties
        assert hasattr(primitive, 'fill')
        assert hasattr(primitive, 'stroke')
        assert hasattr(primitive, 'opacity')
        
        # Should have methods from both
        assert hasattr(primitive, 'with_transform')
        assert hasattr(primitive, 'with_fill')
        assert hasattr(primitive, 'get_bounds')
        assert hasattr(primitive, 'accept')
    
    def test_primitive_styling(self):
        """Test Primitive styling functionality."""
        primitive = ConcretePrimitive()
        
        styled = primitive.with_fill(Color.from_hex("#FF0000")).with_stroke(Color.from_hex("#000000"))
        
        assert styled.fill == Color.from_hex("#FF0000")
        assert styled.stroke == Color.from_hex("#000000")
    
    def test_primitive_transformation(self):
        """Test Primitive transformation functionality."""
        primitive = ConcretePrimitive()
        
        transformed = primitive.translate(10, 20).scale(2)
        
        expected_transform = Transform2D().translate(10, 20).scale(2, 2)
        assert transformed.transform == expected_transform


class TestContainer:
    """Test cases for Container abstract base class."""
    
    def test_container_creation(self):
        """Test basic container creation."""
        container = ConcreteContainer()
        
        assert container.children == []
        assert container.has_children() is False
    
    def test_container_add_child(self):
        """Test adding children to container."""
        container = ConcreteContainer()
        child = ConcreteDrawable()
        
        with_child = container.add_child(child)
        
        # Original unchanged
        assert container.children == []
        
        # New instance has child
        assert len(with_child.children) == 1
        assert with_child.children[0] is child
        assert with_child.has_children() is True
    
    def test_container_remove_child(self):
        """Test removing children from container."""
        container = ConcreteContainer()
        child1 = ConcreteDrawable()
        child2 = ConcreteDrawable()
        
        # Add children
        with_children = container.add_child(child1).add_child(child2)
        assert len(with_children.children) == 2
        
        # Remove one child
        without_child1 = with_children.remove_child(child1.id)
        assert len(without_child1.children) == 1
        assert without_child1.children[0] is child2
    
    def test_container_get_children(self):
        """Test get_children method."""
        container = ConcreteContainer()
        child = ConcreteDrawable()
        
        with_child = container.add_child(child)
        children = with_child.get_children()
        
        assert len(children) == 1
        assert children[0] is child
        
        # Should return a copy
        children.append("fake_child")
        assert len(with_child.get_children()) == 1
    
    def test_container_get_child_by_id(self):
        """Test get_child_by_id method."""
        container = ConcreteContainer()
        child = ConcreteDrawable()
        
        with_child = container.add_child(child)
        
        # Should find the child
        found = with_child.get_child_by_id(child.id)
        assert found is child
        
        # Should return None for non-existent ID
        not_found = with_child.get_child_by_id("non_existent")
        assert not_found is None
    
    def test_container_bounds_empty(self):
        """Test bounds calculation for empty container."""
        container = ConcreteContainer()
        bounds = container.get_bounds()
        
        assert bounds == BoundingBox(x=0, y=0, width=0, height=0)
    
    def test_container_bounds_with_children(self):
        """Test bounds calculation with children."""
        container = ConcreteContainer()
        
        # Mock children with different bounds
        class MockChild(ConcreteDrawable):
            def __init__(self, bounds: BoundingBox):
                super().__init__()
                self._bounds = bounds
            
            def get_bounds(self) -> BoundingBox:
                return self._bounds
        
        child1 = MockChild(BoundingBox(x=0, y=0, width=10, height=10))
        child2 = MockChild(BoundingBox(x=20, y=30, width=15, height=5))
        
        with_children = container.add_child(child1).add_child(child2)
        bounds = with_children.get_bounds()
        
        # Should encompass both children
        # min_x = 0, min_y = 0, max_x = 35, max_y = 35
        expected = BoundingBox(x=0, y=0, width=35, height=35)
        assert bounds == expected
    
    def test_container_immutability(self):
        """Test that Container is immutable."""
        from pydantic import ValidationError
        
        container = ConcreteContainer()
        
        with pytest.raises(ValidationError):
            container.children = []