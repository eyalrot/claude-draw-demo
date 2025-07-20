"""Container implementations for Claude Draw.

This module provides concrete implementations of container objects that can
hold and organize other drawable objects in hierarchical structures.
"""

from typing import List, Optional, Any, TYPE_CHECKING
from enum import Enum
from pydantic import Field, ConfigDict

from claude_draw.base import Container, Drawable
from claude_draw.models.bounding_box import BoundingBox

if TYPE_CHECKING:
    from claude_draw.protocols import DrawableVisitor


class BlendMode(str, Enum):
    """Enumeration of supported blend modes for layers."""
    
    NORMAL = "normal"
    MULTIPLY = "multiply"
    SCREEN = "screen"
    OVERLAY = "overlay"
    DARKEN = "darken"
    LIGHTEN = "lighten"
    COLOR_DODGE = "color-dodge"
    COLOR_BURN = "color-burn"
    HARD_LIGHT = "hard-light"
    SOFT_LIGHT = "soft-light"
    DIFFERENCE = "difference"
    EXCLUSION = "exclusion"


class Group(Container):
    """A container that groups drawable objects together.
    
    Groups allow organizing multiple drawable objects into a single unit
    that can be transformed, styled, and managed as a cohesive entity.
    Groups can contain other groups, creating hierarchical structures.
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )
    
    # Optional name for identification
    name: Optional[str] = Field(None, description="Optional name for the group")
    
    # Z-index for ordering
    z_index: int = Field(0, description="Z-index for rendering order")
    
    def add_child(self, child: Drawable) -> "Group":
        """Add a child drawable object to the group.
        
        Args:
            child: The drawable object to add as a child
            
        Returns:
            Group: A new instance with the child added
        """
        new_children = self.children + [child]
        return self.model_copy(update={"children": new_children})
    
    def remove_child(self, child_id: str) -> "Group":
        """Remove a child drawable object by ID.
        
        Args:
            child_id: The ID of the child to remove
            
        Returns:
            Group: A new instance with the child removed
        """
        new_children = [child for child in self.children if child.id != child_id]
        return self.model_copy(update={"children": new_children})
    
    def with_name(self, name: Optional[str]) -> "Group":
        """Create a new instance with the specified name.
        
        Args:
            name: The new name for the group
            
        Returns:
            Group: A new instance with the updated name
        """
        return self.model_copy(update={"name": name})
    
    def with_z_index(self, z_index: int) -> "Group":
        """Create a new instance with the specified z-index.
        
        Args:
            z_index: The new z-index for ordering
            
        Returns:
            Group: A new instance with the updated z-index
        """
        return self.model_copy(update={"z_index": z_index})
    
    def get_children_sorted(self) -> List[Drawable]:
        """Get children sorted by their z-index if they have one.
        
        Returns:
            List[Drawable]: Children sorted by z-index
        """
        # Sort children by z-index if they have the attribute, otherwise use 0
        return sorted(
            self.children, 
            key=lambda child: getattr(child, 'z_index', 0)
        )
    
    def accept(self, visitor: "DrawableVisitor") -> Any:
        """Accept a visitor for processing this group.
        
        Args:
            visitor: The visitor object that will process this group
            
        Returns:
            Any: The result of the visitor's processing
        """
        return visitor.visit_group(self)


class Layer(Container):
    """A layer container with advanced rendering properties.
    
    Layers are specialized containers that support blend modes, opacity,
    and other advanced rendering features. They provide a way to organize
    content with specific visual effects applied to the entire layer.
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )
    
    # Layer properties
    name: Optional[str] = Field(None, description="Optional name for the layer")
    opacity: float = Field(1.0, ge=0.0, le=1.0, description="Layer opacity")
    blend_mode: BlendMode = Field(BlendMode.NORMAL, description="Blend mode for compositing")
    visible: bool = Field(True, description="Whether the layer is visible")
    locked: bool = Field(False, description="Whether the layer is locked for editing")
    z_index: int = Field(0, description="Z-index for layer ordering")
    
    def add_child(self, child: Drawable) -> "Layer":
        """Add a child drawable object to the layer.
        
        Args:
            child: The drawable object to add as a child
            
        Returns:
            Layer: A new instance with the child added
        """
        new_children = self.children + [child]
        return self.model_copy(update={"children": new_children})
    
    def remove_child(self, child_id: str) -> "Layer":
        """Remove a child drawable object by ID.
        
        Args:
            child_id: The ID of the child to remove
            
        Returns:
            Layer: A new instance with the child removed
        """
        new_children = [child for child in self.children if child.id != child_id]
        return self.model_copy(update={"children": new_children})
    
    def with_name(self, name: Optional[str]) -> "Layer":
        """Create a new instance with the specified name.
        
        Args:
            name: The new name for the layer
            
        Returns:
            Layer: A new instance with the updated name
        """
        return self.model_copy(update={"name": name})
    
    def with_opacity(self, opacity: float) -> "Layer":
        """Create a new instance with the specified opacity.
        
        Args:
            opacity: The new opacity value (0.0 to 1.0)
            
        Returns:
            Layer: A new instance with the updated opacity
        """
        return self.model_copy(update={"opacity": opacity})
    
    def with_blend_mode(self, blend_mode: BlendMode) -> "Layer":
        """Create a new instance with the specified blend mode.
        
        Args:
            blend_mode: The new blend mode
            
        Returns:
            Layer: A new instance with the updated blend mode
        """
        return self.model_copy(update={"blend_mode": blend_mode})
    
    def with_visibility(self, visible: bool) -> "Layer":
        """Create a new instance with the specified visibility.
        
        Args:
            visible: Whether the layer should be visible
            
        Returns:
            Layer: A new instance with the updated visibility
        """
        return self.model_copy(update={"visible": visible})
    
    def with_locked(self, locked: bool) -> "Layer":
        """Create a new instance with the specified lock state.
        
        Args:
            locked: Whether the layer should be locked
            
        Returns:
            Layer: A new instance with the updated lock state
        """
        return self.model_copy(update={"locked": locked})
    
    def with_z_index(self, z_index: int) -> "Layer":
        """Create a new instance with the specified z-index.
        
        Args:
            z_index: The new z-index for ordering
            
        Returns:
            Layer: A new instance with the updated z-index
        """
        return self.model_copy(update={"z_index": z_index})
    
    def is_editable(self) -> bool:
        """Check if the layer can be edited (not locked and visible).
        
        Returns:
            bool: True if the layer can be edited
        """
        return not self.locked and self.visible
    
    def accept(self, visitor: "DrawableVisitor") -> Any:
        """Accept a visitor for processing this layer.
        
        Args:
            visitor: The visitor object that will process this layer
            
        Returns:
            Any: The result of the visitor's processing
        """
        return visitor.visit_layer(self)


