#!/usr/bin/env python3
"""Performance comparison report: C++ vs Python for 100,000 shapes"""

import sys
import os
import time
import numpy as np
import math
from dataclasses import dataclass
from typing import List, Tuple, Dict
import json
from datetime import datetime

try:
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    MATPLOTLIB_AVAILABLE = True
except ImportError:
    MATPLOTLIB_AVAILABLE = False
    print("Warning: Matplotlib not available. Charts will be skipped.")

# Add build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../build-release/src/bindings'))

try:
    import _claude_draw_cpp as cpp_module
    # Import commonly used classes
    cpp = cpp_module.core
    batch = cpp_module.batch
except ImportError:
    print("Failed to import C++ bindings. Make sure to build first.")
    sys.exit(1)

# Pure Python implementations
@dataclass
class PyPoint2D:
    x: float
    y: float
    
    def transform(self, matrix):
        """Apply transformation matrix"""
        x = matrix[0][0] * self.x + matrix[0][1] * self.y + matrix[0][2]
        y = matrix[1][0] * self.x + matrix[1][1] * self.y + matrix[1][2]
        return PyPoint2D(x, y)
    
    def distance_to(self, other):
        dx = other.x - self.x
        dy = other.y - self.y
        return math.sqrt(dx * dx + dy * dy)

@dataclass
class PyColor:
    r: int
    g: int
    b: int
    a: int
    
    def blend_over(self, other):
        alpha = self.a / 255.0
        inv_alpha = 1.0 - alpha
        return PyColor(
            int(self.r * alpha + other.r * inv_alpha),
            int(self.g * alpha + other.g * inv_alpha),
            int(self.b * alpha + other.b * inv_alpha),
            255
        )

@dataclass
class PyBoundingBox:
    min_x: float
    min_y: float
    max_x: float
    max_y: float
    
    def expand(self, point):
        self.min_x = min(self.min_x, point.x)
        self.min_y = min(self.min_y, point.y)
        self.max_x = max(self.max_x, point.x)
        self.max_y = max(self.max_y, point.y)
    
    def intersects(self, other):
        return not (self.max_x < other.min_x or self.min_x > other.max_x or
                   self.max_y < other.min_y or self.min_y > other.max_y)
    
    def area(self):
        return (self.max_x - self.min_x) * (self.max_y - self.min_y)

