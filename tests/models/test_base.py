"""Tests for base model functionality."""

import json
import pytest
from pydantic import ValidationError as PydanticValidationError

from claude_draw.models.base import DrawModel


class TestModel(DrawModel):
    """Test model for testing base functionality."""
    name: str
    value: int


class TestDrawModel:
    """Test the DrawModel base class."""
    
    def test_model_creation(self):
        """Test creating a model instance."""
        model = TestModel(name="test", value=42)
        assert model.name == "test"
        assert model.value == 42
    
    def test_to_dict(self):
        """Test converting model to dictionary."""
        model = TestModel(name="test", value=42)
        data = model.to_dict()
        assert data == {"name": "test", "value": 42}
    
    def test_to_json(self):
        """Test converting model to JSON."""
        model = TestModel(name="test", value=42)
        json_str = model.to_json()
        data = json.loads(json_str)
        assert data == {"name": "test", "value": 42}
    
    def test_from_dict(self):
        """Test creating model from dictionary."""
        data = {"name": "test", "value": 42}
        model = TestModel.from_dict(data)
        assert model.name == "test"
        assert model.value == 42
    
    def test_from_json(self):
        """Test creating model from JSON."""
        json_str = '{"name": "test", "value": 42}'
        model = TestModel.from_json(json_str)
        assert model.name == "test"
        assert model.value == 42
    
    def test_validate_assignment(self):
        """Test that assignment validation works."""
        model = TestModel(name="test", value=42)
        
        # Valid assignment
        model.value = 100
        assert model.value == 100
        
        # Invalid assignment
        with pytest.raises(PydanticValidationError):
            model.value = "not an int"  # type: ignore
    
    def test_extra_fields_forbidden(self):
        """Test that extra fields are forbidden."""
        with pytest.raises(PydanticValidationError):
            TestModel(name="test", value=42, extra="field")  # type: ignore
    
    def test_strict_mode(self):
        """Test that strict mode is enabled."""
        # Strict mode should not coerce types
        with pytest.raises(PydanticValidationError):
            TestModel(name="test", value="42")  # type: ignore