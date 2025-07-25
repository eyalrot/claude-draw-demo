"""Base model class and common functionality for Claude Draw models.

This module provides the foundational DrawModel class that serves as the base
for all data models in the Claude Draw library. It establishes common patterns
for serialization, validation, and data handling throughout the library.
"""

from typing import Any, Dict
from pydantic import BaseModel, ConfigDict


class DrawModel(BaseModel):
    """Base model for all Claude Draw data models.
    
    This class provides the foundation for all data models in the library,
    establishing common functionality for serialization, validation, and
    data manipulation. It leverages Pydantic's powerful validation and
    serialization capabilities while adding enhanced features specific to
    the Claude Draw ecosystem.
    
    Key features:
    - Automatic validation on instantiation and assignment
    - Type-safe serialization/deserialization
    - Enhanced JSON encoding with type discriminators
    - Strict mode for better runtime safety
    - Forbidden extra fields to catch typos and ensure data integrity
    
    Configuration:
    - use_enum_values: Enums serialize to their values, not names
    - validate_assignment: Validates fields when assigned after creation
    - extra="forbid": Rejects any fields not defined in the model
    - strict=True: Enables strict type checking and validation
    
    Example:
        >>> from claude_draw.models import Point2D
        >>> point = Point2D(x=10, y=20)
        >>> json_str = point.to_json()
        >>> restored = Point2D.from_json(json_str)
    """
    
    model_config = ConfigDict(
        # Use enum values instead of names in JSON for better interoperability
        # This ensures enums are serialized as their actual values (e.g., "red")
        # rather than their Python names (e.g., "Color.RED")
        use_enum_values=True,
        
        # Validate field values on assignment after model creation
        # This ensures that any mutations maintain type safety
        validate_assignment=True,
        
        # Forbid extra fields to catch typos and ensure data integrity
        # Any field not explicitly defined in the model will raise an error
        extra="forbid",
        
        # Disable arbitrary types for better performance and safety
        # All types must be Pydantic-compatible
        arbitrary_types_allowed=False,
        
        # Enable strict mode for better validation
        # This provides stricter type checking and coercion rules
        strict=True,
    )
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert model to a standard Python dictionary.
        
        This method provides a simple way to convert the model instance
        to a dictionary representation suitable for serialization or
        further processing. The output excludes any None values and
        uses the field aliases if defined.
        
        Returns:
            Dict[str, Any]: Dictionary representation of the model with
                all fields converted to Python native types
                
        Example:
            >>> point = Point2D(x=10.5, y=20.7)
            >>> point.to_dict()
            {'x': 10.5, 'y': 20.7}
        """
        return self.model_dump()
    
    def to_json(self) -> str:
        """Convert model to a JSON string representation.
        
        This method serializes the model to a JSON string using Pydantic's
        built-in JSON encoder. The output is a compact JSON representation
        suitable for storage or transmission.
        
        Returns:
            str: JSON string representation of the model
            
        Example:
            >>> circle = Circle(center=Point2D(x=0, y=0), radius=5)
            >>> json_str = circle.to_json()
            >>> print(json_str)
            '{"center":{"x":0,"y":0},"radius":5,...}'
        """
        return self.model_dump_json()
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "DrawModel":
        """Create a model instance from a dictionary.
        
        This class method provides a way to instantiate a model from
        a dictionary representation. It performs full validation to
        ensure the data conforms to the model's schema.
        
        Args:
            data: Dictionary containing model data. Keys should match
                the model's field names and values should be compatible
                with the field types.
            
        Returns:
            DrawModel: New validated model instance
            
        Raises:
            ValidationError: If the data doesn't conform to the model schema
            
        Example:
            >>> data = {'x': 10, 'y': 20}
            >>> point = Point2D.from_dict(data)
            >>> assert point.x == 10 and point.y == 20
        """
        return cls.model_validate(data)
    
    @classmethod
    def from_json(cls, json_str: str) -> "DrawModel":
        """Create a model instance from a JSON string.
        
        This class method deserializes a JSON string into a model instance,
        performing full validation to ensure data integrity.
        
        Args:
            json_str: JSON string containing model data. The JSON structure
                should match the model's field definitions.
            
        Returns:
            DrawModel: New validated model instance
            
        Raises:
            ValidationError: If the JSON data doesn't conform to the model schema
            JSONDecodeError: If the string is not valid JSON
            
        Example:
            >>> json_str = '{"x": 10, "y": 20}'
            >>> point = Point2D.from_json(json_str)
            >>> assert point.x == 10 and point.y == 20
        """
        return cls.model_validate_json(json_str)
    
    def to_json_enhanced(self, include_version: bool = True, **kwargs) -> str:
        """Serialize to JSON with enhanced features including type discriminators.
        
        This method provides advanced serialization capabilities beyond the
        standard to_json() method. It includes type discriminators that allow
        for polymorphic deserialization, making it possible to reconstruct
        the exact type hierarchy from JSON.
        
        Features:
        - Adds __type__ field for each object indicating its class
        - Optionally includes __version__ for format versioning
        - Handles circular references gracefully
        - Preserves the full type hierarchy for nested objects
        
        Args:
            include_version: Whether to include version information in the output.
                This helps with forward/backward compatibility.
            **kwargs: Additional arguments passed to the JSON encoder, such as:
                - indent: Number of spaces for pretty-printing
                - sort_keys: Whether to sort dictionary keys
                - ensure_ascii: Whether to escape non-ASCII characters
            
        Returns:
            str: Enhanced JSON string with type information that can be
                used with deserialize_drawable() for perfect round-trip
                serialization
                
        Example:
            >>> shape = Circle(center=Point2D(x=0, y=0), radius=5)
            >>> json_str = shape.to_json_enhanced(indent=2)
            >>> # JSON includes __type__ fields for polymorphic deserialization
        """
        # Import here to avoid circular imports
        from claude_draw.serialization import serialize_drawable
        return serialize_drawable(self, **kwargs)
    
    def to_dict_enhanced(self, include_version: bool = True) -> Dict[str, Any]:
        """Convert to dictionary with enhanced features including type discriminators.
        
        This method provides a dictionary representation that includes type
        information, making it suitable for scenarios where type preservation
        is important (e.g., storing in databases, sending over APIs).
        
        The enhanced dictionary includes:
        - All model fields with their values
        - __type__ field indicating the object's class
        - __version__ field for format versioning (if enabled)
        - Proper handling of nested DrawModel objects
        
        Args:
            include_version: Whether to include version information.
                Useful for maintaining compatibility when the format evolves.
            
        Returns:
            Dict[str, Any]: Enhanced dictionary representation with type
                discriminators and all nested objects properly converted
                
        Example:
            >>> rect = Rectangle(x=0, y=0, width=100, height=50)
            >>> enhanced_dict = rect.to_dict_enhanced()
            >>> print(enhanced_dict['__type__'])  # 'Rectangle'
            >>> # Can be used to reconstruct the exact object type later
        """
        # Import here to avoid circular imports
        from claude_draw.serialization import EnhancedJSONEncoder
        encoder = EnhancedJSONEncoder(include_version=include_version)
        return encoder._serialize_draw_model(self)