class PyTransform2D:
    def __init__(self, matrix=None):
        self.matrix = matrix if matrix else [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
    
    @staticmethod
    def compose(t1, t2):
        """Multiply two transformation matrices"""
        result = [[0, 0, 0], [0, 0, 0], [0, 0, 1]]
        for i in range(2):
            for j in range(3):
                result[i][j] = (t1.matrix[i][0] * t2.matrix[0][j] +
                              t1.matrix[i][1] * t2.matrix[1][j] +
                              t1.matrix[i][2] * t2.matrix[2][j])
        return PyTransform2D(result)
    
    @staticmethod
    def translate(x, y):
        return PyTransform2D([[1, 0, x], [0, 1, y], [0, 0, 1]])
    
    @staticmethod
    def rotate(angle):
        c = math.cos(angle)
        s = math.sin(angle)
        return PyTransform2D([[c, -s, 0], [s, c, 0], [0, 0, 1]])
    
    @staticmethod
    def scale(sx, sy):
        return PyTransform2D([[sx, 0, 0], [0, sy, 0], [0, 0, 1]])

class BenchmarkResults:
    def __init__(self):
        self.results = {}
        self.shape_count = 100000
        self.metadata = {
            'timestamp': datetime.now().isoformat(),
            'shape_count': self.shape_count,
            'system': {
                'python_version': sys.version,
                'platform': sys.platform
            }
        }
    
    def add_result(self, test_name, cpp_time, py_time, details=None):
        self.results[test_name] = {
            'cpp_time': cpp_time,
            'python_time': py_time,
            'speedup': py_time / cpp_time if cpp_time > 0 else float('inf'),
            'details': details or {}
        }
    
    def generate_report(self):
        """Generate comprehensive report"""
        report = []
        report.append("=" * 80)
        report.append(f"C++ vs Python Performance Report - {self.shape_count:,} Shapes")
        report.append("=" * 80)
        report.append(f"Generated: {self.metadata['timestamp']}")
        report.append("")
        
        # Summary table
        report.append("PERFORMANCE SUMMARY")
        report.append("-" * 80)
        report.append(f"{'Test':<30} {'C++ (ms)':<15} {'Python (ms)':<15} {'Speedup':<10}")
        report.append("-" * 80)
        
        total_cpp = 0
        total_py = 0
        
        for test, data in self.results.items():
            cpp_ms = data['cpp_time'] * 1000
            py_ms = data['python_time'] * 1000
            speedup = data['speedup']
            
            total_cpp += cpp_ms
            total_py += py_ms
            
            report.append(f"{test:<30} {cpp_ms:<15.2f} {py_ms:<15.2f} {speedup:<10.1f}x")
        
        report.append("-" * 80)
        report.append(f"{'TOTAL':<30} {total_cpp:<15.2f} {total_py:<15.2f} {total_py/total_cpp:<10.1f}x")
        report.append("")
        
        # Detailed results
        report.append("DETAILED RESULTS")
        report.append("-" * 80)
        
        for test, data in self.results.items():
            report.append(f"\n{test}:")
            report.append(f"  C++ Time: {data['cpp_time']*1000:.3f} ms")
            report.append(f"  Python Time: {data['python_time']*1000:.3f} ms")
            report.append(f"  Speedup: {data['speedup']:.1f}x")
            
            if data['details']:
                for key, value in data['details'].items():
                    report.append(f"  {key}: {value}")
        
        return "\n".join(report)
    
    def save_json(self, filename):
        """Save results as JSON"""
        with open(filename, 'w') as f:
            json.dump({
                'metadata': self.metadata,
                'results': self.results
            }, f, indent=2)
    
    def create_charts(self, filename):
        """Create performance comparison charts"""
        if not MATPLOTLIB_AVAILABLE:
            print("Matplotlib not available - cannot create charts")
            return
            
        tests = list(self.results.keys())
        cpp_times = [self.results[t]['cpp_time'] * 1000 for t in tests]
        py_times = [self.results[t]['python_time'] * 1000 for t in tests]
        speedups = [self.results[t]['speedup'] for t in tests]
        
        fig = plt.figure(figsize=(15, 10))
        
        # Chart 1: Time comparison
        ax1 = plt.subplot(2, 2, 1)
        x = np.arange(len(tests))
        width = 0.35
        
        bars1 = ax1.bar(x - width/2, cpp_times, width, label='C++', color='#2E86AB')
        bars2 = ax1.bar(x + width/2, py_times, width, label='Python', color='#F24236')
        
        ax1.set_xlabel('Operation')
        ax1.set_ylabel('Time (ms)')
        ax1.set_title(f'Execution Time Comparison ({self.shape_count:,} shapes)')
        ax1.set_xticks(x)
        ax1.set_xticklabels(tests, rotation=45, ha='right')
        ax1.legend()
        ax1.set_yscale('log')
        ax1.grid(True, alpha=0.3)
        
        # Add value labels
        for bars in [bars1, bars2]:
            for bar in bars:
                height = bar.get_height()
                ax1.annotate(f'{height:.1f}',
                           xy=(bar.get_x() + bar.get_width() / 2, height),
                           xytext=(0, 3),
                           textcoords="offset points",
                           ha='center', va='bottom',
                           fontsize=8)
        
        # Chart 2: Speedup factors
        ax2 = plt.subplot(2, 2, 2)
        colors = ['#27AE60' if s > 10 else '#F39C12' if s > 5 else '#E74C3C' for s in speedups]
        bars = ax2.bar(tests, speedups, color=colors)
        
        ax2.axhline(y=1, color='black', linestyle='--', alpha=0.5)
        ax2.axhline(y=10, color='green', linestyle='--', alpha=0.3, label='10x speedup')
        ax2.set_xlabel('Operation')
        ax2.set_ylabel('Speedup Factor')
        ax2.set_title('C++ Performance Advantage')
        ax2.set_xticklabels(tests, rotation=45, ha='right')
        ax2.grid(True, alpha=0.3)
        
        # Add value labels
        for bar, speedup in zip(bars, speedups):
            ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.5,
                    f'{speedup:.1f}x', ha='center', va='bottom', fontsize=10)
        
        # Chart 3: Relative performance pie chart
        ax3 = plt.subplot(2, 2, 3)
        total_cpp = sum(cpp_times)
        total_py = sum(py_times)
        
        labels = ['C++ Total', 'Python Total']
        sizes = [total_cpp, total_py]
        colors = ['#2E86AB', '#F24236']
        explode = (0.1, 0)
        
        wedges, texts, autotexts = ax3.pie(sizes, explode=explode, labels=labels,
                                           colors=colors, autopct='%1.1f%%',
                                           shadow=True, startangle=90)
        ax3.set_title(f'Total Time Distribution\nC++: {total_cpp:.1f}ms, Python: {total_py:.1f}ms')
        
        # Chart 4: Operations per second
        ax4 = plt.subplot(2, 2, 4)
        cpp_ops = [self.shape_count / (t / 1000) if t > 0 else 0 for t in cpp_times]
        py_ops = [self.shape_count / (t / 1000) if t > 0 else 0 for t in py_times]
        
        x = np.arange(len(tests))
        bars1 = ax4.bar(x - width/2, cpp_ops, width, label='C++', color='#2E86AB')
        bars2 = ax4.bar(x + width/2, py_ops, width, label='Python', color='#F24236')
        
        ax4.set_xlabel('Operation')
        ax4.set_ylabel('Shapes/second')
        ax4.set_title('Throughput Comparison')
        ax4.set_xticks(x)
        ax4.set_xticklabels(tests, rotation=45, ha='right')
        ax4.legend()
        ax4.grid(True, alpha=0.3)
        ax4.ticklabel_format(style='scientific', axis='y', scilimits=(0,0))
        
        plt.tight_layout()
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        print(f"Charts saved to {filename}")

