#!/usr/bin/env python3
"""
Example: Drawing a house with trees using Claude Draw.

This example demonstrates the container system, basic shapes, and styling
capabilities of the Claude Draw library.
"""

from claude_draw.containers import Drawing, Layer, Group
from claude_draw.shapes import Circle, Rectangle, Ellipse
from claude_draw.models.point import Point2D
from claude_draw.models.color import Color

def create_house_with_trees():
    """Create a drawing of a house with trees."""
    
    # Create the main drawing canvas
    drawing = Drawing(
        width=800, 
        height=600, 
        title="House with Trees",
        description="A simple scene with a house and trees",
        background_color="#87CEEB"  # Sky blue background
    )
    
    # Create layers for organization
    background_layer = Layer(name="background", z_index=1)
    house_layer = Layer(name="house", z_index=2) 
    foreground_layer = Layer(name="foreground", z_index=3)
    
    # === BACKGROUND ELEMENTS ===
    
    # Sun
    sun = Circle(
        center=Point2D(x=650, y=100),
        radius=40,
        fill=Color.from_hex("#FFD700"),  # Gold
        stroke=Color.from_hex("#FFA500"),  # Orange
        stroke_width=2
    )
    
    # Clouds
    cloud1_group = Group(name="cloud1")
    cloud1_parts = [
        Circle(center=Point2D(x=150, y=80), radius=25, fill=Color.from_hex("#FFFFFF")),
        Circle(center=Point2D(x=175, y=75), radius=30, fill=Color.from_hex("#FFFFFF")),
        Circle(center=Point2D(x=200, y=80), radius=25, fill=Color.from_hex("#FFFFFF")),
        Circle(center=Point2D(x=125, y=90), radius=20, fill=Color.from_hex("#FFFFFF"))
    ]
    for part in cloud1_parts:
        cloud1_group = cloud1_group.add_child(part)
    
    # Ground
    ground = Rectangle(
        x=0, y=450,
        width=800, height=150,
        fill=Color.from_hex("#90EE90")  # Light green
    )
    
    background_layer = background_layer.add_child(sun).add_child(cloud1_group).add_child(ground)
    
    # === HOUSE ===
    
    house_group = Group(name="house_structure")
    
    # House base
    house_base = Rectangle(
        x=300, y=300,
        width=200, height=150,
        fill=Color.from_hex("#DEB887"),  # Burlywood
        stroke=Color.from_hex("#8B4513"),  # Saddle brown
        stroke_width=2
    )
    
    # Roof - using an ellipse to approximate a triangle roof
    roof = Ellipse(
        center=Point2D(x=400, y=280),
        rx=120, ry=40,
        fill=Color.from_hex("#8B4513"),  # Saddle brown
        stroke=Color.from_hex("#654321"),  # Dark brown
        stroke_width=2
    )
    
    # Door
    door = Rectangle(
        x=375, y=380,
        width=50, height=70,
        fill=Color.from_hex("#8B4513"),  # Saddle brown
        stroke=Color.from_hex("#654321"),
        stroke_width=2
    )
    
    # Door knob
    door_knob = Circle(
        center=Point2D(x=415, y=415),
        radius=3,
        fill=Color.from_hex("#FFD700"),  # Gold
    )
    
    # Windows
    window1 = Rectangle(
        x=320, y=330,
        width=40, height=40,
        fill=Color.from_hex("#87CEEB"),  # Sky blue (glass)
        stroke=Color.from_hex("#654321"),
        stroke_width=2
    )
    
    window2 = Rectangle(
        x=440, y=330,
        width=40, height=40,
        fill=Color.from_hex("#87CEEB"),  # Sky blue (glass)
        stroke=Color.from_hex("#654321"),
        stroke_width=2
    )
    
    # Window cross patterns
    window1_cross_h = Rectangle(x=320, y=348, width=40, height=4, fill=Color.from_hex("#654321"))
    window1_cross_v = Rectangle(x=338, y=330, width=4, height=40, fill=Color.from_hex("#654321"))
    window2_cross_h = Rectangle(x=440, y=348, width=40, height=4, fill=Color.from_hex("#654321"))
    window2_cross_v = Rectangle(x=458, y=330, width=4, height=40, fill=Color.from_hex("#654321"))
    
    # Assemble house
    house_parts = [house_base, roof, door, door_knob, window1, window2, 
                   window1_cross_h, window1_cross_v, window2_cross_h, window2_cross_v]
    for part in house_parts:
        house_group = house_group.add_child(part)
    
    house_layer = house_layer.add_child(house_group)
    
    # === TREES ===
    
    # Left tree
    left_tree_group = Group(name="left_tree")
    
    left_trunk = Rectangle(
        x=90, y=350,
        width=20, height=100,
        fill=Color.from_hex("#8B4513"),  # Saddle brown
        stroke=Color.from_hex("#654321"),
        stroke_width=1
    )
    
    left_canopy = Circle(
        center=Point2D(x=100, y=330),
        radius=50,
        fill=Color.from_hex("#228B22"),  # Forest green
        stroke=Color.from_hex("#006400"),  # Dark green
        stroke_width=2
    )
    
    left_tree_group = left_tree_group.add_child(left_trunk).add_child(left_canopy)
    
    # Right tree
    right_tree_group = Group(name="right_tree")
    
    right_trunk = Rectangle(
        x=680, y=360,
        width=25, height=90,
        fill=Color.from_hex("#8B4513"),  # Saddle brown
        stroke=Color.from_hex("#654321"),
        stroke_width=1
    )
    
    right_canopy = Ellipse(
        center=Point2D(x=692, y=340),
        rx=45, ry=60,
        fill=Color.from_hex("#228B22"),  # Forest green
        stroke=Color.from_hex("#006400"),  # Dark green
        stroke_width=2
    )
    
    right_tree_group = right_tree_group.add_child(right_trunk).add_child(right_canopy)
    
    # Small bushes
    bush1 = Circle(
        center=Point2D(x=200, y=420),
        radius=25,
        fill=Color.from_hex("#32CD32"),  # Lime green
        stroke=Color.from_hex("#228B22"),
        stroke_width=1
    )
    
    bush2 = Circle(
        center=Point2D(x=580, y=430),
        radius=20,
        fill=Color.from_hex("#32CD32"),  # Lime green
        stroke=Color.from_hex("#228B22"),
        stroke_width=1
    )
    
    foreground_layer = foreground_layer.add_child(left_tree_group).add_child(right_tree_group)
    foreground_layer = foreground_layer.add_child(bush1).add_child(bush2)
    
    # === ASSEMBLE DRAWING ===
    
    drawing = drawing.add_child(background_layer).add_child(house_layer).add_child(foreground_layer)
    
    return drawing

