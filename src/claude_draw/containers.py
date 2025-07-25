"""Container implementations for Claude Draw.

This module provides concrete implementations of container objects that can
hold and organize other drawable objects in hierarchical structures. Containers
are the key to building complex graphics from simple primitives, enabling
layering, grouping, and sophisticated scene management.

Container hierarchy:
- Drawing: Root container representing the entire canvas/document
- Layer: Advanced container with blend modes and opacity control
- Group: Basic container for logical organization of shapes

Key features:
- **Hierarchical composition**: Containers can nest indefinitely
- **Immutable operations**: All modifications return new instances
- **Z-ordering**: Control rendering order within and between containers
- **Visitor pattern**: Recursive traversal for rendering and analysis
- **Bounds aggregation**: Automatic calculation of containing bounds

Design patterns:
- Composite Pattern: Uniform treatment of individual shapes and groups
- Immutable Object Pattern: Thread-safe, predictable behavior
- Fluent Interface: Chainable methods for convenient updates
"""

from typing import List, Optional, Any, TYPE_CHECKING
from enum import Enum
from pydantic import Field, ConfigDict

from claude_draw.base import Container, Drawable
from claude_draw.models.bounding_box import BoundingBox

if TYPE_CHECKING:
    from claude_draw.protocols import DrawableVisitor