def benchmark_shape_creation(count):
    """Benchmark creating shapes"""
    print(f"\nBenchmarking shape creation ({count:,} shapes)...")
    
    # C++ version
    start = time.perf_counter()
    cpp_shapes = []
    for i in range(count):
        # Create rectangle vertices
        vertices = [
            cpp.Point2D(0, 0),
            cpp.Point2D(10, 0),
            cpp.Point2D(10, 10),
            cpp.Point2D(0, 10)
        ]
        color = cpp.Color(i % 256, (i * 2) % 256, (i * 3) % 256, 200)
        bbox = cpp.BoundingBox(0, 0, 10, 10)
        cpp_shapes.append((vertices, color, bbox))
    cpp_time = time.perf_counter() - start
    
    # Python version
    start = time.perf_counter()
    py_shapes = []
    for i in range(count):
        vertices = [
            PyPoint2D(0, 0),
            PyPoint2D(10, 0),
            PyPoint2D(10, 10),
            PyPoint2D(0, 10)
        ]
        color = PyColor(i % 256, (i * 2) % 256, (i * 3) % 256, 200)
        bbox = PyBoundingBox(0, 0, 10, 10)
        py_shapes.append((vertices, color, bbox))
    py_time = time.perf_counter() - start
    
    return cpp_time, py_time, {'shapes_created': count}

def benchmark_shape_transformation(count):
    """Benchmark transforming all shapes"""
    print(f"\nBenchmarking shape transformation ({count:,} shapes)...")
    
    # Setup
    np.random.seed(42)
    
    # C++ version
    cpp_shapes = []
    for i in range(count):
        vertices = [cpp.Point2D(0, 0), cpp.Point2D(10, 0), 
                   cpp.Point2D(10, 10), cpp.Point2D(0, 10)]
        cpp_shapes.append(vertices)
    
    transforms = []
    for i in range(count):
        angle = np.random.uniform(0, 2 * math.pi)
        tx = np.random.uniform(-100, 100)
        ty = np.random.uniform(-100, 100)
        scale = np.random.uniform(0.5, 2.0)
        
        t = cpp.Transform2D.translate(tx, ty)
        t = cpp.Transform2D.rotate(angle) * t
        t = cpp.Transform2D.scale(scale, scale) * t
        transforms.append(t)
    
    start = time.perf_counter()
    for i, (vertices, transform) in enumerate(zip(cpp_shapes, transforms)):
        transformed = [transform.transform_point(v) for v in vertices]
    cpp_time = time.perf_counter() - start
    
    # Python version
    py_shapes = []
    for i in range(count):
        vertices = [PyPoint2D(0, 0), PyPoint2D(10, 0), 
                   PyPoint2D(10, 10), PyPoint2D(0, 10)]
        py_shapes.append(vertices)
    
    py_transforms = []
    for i in range(count):
        angle = np.random.uniform(0, 2 * math.pi)
        tx = np.random.uniform(-100, 100)
        ty = np.random.uniform(-100, 100)
        scale = np.random.uniform(0.5, 2.0)
        
        t1 = PyTransform2D.translate(tx, ty)
        t2 = PyTransform2D.rotate(angle)
        t3 = PyTransform2D.scale(scale, scale)
        combined = PyTransform2D.compose(t3, PyTransform2D.compose(t2, t1))
        py_transforms.append(combined)
    
    start = time.perf_counter()
    for vertices, transform in zip(py_shapes, py_transforms):
        transformed = [v.transform(transform.matrix) for v in vertices]
    py_time = time.perf_counter() - start
    
    return cpp_time, py_time, {'vertices_per_shape': 4, 'total_points_transformed': count * 4}