class Drawing(Container):
    """The root container representing a complete drawing or canvas.
    
    Drawing is the top-level container that holds all other drawable objects.
    It defines the coordinate system, canvas size, and global properties
    for the entire drawing.
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )
    
    # Canvas dimensions
    width: float = Field(800.0, gt=0, description="Canvas width")
    height: float = Field(600.0, gt=0, description="Canvas height")
    
    # Drawing properties
    title: Optional[str] = Field(None, description="Optional title for the drawing")
    description: Optional[str] = Field(None, description="Optional description")
    
    # Background properties
    background_color: Optional[str] = Field(None, description="Background color (CSS color)")
    
    def add_child(self, child: Drawable) -> "Drawing":
        """Add a child drawable object to the drawing.
        
        Args:
            child: The drawable object to add as a child
            
        Returns:
            Drawing: A new instance with the child added
        """
        new_children = self.children + [child]
        return self.model_copy(update={"children": new_children})
    
    def remove_child(self, child_id: str) -> "Drawing":
        """Remove a child drawable object by ID.
        
        Args:
            child_id: The ID of the child to remove
            
        Returns:
            Drawing: A new instance with the child removed
        """
        new_children = [child for child in self.children if child.id != child_id]
        return self.model_copy(update={"children": new_children})
    
    def with_dimensions(self, width: float, height: float) -> "Drawing":
        """Create a new instance with the specified dimensions.
        
        Args:
            width: The new canvas width
            height: The new canvas height
            
        Returns:
            Drawing: A new instance with the updated dimensions
        """
        return self.model_copy(update={"width": width, "height": height})
    
    def with_title(self, title: Optional[str]) -> "Drawing":
        """Create a new instance with the specified title.
        
        Args:
            title: The new title for the drawing
            
        Returns:
            Drawing: A new instance with the updated title
        """
        return self.model_copy(update={"title": title})
    
    def with_description(self, description: Optional[str]) -> "Drawing":
        """Create a new instance with the specified description.
        
        Args:
            description: The new description for the drawing
            
        Returns:
            Drawing: A new instance with the updated description
        """
        return self.model_copy(update={"description": description})
    
    def with_background_color(self, background_color: Optional[str]) -> "Drawing":
        """Create a new instance with the specified background color.
        
        Args:
            background_color: The new background color (CSS color string)
            
        Returns:
            Drawing: A new instance with the updated background color
        """
        return self.model_copy(update={"background_color": background_color})
    
    def get_canvas_bounds(self) -> BoundingBox:
        """Get the bounding box representing the entire canvas.
        
        Returns:
            BoundingBox: The canvas bounding box
        """
        return BoundingBox(x=0, y=0, width=self.width, height=self.height)
    
    def get_layers(self) -> List[Layer]:
        """Get all direct child layers.
        
        Returns:
            List[Layer]: List of child layers
        """
        return [child for child in self.children if isinstance(child, Layer)]
    
    def get_groups(self) -> List[Group]:
        """Get all direct child groups.
        
        Returns:
            List[Group]: List of child groups
        """
        return [child for child in self.children if isinstance(child, Group)]
    
    def get_layers_sorted(self) -> List[Layer]:
        """Get all layers sorted by z-index.
        
        Returns:
            List[Layer]: Layers sorted by z-index
        """
        layers = self.get_layers()
        return sorted(layers, key=lambda layer: layer.z_index)
    
    def accept(self, visitor: "DrawableVisitor") -> Any:
        """Accept a visitor for processing this drawing.
        
        Args:
            visitor: The visitor object that will process this drawing
            
        Returns:
            Any: The result of the visitor's processing
        """
        return visitor.visit_drawing(self)