"""Visitor pattern implementation for Claude Draw."""

from abc import ABC, abstractmethod
from typing import Any, List, Optional, Dict, TYPE_CHECKING
from dataclasses import dataclass, field

from claude_draw.models.transform import Transform2D
from claude_draw.models.color import Color
from claude_draw.protocols import DrawableVisitor

if TYPE_CHECKING:
    from claude_draw.shapes import Circle, Rectangle, Line, Ellipse
    from claude_draw.containers import Group, Layer, Drawing
    from claude_draw.base import Drawable


@dataclass
class RenderState:
    """Represents the current rendering state.
    
    This class encapsulates all the rendering attributes that can be
    inherited or modified during the rendering process.
    """
    
    # Transformation matrix
    transform: Transform2D = field(default_factory=Transform2D)
    
    # Style properties
    fill: Optional[Color] = None
    stroke: Optional[Color] = None
    stroke_width: float = 1.0
    opacity: float = 1.0
    
    # Visibility
    visible: bool = True
    
    def copy(self) -> "RenderState":
        """Create a copy of this render state.
        
        Returns:
            RenderState: A new instance with the same values
        """
        return RenderState(
            transform=self.transform,
            fill=self.fill,
            stroke=self.stroke,
            stroke_width=self.stroke_width,
            opacity=self.opacity,
            visible=self.visible
        )


class RenderContext:
    """Manages rendering context with transformation matrix stack and state.
    
    The RenderContext maintains a stack of transformations and rendering
    states that allows for proper handling of nested groups and hierarchical
    transformations during the rendering process.
    """
    
    def __init__(self):
        """Initialize the render context with default state."""
        self._transform_stack: List[Transform2D] = [Transform2D()]
        self._state_stack: List[RenderState] = [RenderState()]
        
    @property
    def current_transform(self) -> Transform2D:
        """Get the current transformation matrix.
        
        Returns:
            Transform2D: The current combined transformation
        """
        return self._transform_stack[-1]
    
    @property
    def current_state(self) -> RenderState:
        """Get the current rendering state.
        
        Returns:
            RenderState: The current rendering state
        """
        return self._state_stack[-1]
    
    def push_transform(self, transform: Transform2D) -> None:
        """Push a new transformation onto the stack.
        
        The new transformation is combined with the current transformation
        to create a cumulative effect.
        
        Args:
            transform: The transformation to apply
        """
        current = self._transform_stack[-1]
        new_transform = current * transform
        self._transform_stack.append(new_transform)
    
    def pop_transform(self) -> Transform2D:
        """Pop the current transformation from the stack.
        
        Returns:
            Transform2D: The transformation that was removed
            
        Raises:
            ValueError: If attempting to pop the base transformation
        """
        if len(self._transform_stack) <= 1:
            raise ValueError("Cannot pop base transformation")
        return self._transform_stack.pop()
    
    def push_state(self, **updates) -> None:
        """Push a new rendering state onto the stack.
        
        Args:
            **updates: State properties to update from current state
        """
        current_state = self._state_stack[-1]
        new_state = current_state.copy()
        
        # Update state with provided values
        for key, value in updates.items():
            if hasattr(new_state, key):
                setattr(new_state, key, value)
        
        self._state_stack.append(new_state)
    
    def pop_state(self) -> RenderState:
        """Pop the current rendering state from the stack.
        
        Returns:
            RenderState: The state that was removed
            
        Raises:
            ValueError: If attempting to pop the base state
        """
        if len(self._state_stack) <= 1:
            raise ValueError("Cannot pop base rendering state")
        return self._state_stack.pop()
    
    def push(self, transform: Optional[Transform2D] = None, **state_updates) -> None:
        """Push both transformation and state in one operation.
        
        Args:
            transform: Optional transformation to push
            **state_updates: State properties to update
        """
        if transform is not None:
            self.push_transform(transform)
        else:
            # Push identity transform to keep stacks in sync
            self.push_transform(Transform2D())
            
        self.push_state(**state_updates)
    
    def pop(self) -> tuple[Transform2D, RenderState]:
        """Pop both transformation and state in one operation.
        
        Returns:
            tuple: (popped_transform, popped_state)
        """
        state = self.pop_state()
        transform = self.pop_transform()
        return transform, state
    
    def get_effective_opacity(self) -> float:
        """Calculate the effective opacity from all states in the stack.
        
        Returns:
            float: The combined opacity value
        """
        opacity = 1.0
        for state in self._state_stack:
            opacity *= state.opacity
        return opacity
    
    def is_visible(self) -> bool:
        """Check if the current object should be visible.
        
        Returns:
            bool: True if all states in the stack are visible
        """
        return all(state.visible for state in self._state_stack)


