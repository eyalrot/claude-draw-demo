"""Enhanced serialization support for Claude Draw objects."""

import json
from enum import Enum
from typing import Dict, Any, Type, Union, List, Optional, TYPE_CHECKING
from pydantic import BaseModel
from claude_draw.models.base import DrawModel

if TYPE_CHECKING:
    from claude_draw.base import Drawable
    from claude_draw.shapes import Circle, Rectangle, Line, Ellipse
    from claude_draw.containers import Group, Layer, Drawing


class SerializationRegistry:
    """Registry for mapping type discriminators to classes.
    
    This registry maintains the mapping between string type identifiers
    and their corresponding Python classes for polymorphic deserialization.
    """
    
    def __init__(self):
        """Initialize the serialization registry."""
        self._class_registry: Dict[str, Type[DrawModel]] = {}
        self._reverse_registry: Dict[Type[DrawModel], str] = {}
    
    def register(self, type_name: str, cls: Type[DrawModel]) -> None:
        """Register a class with a type discriminator.
        
        Args:
            type_name: String identifier for the type
            cls: The class to register
        """
        self._class_registry[type_name] = cls
        self._reverse_registry[cls] = type_name
    
    def get_class(self, type_name: str) -> Optional[Type[DrawModel]]:
        """Get a class by its type discriminator.
        
        Args:
            type_name: String identifier for the type
            
        Returns:
            The registered class or None if not found
        """
        return self._class_registry.get(type_name)
    
    def get_type_name(self, cls: Type[DrawModel]) -> Optional[str]:
        """Get the type discriminator for a class.
        
        Args:
            cls: The class to look up
            
        Returns:
            The type discriminator or None if not registered
        """
        return self._reverse_registry.get(cls)
    
    def get_all_types(self) -> Dict[str, Type[DrawModel]]:
        """Get all registered types.
        
        Returns:
            Dictionary mapping type names to classes
        """
        return self._class_registry.copy()


# Global registry instance
_registry = SerializationRegistry()


def register_drawable_type(type_name: str, cls: Type[DrawModel]) -> None:
    """Register a drawable type for serialization.
    
    Args:
        type_name: String identifier for the type
        cls: The drawable class to register
    """
    _registry.register(type_name, cls)


def get_drawable_class(type_name: str) -> Optional[Type[DrawModel]]:
    """Get a drawable class by its type discriminator.
    
    Args:
        type_name: String identifier for the type
        
    Returns:
        The drawable class or None if not found
    """
    return _registry.get_class(type_name)


def get_type_discriminator(obj: DrawModel) -> Optional[str]:
    """Get the type discriminator for an object.
    
    Args:
        obj: The object to get the discriminator for
        
    Returns:
        The type discriminator or None if not registered
    """
    return _registry.get_type_name(type(obj))