def main():
    """Create the drawing and attempt to save as SVG."""
    
    print("Creating house with trees drawing...")
    
    # Create the drawing
    drawing = create_house_with_trees()
    
    print(f"✅ Drawing created successfully!")
    print(f"   - Canvas: {drawing.width}x{drawing.height}")
    print(f"   - Title: {drawing.title}")
    print(f"   - Layers: {len(drawing.get_layers())}")
    print(f"   - Total children: {len(drawing.children)}")
    
    # For now, let's print the structure since SVG export isn't implemented yet
    print("\n📋 Drawing Structure:")
    for i, child in enumerate(drawing.children):
        print(f"   {i+1}. {type(child).__name__}: {getattr(child, 'name', 'unnamed')}")
        if hasattr(child, 'children') and child.children:
            for j, grandchild in enumerate(child.children):
                if hasattr(grandchild, 'children') and grandchild.children:
                    print(f"      └── {type(grandchild).__name__}: {getattr(grandchild, 'name', 'unnamed')} ({len(grandchild.children)} items)")
                else:
                    print(f"      └── {type(grandchild).__name__}")
    
    # Note about SVG export
    print("\n📝 Note: SVG export functionality is not yet implemented.")
    print("    This example demonstrates the object structure and hierarchy.")
    print("    Once the visitor pattern renderer is implemented, this drawing")
    print("    can be exported to SVG, PNG, and other formats.")
    
    # Save the drawing data structure for inspection
    print(f"\n💾 Drawing object created with {len(drawing.children)} top-level layers.")
    print("    You can inspect the drawing object in the Python REPL.")
    
    return drawing

if __name__ == "__main__":
    drawing = main()