class BaseRenderer(DrawableVisitor, ABC):
    """Abstract base class for implementing renderers using the visitor pattern.
    
    This class provides the foundation for all renderers, managing the render
    context and providing template methods for the rendering process.
    """
    
    def __init__(self):
        """Initialize the base renderer."""
        self.context = RenderContext()
        self._output: List[str] = []
    
    def render(self, drawable: "Drawable") -> str:
        """Render a drawable object and return the output.
        
        Args:
            drawable: The drawable object to render
            
        Returns:
            str: The rendered output
        """
        self._output.clear()
        self.begin_render()
        
        try:
            drawable.accept(self)
        finally:
            self.end_render()
        
        return self.get_output()
    
    @abstractmethod
    def begin_render(self) -> None:
        """Initialize the rendering process.
        
        This method is called before rendering begins and should set up
        any necessary headers, initialization, or state.
        """
        pass
    
    @abstractmethod
    def end_render(self) -> None:
        """Finalize the rendering process.
        
        This method is called after rendering is complete and should
        add any necessary footers or cleanup.
        """
        pass
    
    def get_output(self) -> str:
        """Get the current rendered output.
        
        Returns:
            str: The complete rendered output
        """
        return "".join(self._output)
    
    def emit(self, content: str) -> None:
        """Emit content to the output.
        
        Args:
            content: The content to add to the output
        """
        self._output.append(content)
    
    def pre_visit(self, drawable: "Drawable") -> None:
        """Called before visiting a drawable object.
        
        Override this method to perform setup operations before
        processing each drawable.
        
        Args:
            drawable: The drawable object about to be visited
        """
        pass
    
    def post_visit(self, drawable: "Drawable") -> None:
        """Called after visiting a drawable object.
        
        Override this method to perform cleanup operations after
        processing each drawable.
        
        Args:
            drawable: The drawable object that was just visited
        """
        pass
    
    # Container visit methods with default implementations
    
    def visit_group(self, group: "Group") -> Any:
        """Visit a group object.
        
        Args:
            group: The group object to visit
            
        Returns:
            Any: Result of processing the group
        """
        self.pre_visit(group)
        
        # Push group's transform and state
        self.context.push(
            transform=group.transform,
            opacity=getattr(group, 'opacity', 1.0),
            visible=getattr(group, 'visible', True)
        )
        
        try:
            # Visit children sorted by z-index
            children = group.get_children_sorted() if hasattr(group, 'get_children_sorted') else group.children
            for child in children:
                if self.context.is_visible():
                    child.accept(self)
        finally:
            self.context.pop()
        
        self.post_visit(group)
        return None
    
    def visit_layer(self, layer: "Layer") -> Any:
        """Visit a layer object.
        
        Args:
            layer: The layer object to visit
            
        Returns:
            Any: Result of processing the layer
        """
        self.pre_visit(layer)
        
        # Push layer's transform and state
        self.context.push(
            transform=layer.transform,
            opacity=layer.opacity,
            visible=layer.visible
        )
        
        try:
            # Only render if layer is visible
            if layer.visible and self.context.is_visible():
                # Visit children sorted by z-index
                children = sorted(layer.children, key=lambda child: getattr(child, 'z_index', 0))
                for child in children:
                    child.accept(self)
        finally:
            self.context.pop()
        
        self.post_visit(layer)
        return None
    
    def visit_drawing(self, drawing: "Drawing") -> Any:
        """Visit a drawing object.
        
        Args:
            drawing: The drawing object to visit
            
        Returns:
            Any: Result of processing the drawing
        """
        self.pre_visit(drawing)
        
        # Push drawing's transform (usually identity)
        self.context.push(transform=drawing.transform)
        
        try:
            # Visit all children
            for child in drawing.children:
                child.accept(self)
        finally:
            self.context.pop()
        
        self.post_visit(drawing)
        return None
    
    # Abstract methods for primitive shapes - concrete renderers must implement these
    
    @abstractmethod
    def visit_circle(self, circle: "Circle") -> Any:
        """Visit a circle object."""
        pass
    
    @abstractmethod
    def visit_rectangle(self, rectangle: "Rectangle") -> Any:
        """Visit a rectangle object."""
        pass
    
    @abstractmethod
    def visit_line(self, line: "Line") -> Any:
        """Visit a line object."""
        pass
    
    @abstractmethod
    def visit_ellipse(self, ellipse: "Ellipse") -> Any:
        """Visit an ellipse object."""
        pass