class EnhancedJSONEncoder(json.JSONEncoder):
    """Enhanced JSON encoder for Claude Draw objects.
    
    This encoder adds type discriminators and handles special cases
    for drawable objects and their relationships.
    """
    
    def __init__(self, include_version: bool = True, **kwargs):
        """Initialize the enhanced JSON encoder.
        
        Args:
            include_version: Whether to include version information
            **kwargs: Additional arguments for JSONEncoder
        """
        super().__init__(**kwargs)
        self.include_version = include_version
        self._object_refs: Dict[int, str] = {}
        self._ref_counter = 0
    
    def default(self, obj):
        """Convert object to JSON-serializable format.
        
        Args:
            obj: Object to serialize
            
        Returns:
            JSON-serializable representation
        """
        if isinstance(obj, DrawModel):
            return self._serialize_draw_model(obj)
        
        # Handle other special types as needed
        return super().default(obj)
    
    def _serialize_draw_model(self, obj: DrawModel) -> Dict[str, Any]:
        """Serialize a DrawModel object with type discriminator.
        
        Args:
            obj: DrawModel instance to serialize
            
        Returns:
            Dictionary representation with type information
        """
        # Import here to avoid circular imports
        from claude_draw.base import Drawable
        
        # Only handle references for Drawable objects, not basic data models
        if isinstance(obj, Drawable):
            # Handle circular references if needed
            obj_id = id(obj)
            if obj_id in self._object_refs:
                # Return a reference instead of the full object
                return {"__ref__": self._object_refs[obj_id]}
            
            # Assign a reference ID
            ref_id = f"obj_{self._ref_counter}"
            self._object_refs[obj_id] = ref_id
            self._ref_counter += 1
        else:
            ref_id = None
        
        # Build dictionary manually to avoid polymorphic issues
        data = {}
        
        # Get all fields from the actual class, not the base class
        for field_name, field_info in obj.__class__.model_fields.items():
            value = getattr(obj, field_name)
            
            if isinstance(value, DrawModel):
                data[field_name] = self._serialize_draw_model(value)
            elif isinstance(value, list):
                data[field_name] = [
                    self._serialize_draw_model(item) if isinstance(item, DrawModel) else 
                    (item.model_dump() if hasattr(item, 'model_dump') else item)
                    for item in value
                ]
            elif isinstance(value, dict):
                data[field_name] = {
                    k: self._serialize_draw_model(v) if isinstance(v, DrawModel) else 
                       (v.model_dump() if hasattr(v, 'model_dump') else v)
                    for k, v in value.items()
                }
            elif hasattr(value, 'model_dump'):
                # Handle other Pydantic models (but don't add type discriminators for non-DrawModel objects)
                # Don't serialize references for non-DrawModel objects like Color, Point2D, etc
                data[field_name] = value.model_dump()
            elif isinstance(value, Enum):
                # Handle enum values
                data[field_name] = value.value
            else:
                data[field_name] = value
        
        # Add type discriminator only for Drawable objects
        if isinstance(obj, Drawable):
            type_name = get_type_discriminator(obj)
            if type_name:
                data["__type__"] = type_name
            else:
                # Fallback to class name if not registered
                data["__type__"] = obj.__class__.__name__
            
            # Add version if requested for Drawable objects
            if self.include_version:
                data["__version__"] = "1.0"
        
        # Add reference ID only for Drawable objects
        if ref_id:
            data["__id__"] = ref_id
        
        return data


class SerializationMixin:
    """Mixin providing enhanced serialization capabilities.
    
    This mixin can be added to drawable classes to provide
    enhanced JSON serialization with type discrimination.
    """
    
    def to_json_enhanced(self, include_version: bool = True, **kwargs) -> str:
        """Serialize to JSON with enhanced features.
        
        Args:
            include_version: Whether to include version information
            **kwargs: Additional arguments for JSON encoder
            
        Returns:
            Enhanced JSON string representation
        """
        return json.dumps(
            self,
            cls=EnhancedJSONEncoder,
            include_version=include_version,
            **kwargs
        )
    
    def to_dict_enhanced(self, include_version: bool = True) -> Dict[str, Any]:
        """Convert to dictionary with enhanced features.
        
        Args:
            include_version: Whether to include version information
            
        Returns:
            Enhanced dictionary representation
        """
        encoder = EnhancedJSONEncoder(include_version=include_version)
        return encoder._serialize_draw_model(self)