def benchmark_bounding_box_calculation(count):
    """Benchmark bounding box calculations"""
    print(f"\nBenchmarking bounding box calculation ({count:,} shapes)...")
    
    # Generate random transformed shapes
    np.random.seed(42)
    all_vertices = []
    
    for i in range(count):
        # Random transform parameters
        angle = np.random.uniform(0, 2 * math.pi)
        tx = np.random.uniform(-100, 100)
        ty = np.random.uniform(-100, 100)
        scale = np.random.uniform(0.5, 2.0)
        
        # Transform vertices
        vertices = []
        for x, y in [(0, 0), (10, 0), (10, 10), (0, 10)]:
            # Apply transformations manually
            rx = x * scale * math.cos(angle) - y * scale * math.sin(angle) + tx
            ry = x * scale * math.sin(angle) + y * scale * math.cos(angle) + ty
            vertices.append((rx, ry))
        all_vertices.append(vertices)
    
    # C++ version
    cpp_vertices = []
    for vertices in all_vertices:
        cpp_verts = [cpp.Point2D(x, y) for x, y in vertices]
        cpp_vertices.append(cpp_verts)
    
    start = time.perf_counter()
    cpp_bboxes = []
    for vertices in cpp_vertices:
        bbox = cpp.BoundingBox()
        for v in vertices:
            bbox.expand(v)
        cpp_bboxes.append(bbox)
    cpp_time = time.perf_counter() - start
    
    # Python version
    py_vertices = []
    for vertices in all_vertices:
        py_verts = [PyPoint2D(x, y) for x, y in vertices]
        py_vertices.append(py_verts)
    
    start = time.perf_counter()
    py_bboxes = []
    for vertices in py_vertices:
        bbox = PyBoundingBox(float('inf'), float('inf'), float('-inf'), float('-inf'))
        for v in vertices:
            bbox.expand(v)
        py_bboxes.append(bbox)
    py_time = time.perf_counter() - start
    
    return cpp_time, py_time, {'bounding_boxes_calculated': count}

def benchmark_collision_detection(count):
    """Benchmark collision detection between shapes"""
    print(f"\nBenchmarking collision detection ({count:,} shapes)...")
    
    # Generate bounding boxes
    np.random.seed(42)
    
    # C++ version
    cpp_bboxes = []
    for i in range(count):
        x = np.random.uniform(-1000, 1000)
        y = np.random.uniform(-1000, 1000)
        w = np.random.uniform(5, 50)
        h = np.random.uniform(5, 50)
        cpp_bboxes.append(cpp.BoundingBox(x, y, x + w, y + h))
    
    start = time.perf_counter()
    cpp_collisions = 0
    # Only check first 1000 shapes against each other to keep time reasonable
    check_count = min(1000, count)
    for i in range(check_count):
        for j in range(i + 1, check_count):
            if cpp_bboxes[i].intersects(cpp_bboxes[j]):
                cpp_collisions += 1
    cpp_time = time.perf_counter() - start
    
    # Python version
    py_bboxes = []
    for i in range(count):
        x = np.random.uniform(-1000, 1000)
        y = np.random.uniform(-1000, 1000)
        w = np.random.uniform(5, 50)
        h = np.random.uniform(5, 50)
        py_bboxes.append(PyBoundingBox(x, y, x + w, y + h))
    
    start = time.perf_counter()
    py_collisions = 0
    for i in range(check_count):
        for j in range(i + 1, check_count):
            if py_bboxes[i].intersects(py_bboxes[j]):
                py_collisions += 1
    py_time = time.perf_counter() - start
    
    checks_performed = check_count * (check_count - 1) // 2
    return cpp_time, py_time, {
        'shapes_checked': check_count,
        'collision_checks': checks_performed,
        'collisions_found': cpp_collisions
    }