class BlendMode(str, Enum):
    """Enumeration of supported blend modes for layer compositing.
    
    Blend modes determine how a layer's pixels are combined with the pixels
    of layers beneath it. These modes follow standard graphics conventions
    used in tools like Photoshop, CSS, and SVG.
    
    Categories of blend modes:
    
    **Normal**:
    - NORMAL: Standard alpha blending (default)
    
    **Darkening modes** (result is always darker):
    - MULTIPLY: Multiplies colors (darkens)
    - DARKEN: Keeps the darker color
    - COLOR_BURN: Darkens by increasing contrast
    
    **Lightening modes** (result is always lighter):
    - SCREEN: Inverted multiply (lightens)
    - LIGHTEN: Keeps the lighter color  
    - COLOR_DODGE: Lightens by decreasing contrast
    
    **Contrast modes** (increases contrast):
    - OVERLAY: Multiply or screen based on base color
    - HARD_LIGHT: Vivid light mixing
    - SOFT_LIGHT: Subtle light mixing
    
    **Difference modes** (based on color difference):
    - DIFFERENCE: Absolute difference between colors
    - EXCLUSION: Similar to difference but lower contrast
    
    Implementation note: Actual rendering depends on the backend
    (SVG renderer, canvas, etc.) supporting these modes.
    """
    
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
    """A basic container for logically organizing drawable objects.
    
    Groups are the fundamental building blocks for creating structured graphics.
    They allow you to collect related shapes and treat them as a single unit
    for transformations, organization, and manipulation.
    
    Key characteristics:
    - **Logical organization**: Group related shapes (e.g., all parts of an icon)
    - **Transform inheritance**: Transformations apply to all children
    - **Nested hierarchies**: Groups can contain other groups
    - **Named references**: Optional names for easy identification
    - **Z-ordering**: Control rendering order among siblings
    
    Common use cases:
    - **Component reuse**: Create reusable graphic components
    - **Batch operations**: Transform/style multiple objects at once
    - **Scene organization**: Separate foreground, midground, background
    - **Animation targets**: Animate entire groups as units
    - **Modular design**: Build complex graphics from simple parts
    
    Example structure:
    ```
    Group("car")
    ├── Group("body")
    │   ├── Rectangle (main body)
    │   └── Rectangle (roof)
    ├── Group("wheels")
    │   ├── Circle (front wheel)
    │   └── Circle (rear wheel)
    └── Group("details")
        ├── Rectangle (window)
        └── Line (antenna)
    ```
    
    Attributes:
        name: Optional identifier for the group
        z_index: Rendering order (higher values drawn on top)
        children: List of contained drawable objects
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )
    
    # Optional name for identification
    name: Optional[str] = Field(
        None, 
        description="Optional name for identifying this group (e.g., 'header', 'navigation', 'icon')"
    )
    
    # Z-index for ordering
    z_index: int = Field(
        0, 
        description="Z-index for rendering order. Higher values are drawn on top of lower values"
    )
    
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
    """An advanced container with rendering effects and compositing control.
    
    Layers are sophisticated containers inspired by graphics software like
    Photoshop and Illustrator. They provide fine-grained control over how
    groups of objects are rendered and composited together.
    
    Key features:
    - **Opacity control**: Affect transparency of all contained objects
    - **Blend modes**: Advanced pixel compositing with layers below
    - **Visibility toggle**: Show/hide entire layers
    - **Lock state**: Prevent accidental modifications
    - **Named organization**: Meaningful names for complex documents
    
    Layer effects:
    - **Opacity**: 0.0 (transparent) to 1.0 (opaque)
    - **Blend modes**: Various mathematical operations for pixel mixing
    - **Visibility**: Completely hide layers without deleting
    - **Locking**: Protect layers from edits in design tools
    
    Common layer structures:
    ```
    Drawing
    ├── Layer("background", opacity=1.0)
    │   └── Rectangle (background fill)
    ├── Layer("content", opacity=1.0)
    │   ├── Text elements
    │   └── Shape elements
    ├── Layer("overlays", opacity=0.8, blend_mode=MULTIPLY)
    │   └── Decorative elements
    └── Layer("annotations", visible=False)
        └── Debug information
    ```
    
    Professional workflows:
    - **Non-destructive editing**: Toggle layers without deletion
    - **Effect experimentation**: Try different blend modes
    - **Version control**: Keep multiple versions in hidden layers
    - **Collaborative work**: Lock layers to prevent conflicts
    
    Attributes:
        name: Human-readable layer identifier
        opacity: Overall transparency (multiplied with object opacity)
        blend_mode: How layer pixels mix with layers below
        visible: Whether to render this layer
        locked: Whether layer accepts modifications
        z_index: Stacking order among sibling layers
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )
    
    # Layer properties
    name: Optional[str] = Field(
        None, 
        description="Human-readable name for the layer (e.g., 'Background', 'Foreground', 'Effects')"
    )
    opacity: float = Field(
        1.0, 
        ge=0.0, 
        le=1.0, 
        description="Layer-wide opacity multiplier. 0.0 is fully transparent, 1.0 is fully opaque"
    )
    blend_mode: BlendMode = Field(
        BlendMode.NORMAL, 
        description="Pixel blending mode determining how this layer composites with layers below"
    )
    visible: bool = Field(
        True, 
        description="Whether this layer is rendered. False completely hides the layer"
    )
    locked: bool = Field(
        False, 
        description="Whether this layer is locked against modifications. Useful for preserving finalized content"
    )
    z_index: int = Field(
        0, 
        description="Stacking order for layers. Higher values are rendered on top"
    )
    
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
    
    Drawing is the top-level container that serves as the canvas for all
    graphic content. It establishes the coordinate system, defines the
    drawable area, and holds global properties that affect the entire
    document.
    
    Coordinate system:
    - **Origin**: (0, 0) at top-left corner
    - **X-axis**: Increases rightward
    - **Y-axis**: Increases downward
    - **Units**: Typically pixels, but renderer-dependent
    
    Key responsibilities:
    - **Canvas definition**: Sets the drawable area dimensions
    - **Global settings**: Background color, title, metadata
    - **Root container**: Holds all layers, groups, and shapes
    - **Document identity**: Title and description for the drawing
    
    Structure example:
    ```
    Drawing(width=1920, height=1080, title="My Artwork")
    ├── Layer("background")
    │   └── Rectangle(x=0, y=0, width=1920, height=1080, fill=white)
    ├── Layer("main-content")
    │   ├── Group("header")
    │   ├── Group("body") 
    │   └── Group("footer")
    └── Layer("watermark", opacity=0.5)
        └── Text("© 2024")
    ```
    
    Canvas considerations:
    - **Aspect ratio**: width/height determines proportions
    - **Resolution**: Pixel dimensions for raster output
    - **Bounds**: Objects outside canvas may be clipped
    - **Background**: Optional fill color behind all content
    
    Export targets:
    - **SVG**: Scalable vector graphics
    - **PNG/JPEG**: Raster images at canvas resolution
    - **PDF**: Document format preserving vectors
    - **Canvas API**: Direct rendering to HTML5 canvas
    
    Attributes:
        width: Canvas width in pixels (must be positive)
        height: Canvas height in pixels (must be positive)
        title: Human-readable name for the drawing
        description: Extended description or metadata
        background_color: CSS color string for canvas background
        children: Top-level layers and groups
    """
    
    model_config = ConfigDict(
        frozen=True,
        validate_assignment=True,
        extra="forbid",
        strict=True,
    )
    
    # Canvas dimensions
    width: float = Field(
        800.0, 
        gt=0, 
        description="Canvas width in pixels. Defines the horizontal size of the drawable area"
    )
    height: float = Field(
        600.0, 
        gt=0, 
        description="Canvas height in pixels. Defines the vertical size of the drawable area"
    )
    
    # Drawing properties
    title: Optional[str] = Field(
        None, 
        description="Human-readable title for the drawing. Used in exports and for identification"
    )
    description: Optional[str] = Field(
        None, 
        description="Extended description, notes, or metadata about the drawing's purpose or content"
    )
    
    # Background properties
    background_color: Optional[str] = Field(
        None, 
        description="CSS color string for canvas background (e.g., '#FFFFFF', 'white', 'rgb(255,255,255)'). None means transparent"
    )
    
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