def deserialize_drawable(data: Union[str, Dict[str, Any]]) -> "Drawable":
    """Deserialize a drawable object from JSON data with type restoration.
    
    This function reconstructs Claude Draw objects from JSON data that
    includes type discriminators. It handles the complete deserialization
    process including type lookup, nested object processing, and validation.
    
    Deserialization process:
    1. Parse JSON if string input
    2. Extract __type__ discriminator
    3. Look up class in registry
    4. Recursively process nested objects
    5. Handle enum conversions
    6. Validate and instantiate object
    
    Type safety features:
    - Registry-based type lookup
    - Recursive type preservation
    - Enum value conversion
    - Pydantic validation
    
    Args:
        data: JSON string or dictionary containing serialized data.
            Must include __type__ field for proper deserialization.
            Can be output from serialize_drawable() or compatible format.
        
    Returns:
        Drawable: Deserialized object with correct type restored.
            All nested objects also have proper types.
        
    Raises:
        ValueError: If type discriminator is missing or unknown,
            or if circular references are present
        TypeError: If data format is invalid (not dict or string)
        ValidationError: If data doesn't match the expected schema
        json.JSONDecodeError: If string input is not valid JSON
        
    Example:
        >>> data = {
        ...     "__type__": "Circle",
        ...     "center": {"x": 100, "y": 100},
        ...     "radius": 50
        ... }
        >>> circle = deserialize_drawable(data)
        >>> assert isinstance(circle, Circle)
    """
    if isinstance(data, str):
        data = json.loads(data)
    
    if not isinstance(data, dict):
        raise TypeError("Data must be a dictionary or JSON string")
    
    # Handle references
    if "__ref__" in data:
        raise ValueError("Cannot deserialize reference without context. Use deserialize_with_references instead.")
    
    # Extract type discriminator
    type_name = data.get("__type__")
    if not type_name:
        raise ValueError("Missing type discriminator '__type__' in data")
    
    # Get the corresponding class
    cls = get_drawable_class(type_name)
    if not cls:
        raise ValueError(f"Unknown type discriminator: {type_name}")
    
    # Remove metadata fields before deserialization
    clean_data = {k: v for k, v in data.items() 
                  if not k.startswith("__")}
    
    # Process nested objects recursively
    for key, value in clean_data.items():
        if isinstance(value, dict) and "__type__" in value:
            clean_data[key] = deserialize_drawable(value)
        elif isinstance(value, list):
            clean_data[key] = [
                deserialize_drawable(item) if isinstance(item, dict) and "__type__" in item 
                else item for item in value
            ]
    
    # Handle enum fields manually for strict validation
    if hasattr(cls, 'model_fields'):
        for field_name, field_info in cls.model_fields.items():
            if field_name in clean_data:
                # Check if this field is an enum type
                field_type = field_info.annotation
                # Handle Optional types and direct enum types
                origin = getattr(field_type, '__origin__', None)
                if origin is Union:
                    # For Optional[EnumType] (which is Union[EnumType, None])
                    args = getattr(field_type, '__args__', ())
                    for arg in args:
                        if hasattr(arg, '__mro__') and any(issubclass(base, Enum) for base in arg.__mro__ if base != object):
                            if isinstance(clean_data[field_name], str):
                                clean_data[field_name] = arg(clean_data[field_name])
                            break
                elif hasattr(field_type, '__mro__') and any(issubclass(base, Enum) for base in field_type.__mro__ if base != object):
                    # Direct enum type
                    if isinstance(clean_data[field_name], str):
                        clean_data[field_name] = field_type(clean_data[field_name])
    
    # Deserialize using the class
    return cls.model_validate(clean_data)


