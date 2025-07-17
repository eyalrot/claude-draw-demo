#!/usr/bin/env python3
"""Simple example demonstrating Claude Draw usage."""

from claude_draw import Canvas, Circle, Rectangle


def main():
    """Create a simple drawing with Claude Draw."""
    # Create a canvas
    canvas = Canvas(width=400, height=300, background="lightgray")
    
    # Add some shapes
    canvas.add(Circle(x=200, y=150, radius=50, fill="blue", stroke="darkblue", stroke_width=2))
    canvas.add(Rectangle(x=50, y=50, width=100, height=80, fill="red", stroke="darkred"))
    canvas.add(Circle(x=350, y=250, radius=30, fill="green"))
    canvas.add(Rectangle(x=250, y=30, width=120, height=60, fill="yellow", stroke="orange", stroke_width=3))
    
    # Save the result
    canvas.save("hello_world.svg")
    print("Drawing saved to hello_world.svg")
    
    # Also print the SVG to console
    print("\nGenerated SVG:")
    print(canvas.to_svg())


if __name__ == "__main__":
    main()