def benchmark_color_blending(count):
    """Benchmark color blending operations"""
    print(f"\nBenchmarking color blending ({count:,} shapes)...")
    
    # C++ version
    cpp_colors = []
    for i in range(count):
        cpp_colors.append(cpp.Color(
            np.random.randint(0, 256),
            np.random.randint(0, 256),
            np.random.randint(0, 256),
            np.random.randint(100, 200)
        ))
    
    canvas = cpp.Color(255, 255, 255, 255)
    start = time.perf_counter()
    for color in cpp_colors:
        canvas = color.blend_over(canvas)
    cpp_time = time.perf_counter() - start
    
    # Python version
    py_colors = []
    for i in range(count):
        py_colors.append(PyColor(
            np.random.randint(0, 256),
            np.random.randint(0, 256),
            np.random.randint(0, 256),
            np.random.randint(100, 200)
        ))
    
    canvas = PyColor(255, 255, 255, 255)
    start = time.perf_counter()
    for color in py_colors:
        canvas = color.blend_over(canvas)
    py_time = time.perf_counter() - start
    
    return cpp_time, py_time, {'blend_operations': count}

def benchmark_batch_operations(count):
    """Benchmark batch operations (C++ only has this feature)"""
    print(f"\nBenchmarking batch operations ({count:,} shapes)...")
    
    # C++ batch version
    start = time.perf_counter()
    # Create batch of points (4 vertices per shape)
    points = batch.create_points(count * 4, 0, 0)
    
    # Fill with rectangle vertices
    for i in range(count):
        base = i * 4
        points[base] = [0, 0]
        points[base + 1] = [10, 0]
        points[base + 2] = [10, 10]
        points[base + 3] = [0, 10]
    
    # Batch transform
    transform = cpp.Transform2D.scale(2, 2)
    transform = cpp.Transform2D.rotate(0.5) * transform
    batch.transform_points_batch(transform, points)
    
    cpp_time = time.perf_counter() - start
    
    # Python equivalent (no batch API, use loops)
    start = time.perf_counter()
    py_points = []
    for i in range(count):
        py_points.extend([
            PyPoint2D(0, 0),
            PyPoint2D(10, 0),
            PyPoint2D(10, 10),
            PyPoint2D(0, 10)
        ])
    
    # Transform all points
    angle = 0.5
    cos_a = math.cos(angle)
    sin_a = math.sin(angle)
    for i in range(len(py_points)):
        p = py_points[i]
        # Scale
        x = p.x * 2
        y = p.y * 2
        # Rotate
        rx = x * cos_a - y * sin_a
        ry = x * sin_a + y * cos_a
        py_points[i] = PyPoint2D(rx, ry)
    
    py_time = time.perf_counter() - start
    
    return cpp_time, py_time, {
        'batch_size': count * 4,
        'operations': 'scale + rotate',
        'note': 'C++ uses optimized batch API'
    }