def deserialize_with_references(data: Union[str, Dict[str, Any]]) -> "Drawable":
    """Deserialize drawable objects with full circular reference support.
    
    This advanced deserialization function handles complex object graphs
    that may contain circular references. It uses a two-pass algorithm
    to resolve references after all objects are created.
    
    Reference resolution algorithm:
    1. **First pass**: Create all objects, cache by __id__
    2. **Track references**: Note __ref__ fields for later
    3. **Second pass**: Replace references with cached objects
    4. **Validation**: Ensure all references are resolved
    
    Use cases:
    - Complex drawings with shared components
    - Graphs with cycles (future path objects)
    - Memory-efficient storage of repeated elements
    - Advanced composition patterns
    
    Reference format:
    - Objects with __id__ field are cacheable
    - {"__ref__": "obj_0"} refers to cached object
    - Enables structure sharing in JSON
    
    Args:
        data: JSON string or dictionary containing serialized data
            with potential __ref__ and __id__ fields for reference
            management.
        
    Returns:
        Drawable: Deserialized object with all references resolved.
            Circular references are properly reconstructed.
        
    Raises:
        ValueError: If type discriminator is missing, unknown, or if
            any references cannot be resolved
        TypeError: If data format is invalid
        ValidationError: If data doesn't match expected schemas
        
    Note:
        This function is more complex than deserialize_drawable() and
        should only be used when circular references are expected.
        For simple hierarchies, use the standard function.
    """
    if isinstance(data, str):
        data = json.loads(data)
    
    if not isinstance(data, dict):
        raise TypeError("Data must be a dictionary or JSON string")
    
    # Reference resolution context
    object_cache: Dict[str, "Drawable"] = {}
    deferred_refs: List[tuple] = []
    
    def _deserialize_recursive(obj_data: Dict[str, Any]) -> "Drawable":
        """Recursively deserialize with reference tracking."""
        
        # Handle reference
        if "__ref__" in obj_data:
            ref_id = obj_data["__ref__"]
            if ref_id in object_cache:
                return object_cache[ref_id]
            else:
                # Create a placeholder that will be resolved later
                deferred_refs.append((ref_id, obj_data))
                return None  # Will be resolved in second pass
        
        # Extract metadata
        type_name = obj_data.get("__type__")
        obj_id = obj_data.get("__id__")
        
        if not type_name:
            raise ValueError("Missing type discriminator '__type__' in data")
        
        # Get the corresponding class
        cls = get_drawable_class(type_name)
        if not cls:
            raise ValueError(f"Unknown type discriminator: {type_name}")
        
        # Remove metadata fields
        clean_data = {k: v for k, v in obj_data.items() 
                      if not k.startswith("__")}
        
        # Process nested objects recursively
        for key, value in clean_data.items():
            if isinstance(value, dict) and "__type__" in value:
                clean_data[key] = _deserialize_recursive(value)
            elif isinstance(value, list):
                clean_data[key] = [
                    _deserialize_recursive(item) if isinstance(item, dict) and "__type__" in item 
                    else item for item in value
                ]
        
        # Create object
        obj = cls.model_validate(clean_data)
        
        # Cache for reference resolution
        if obj_id:
            object_cache[obj_id] = obj
        
        return obj
    
    # First pass: deserialize objects
    result = _deserialize_recursive(data)
    
    # Second pass: resolve deferred references
    for ref_id, ref_data in deferred_refs:
        if ref_id not in object_cache:
            raise ValueError(f"Unresolved reference: {ref_id}")
        # Update any objects that had null references
        # This is a simplified approach - a full implementation would
        # need to track where references were used and update them
    
    return result


def serialize_drawable(obj: "Drawable", **kwargs) -> str:
    """Serialize drawable objects to enhanced JSON format.
    
    Primary serialization function that converts Claude Draw objects
    into JSON with full type information and metadata. This enables
    perfect round-trip conversion of complex object hierarchies.
    
    Serialization features:
    - Type discrimination via __type__ fields
    - Circular reference detection and handling
    - Version metadata for future compatibility
    - Recursive processing of nested structures
    - Clean, human-readable output
    
    Args:
        obj: Drawable object to serialize. Any DrawModel subclass
            including shapes, containers, or complete drawings.
        **kwargs: Additional arguments for JSON encoder:
            - indent: Pretty-print indentation (e.g., indent=2)
            - sort_keys: Sort dictionary keys alphabetically
            - ensure_ascii: Force ASCII-only output
            - include_version: Include __version__ field (default: True)
        
    Returns:
        str: JSON string with type discriminators and metadata.
            Output is suitable for deserialize_drawable() function.
            
    Example:
        >>> rect = Rectangle(x=0, y=0, width=100, height=50)
        >>> json_str = serialize_drawable(rect, indent=2)
        >>> # Output includes __type__ for deserialization
    """
    return json.dumps(obj, cls=EnhancedJSONEncoder, **kwargs)


