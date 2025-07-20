"""Base model class and common functionality for Claude Draw models."""

from typing import Any, Dict
from pydantic import BaseModel, ConfigDict


class DrawModel(BaseModel):
    """Base model for all Claude Draw data models.
    
    Provides common functionality and configuration for all models.
    """
    
    model_config = ConfigDict(
        # Use enum values instead of names in JSON
        use_enum_values=True,
        # Validate field values on assignment
        validate_assignment=True,
        # Allow extra fields to be ignored
        extra="forbid",
        # Optimize model creation
        arbitrary_types_allowed=False,
        # Enable strict mode for better validation
        strict=True,
    )
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert model to dictionary.
        
        Returns:
            Dictionary representation of the model
        """
        return self.model_dump()
    
    def to_json(self) -> str:
        """Convert model to JSON string.
        
        Returns:
            JSON string representation of the model
        """
        return self.model_dump_json()
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "DrawModel":
        """Create model from dictionary.
        
        Args:
            data: Dictionary containing model data
            
        Returns:
            New model instance
        """
        return cls.model_validate(data)
    
    @classmethod
    def from_json(cls, json_str: str) -> "DrawModel":
        """Create model from JSON string.
        
        Args:
            json_str: JSON string containing model data
            
        Returns:
            New model instance
        """
        return cls.model_validate_json(json_str)
    
    def to_json_enhanced(self, include_version: bool = True, **kwargs) -> str:
        """Serialize to JSON with enhanced features including type discriminators.
        
        Args:
            include_version: Whether to include version information
            **kwargs: Additional arguments for JSON encoder
            
        Returns:
            Enhanced JSON string representation
        """
        # Import here to avoid circular imports
        from claude_draw.serialization import serialize_drawable
        return serialize_drawable(self, **kwargs)
    
    def to_dict_enhanced(self, include_version: bool = True) -> Dict[str, Any]:
        """Convert to dictionary with enhanced features including type discriminators.
        
        Args:
            include_version: Whether to include version information
            
        Returns:
            Enhanced dictionary representation
        """
        # Import here to avoid circular imports
        from claude_draw.serialization import EnhancedJSONEncoder
        encoder = EnhancedJSONEncoder(include_version=include_version)
        return encoder._serialize_draw_model(self)