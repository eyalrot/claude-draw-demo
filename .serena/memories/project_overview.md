# Claude Draw Project Overview

Claude Draw is a Python-based 2D vector graphics library with an optional high-performance C++ backend.

## Purpose
- Create 2D vector graphics with an intuitive API
- Strong type safety using Pydantic v2
- Immutable design patterns
- Support for SVG export
- Optional C++ backend for handling millions of objects (50-100x speedup)

## Tech Stack
- **Python**: Primary language (3.9+)
- **Pydantic v2**: Data models with validation
- **C++ (optional)**: High-performance backend with SIMD optimization
- **Build Tools**: CMake (C++), setuptools (Python)
- **Testing**: pytest (Python), Google Test (C++)

## Key Features
- Core data models: Point2D, Color (RGB/HSL/Hex), Transform2D, BoundingBox
- Basic shapes: Circle, Rectangle, Ellipse, Line
- Container system: Groups, Layers with blend modes, Drawing canvas
- Visitor pattern for extensible rendering
- Full serialization/deserialization with type safety

## Project Status
- 75% Complete (9 of 12 major tasks)
- 89% test coverage with 373 tests passing
- Active development on C++ performance layer