def benchmark_complete_rendering_pipeline(count):
    """Benchmark complete rendering pipeline"""
    print(f"\nBenchmarking complete rendering pipeline ({count:,} shapes)...")
    
    # Setup scene
    np.random.seed(42)
    
    # C++ version
    start = time.perf_counter()
    
    # Create shapes
    cpp_shapes = []
    for i in range(count):
        # Create rectangle
        vertices = [cpp.Point2D(0, 0), cpp.Point2D(10, 0), 
                   cpp.Point2D(10, 10), cpp.Point2D(0, 10)]
        
        # Random transform
        x = np.random.uniform(-500, 500)
        y = np.random.uniform(-500, 500)
        angle = np.random.uniform(0, 2 * math.pi)
        scale = np.random.uniform(0.5, 2.0)
        
        transform = cpp.Transform2D.translate(x, y)
        transform = cpp.Transform2D.rotate(angle) * transform
        transform = cpp.Transform2D.scale(scale, scale) * transform
        
        # Transform vertices
        transformed = [transform.transform_point(v) for v in vertices]
        
        # Calculate bounding box
        bbox = cpp.BoundingBox()
        for v in transformed:
            bbox.expand(v)
        
        # Create color
        color = cpp.Color(
            int(np.random.randint(0, 256)),
            int(np.random.randint(0, 256)),
            int(np.random.randint(0, 256)),
            180
        )
        
        cpp_shapes.append({
            'vertices': transformed,
            'bbox': bbox,
            'color': color
        })
    
    # Render (blend all colors)
    canvas = cpp.Color(255, 255, 255, 255)
    for shape in cpp_shapes:
        canvas = shape['color'].blend_over(canvas)
    
    cpp_time = time.perf_counter() - start
    
    # Python version
    start = time.perf_counter()
    
    py_shapes = []
    for i in range(count):
        # Create rectangle
        vertices = [PyPoint2D(0, 0), PyPoint2D(10, 0), 
                   PyPoint2D(10, 10), PyPoint2D(0, 10)]
        
        # Random transform
        x = np.random.uniform(-500, 500)
        y = np.random.uniform(-500, 500)
        angle = np.random.uniform(0, 2 * math.pi)
        scale = np.random.uniform(0.5, 2.0)
        
        # Build transform
        t1 = PyTransform2D.translate(x, y)
        t2 = PyTransform2D.rotate(angle)
        t3 = PyTransform2D.scale(scale, scale)
        transform = PyTransform2D.compose(t3, PyTransform2D.compose(t2, t1))
        
        # Transform vertices
        transformed = [v.transform(transform.matrix) for v in vertices]
        
        # Calculate bounding box
        bbox = PyBoundingBox(float('inf'), float('inf'), float('-inf'), float('-inf'))
        for v in transformed:
            bbox.expand(v)
        
        # Create color
        color = PyColor(
            np.random.randint(0, 256),
            np.random.randint(0, 256),
            np.random.randint(0, 256),
            180
        )
        
        py_shapes.append({
            'vertices': transformed,
            'bbox': bbox,
            'color': color
        })
    
    # Render
    canvas = PyColor(255, 255, 255, 255)
    for shape in py_shapes:
        canvas = shape['color'].blend_over(canvas)
    
    py_time = time.perf_counter() - start
    
    return cpp_time, py_time, {
        'pipeline_steps': 'create → transform → bbox → blend',
        'vertices_per_shape': 4,
        'total_vertices': count * 4
    }

def main():
    """Run all benchmarks and generate report"""
    print("=" * 80)
    print("C++ vs Python Performance Benchmark")
    print(f"Testing with 100,000 shapes")
    print("=" * 80)
    
    results = BenchmarkResults()
    
    # Run benchmarks
    shape_count = 100000
    
    # Test 1: Shape Creation
    cpp_time, py_time, details = benchmark_shape_creation(shape_count)
    results.add_result("Shape Creation", cpp_time, py_time, details)
    
    # Test 2: Shape Transformation
    cpp_time, py_time, details = benchmark_shape_transformation(shape_count)
    results.add_result("Shape Transformation", cpp_time, py_time, details)
    
    # Test 3: Bounding Box Calculation
    cpp_time, py_time, details = benchmark_bounding_box_calculation(shape_count)
    results.add_result("Bounding Box Calc", cpp_time, py_time, details)
    
    # Test 4: Collision Detection (limited to 1000 shapes)
    cpp_time, py_time, details = benchmark_collision_detection(shape_count)
    results.add_result("Collision Detection", cpp_time, py_time, details)
    
    # Test 5: Color Blending
    cpp_time, py_time, details = benchmark_color_blending(shape_count)
    results.add_result("Color Blending", cpp_time, py_time, details)
    
    # Test 6: Batch Operations
    cpp_time, py_time, details = benchmark_batch_operations(shape_count)
    results.add_result("Batch Operations", cpp_time, py_time, details)
    
    # Test 7: Complete Pipeline
    cpp_time, py_time, details = benchmark_complete_rendering_pipeline(shape_count)
    results.add_result("Complete Pipeline", cpp_time, py_time, details)
    
    # Generate outputs
    print("\n" + "=" * 80)
    print(results.generate_report())
    
    # Save results
    results.save_json('shape_performance_results.json')
    print(f"\n✓ Results saved to shape_performance_results.json")
    
    # Create charts
    try:
        results.create_charts('shape_performance_charts.png')
    except ImportError:
        print("\n⚠ Matplotlib not available - skipping charts")
    
    # Save text report
    with open('shape_performance_report.txt', 'w') as f:
        f.write(results.generate_report())
    print(f"✓ Report saved to shape_performance_report.txt")
    
    print("\n✅ Benchmark complete!")

if __name__ == "__main__":
    main()