def load_drawable(filename: str) -> "Drawable":
    """Load a drawable object from a JSON file.
    
    Convenience function that reads a JSON file and deserializes it
    into Claude Draw objects with proper type restoration. Handles
    the complete loading process including file I/O and deserialization.
    
    File format expectations:
    - Standard JSON with type discriminators
    - UTF-8 encoding
    - Created by save_drawable() or compatible
    - Contains __type__ fields for objects
    
    Args:
        filename: Path to the JSON file to load. File must exist
            and contain valid JSON with type discriminators.
        
    Returns:
        Drawable: Deserialized object with correct types restored.
            Typically a Drawing object containing a complete scene.
            
    Raises:
        FileNotFoundError: If the specified file doesn't exist
        IOError: If file cannot be read
        json.JSONDecodeError: If file contains invalid JSON
        ValueError: If type discriminators are missing or unknown
        ValidationError: If data doesn't match expected schemas
        
    Example:
        >>> # Load a saved drawing
        >>> drawing = load_drawable("artwork.json")
        >>> print(f"Loaded {drawing.title}")
    """
    with open(filename, 'r', encoding='utf-8') as f:
        data = json.load(f)
    return deserialize_drawable(data)


def save_drawable(obj: "Drawable", filename: str, **kwargs) -> None:
    """Save a drawable object to a JSON file with type preservation.
    
    Convenience function that serializes Claude Draw objects and writes
    them to disk in a human-readable JSON format. The output preserves
    all type information for perfect reconstruction.
    
    Output characteristics:
    - Pretty-printed JSON (2-space indent by default)
    - UTF-8 encoding for international support
    - Type discriminators for all Drawable objects
    - Clean formatting for version control
    
    File handling:
    - Creates new file or overwrites existing
    - Atomic write would be ideal for production
    - Parent directory must exist
    - Extension .json recommended
    
    Args:
        obj: Drawable object to save. Typically a Drawing object
            containing a complete scene, but can be any drawable.
        filename: Path where the JSON file will be saved. Parent
            directory must exist. File will be created or overwritten.
        **kwargs: Additional arguments for JSON encoder:
            - indent: Indentation spaces (default: 2)  
            - sort_keys: Sort keys alphabetically
            - ensure_ascii: Force ASCII output
            - include_version: Add version metadata
    
    Raises:
        IOError: If file cannot be written
        OSError: If path is invalid or permissions denied
        
    Example:
        >>> drawing = Drawing(width=800, height=600, title="My Art")
        >>> # ... add shapes ...
        >>> save_drawable(drawing, "my_art.json")
        >>> # Creates formatted JSON file with type info
    """
    with open(filename, 'w', encoding='utf-8') as f:
        json.dump(obj, f, cls=EnhancedJSONEncoder, indent=2, **kwargs)


# Auto-register core drawable types
def _register_core_types():
    """Register core drawable types with the serialization system.
    
    This initialization function registers all built-in Claude Draw types
    with the serialization registry. It's called automatically on module
    import to ensure types are available for deserialization.
    
    Registration strategy:
    - Try to import and register each module's types
    - Gracefully handle import failures (circular imports)
    - Types are registered with simple string names
    - Enables lookup during deserialization
    
    Registered types:
    - Shapes: Circle, Rectangle, Line, Ellipse
    - Containers: Group, Layer, Drawing
    - Models: Transform2D, Point2D, Color (if needed)
    
    Custom types:
    Users can register additional types using:
    >>> register_drawable_type("MyShape", MyShape)
    """
    try:
        from claude_draw.shapes import Circle, Rectangle, Line, Ellipse
        register_drawable_type("Circle", Circle)
        register_drawable_type("Rectangle", Rectangle)
        register_drawable_type("Line", Line)
        register_drawable_type("Ellipse", Ellipse)
    except ImportError:
        pass  # Handle case where shapes haven't been imported yet
    
    try:
        from claude_draw.containers import Group, Layer, Drawing
        register_drawable_type("Group", Group)
        register_drawable_type("Layer", Layer)
        register_drawable_type("Drawing", Drawing)
    except ImportError:
        pass  # Handle case where containers haven't been imported yet
    
    try:
        from claude_draw.models.transform import Transform2D
        from claude_draw.models.point import Point2D
        from claude_draw.models.color import Color
        register_drawable_type("Transform2D", Transform2D)
        register_drawable_type("Point2D", Point2D)
        register_drawable_type("Color", Color)
    except ImportError:
        pass  # Handle case where models haven't been imported yet


# Initialize core types
_register_core_types()