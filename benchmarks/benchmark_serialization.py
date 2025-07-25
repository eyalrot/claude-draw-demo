"""Serialization performance benchmarks for Claude Draw."""

import time
import json
import sys
from typing import List, Dict, Any
import statistics

from claude_draw.shapes import Circle, Rectangle, Ellipse, Line
from claude_draw.containers import Group, Layer, Drawing
from claude_draw.models.point import Point2D
from claude_draw.models.color import Color
from claude_draw.models.transform import Transform2D
from claude_draw.serialization import (
    serialize_drawable, 
    deserialize_drawable,
    EnhancedJSONEncoder,
    save_drawable,
    load_drawable
)

from benchmark_utils import (
    BenchmarkResult, 
    BenchmarkSuite, 
    ShapeGenerator,
    benchmark_function,
    MemoryMonitor
)


class SerializationBenchmarks:
    """Benchmarks for serialization and deserialization performance."""
    
    def __init__(self):
        self.generator = ShapeGenerator(seed=42)
        self.suite = BenchmarkSuite()
    
    def benchmark_single_shape_serialization(self, shape_type: str, count: int = 1000) -> BenchmarkResult:
        """Benchmark serialization of individual shapes.
        
        Args:
            shape_type: Type of shape to benchmark ('circle', 'rectangle', 'ellipse', 'line')
            count: Number of shapes to serialize
        """
        # Create shapes based on type
        shapes = []
        for _ in range(count):
            if shape_type == 'circle':
                shapes.append(self.generator.create_random_circle())
            elif shape_type == 'rectangle':
                shapes.append(self.generator.create_random_rectangle())
            elif shape_type == 'ellipse':
                shapes.append(self.generator.create_random_ellipse())
            elif shape_type == 'line':
                shapes.append(self.generator.create_random_line())
        
        monitor = MemoryMonitor()
        memory_before = monitor.start()
        
        # Benchmark serialization
        start_time = time.perf_counter()
        
        serialized_data = []
        for shape in shapes:
            json_str = serialize_drawable(shape)
            serialized_data.append(json_str)
            
            # Update peak memory every 100 iterations
            if len(serialized_data) % 100 == 0:
                monitor.update_peak()
        
        end_time = time.perf_counter()
        
        memory_after, memory_peak = monitor.finish()
        
        execution_time = end_time - start_time
        time_per_object = (execution_time * 1_000_000) / count
        
        # Calculate average JSON size
        avg_size = sum(len(s) for s in serialized_data) / len(serialized_data)
        
        result = BenchmarkResult(
            name=f"Serialize {count} {shape_type}s",
            execution_time=execution_time,
            memory_before=memory_before,
            memory_after=memory_after,
            memory_peak=memory_peak,
            memory_delta=memory_after - memory_before,
            iterations=1,
            objects_created=count,
            time_per_object=time_per_object
        )
        
        print(f"  Average JSON size: {avg_size:.0f} bytes")
        
        return result
    
    def benchmark_single_shape_deserialization(self, shape_type: str, count: int = 1000) -> BenchmarkResult:
        """Benchmark deserialization of individual shapes.
        
        Args:
            shape_type: Type of shape to benchmark
            count: Number of shapes to deserialize
        """
        # First create and serialize shapes
        shapes = []
        for _ in range(count):
            if shape_type == 'circle':
                shapes.append(self.generator.create_random_circle())
            elif shape_type == 'rectangle':
                shapes.append(self.generator.create_random_rectangle())
            elif shape_type == 'ellipse':
                shapes.append(self.generator.create_random_ellipse())
            elif shape_type == 'line':
                shapes.append(self.generator.create_random_line())
        
        # Serialize all shapes
        serialized_data = [serialize_drawable(shape) for shape in shapes]
        
        monitor = MemoryMonitor()
        memory_before = monitor.start()
        
        # Benchmark deserialization
        start_time = time.perf_counter()
        
        deserialized_shapes = []
        for json_str in serialized_data:
            shape = deserialize_drawable(json_str)
            deserialized_shapes.append(shape)
            
            # Update peak memory every 100 iterations
            if len(deserialized_shapes) % 100 == 0:
                monitor.update_peak()
        
        end_time = time.perf_counter()
        
        memory_after, memory_peak = monitor.finish()
        
        execution_time = end_time - start_time
        time_per_object = (execution_time * 1_000_000) / count
        
        return BenchmarkResult(
            name=f"Deserialize {count} {shape_type}s",
            execution_time=execution_time,
            memory_before=memory_before,
            memory_after=memory_after,
            memory_peak=memory_peak,
            memory_delta=memory_after - memory_before,
            iterations=1,
            objects_created=count,
            time_per_object=time_per_object
        )
    
    def benchmark_complex_drawing_serialization(self, layers: int = 5, shapes_per_layer: int = 100) -> BenchmarkResult:
        """Benchmark serialization of complex drawings with nested structures.
        
        Args:
            layers: Number of layers in the drawing
            shapes_per_layer: Number of shapes in each layer
        """
        # Create a complex drawing
        drawing = Drawing(width=1000, height=1000)
        
        total_shapes = 0
        for i in range(layers):
            layer = Layer(name=f"Layer_{i}")
            
            # Add groups with shapes
            for j in range(0, shapes_per_layer, 10):
                group = Group()
                
                # Add 10 mixed shapes to each group
                for k in range(10):
                    if k % 4 == 0:
                        group = group.add_child(self.generator.create_random_circle())
                    elif k % 4 == 1:
                        group = group.add_child(self.generator.create_random_rectangle())
                    elif k % 4 == 2:
                        group = group.add_child(self.generator.create_random_ellipse())
                    else:
                        group = group.add_child(self.generator.create_random_line())
                    total_shapes += 1
                
                layer = layer.add_child(group)
            
            drawing = drawing.add_child(layer)
        
        monitor = MemoryMonitor()
        memory_before = monitor.start()
        
        # Benchmark serialization
        start_time = time.perf_counter()
        
        json_str = serialize_drawable(drawing)
        
        end_time = time.perf_counter()
        
        memory_after, memory_peak = monitor.finish()
        
        execution_time = end_time - start_time
        json_size = len(json_str)
        
        print(f"  Drawing structure: {layers} layers, {total_shapes} total shapes")
        print(f"  JSON size: {json_size:,} bytes ({json_size/1024:.1f} KB)")
        
        return BenchmarkResult(
            name=f"Serialize complex drawing ({total_shapes} shapes)",
            execution_time=execution_time,
            memory_before=memory_before,
            memory_after=memory_after,
            memory_peak=memory_peak,
            memory_delta=memory_after - memory_before,
            iterations=1,
            objects_created=total_shapes,
            time_per_object=(execution_time * 1_000_000) / total_shapes
        )
    
    def benchmark_round_trip_performance(self, count: int = 100) -> BenchmarkResult:
        """Benchmark complete round-trip serialization and deserialization.
        
        Args:
            count: Number of drawings to process
        """
        # Create test drawings
        drawings = []
        total_shapes = 0
        
        for i in range(count):
            drawing = Drawing(width=500, height=500)
            layer = Layer(name=f"Layer_{i}")
            
            # Add 20 random shapes
            for _ in range(20):
                shape = self.generator.create_mixed_shapes(1)[0]
                layer = layer.add_child(shape)
                total_shapes += 1
            
            drawing = drawing.add_child(layer)
            drawings.append(drawing)
        
        monitor = MemoryMonitor()
        memory_before = monitor.start()
        
        # Benchmark round-trip
        start_time = time.perf_counter()
        
        for drawing in drawings:
            # Serialize
            json_str = serialize_drawable(drawing)
            # Deserialize
            reconstructed = deserialize_drawable(json_str)
            
            # Update peak memory every 10 iterations
            if drawings.index(drawing) % 10 == 0:
                monitor.update_peak()
        
        end_time = time.perf_counter()
        
        memory_after, memory_peak = monitor.finish()
        
        execution_time = end_time - start_time
        time_per_drawing = (execution_time * 1000) / count  # milliseconds
        
        print(f"  Round-trip time per drawing: {time_per_drawing:.2f} ms")
        
        return BenchmarkResult(
            name=f"Round-trip {count} drawings ({total_shapes} shapes)",
            execution_time=execution_time,
            memory_before=memory_before,
            memory_after=memory_after,
            memory_peak=memory_peak,
            memory_delta=memory_after - memory_before,
            iterations=1,
            objects_created=count,
            time_per_object=time_per_drawing * 1000  # Convert to microseconds
        )
    
    def benchmark_json_format_comparison(self) -> None:
        """Compare standard JSON vs enhanced JSON serialization."""
        print("\n📊 JSON Format Comparison")
        print("="*60)
        
        # Create a test drawing
        drawing = Drawing(width=800, height=600)
        layer = Layer(name="Test Layer")
        
        # Add various shapes
        circle = Circle(center=Point2D(x=100, y=100), radius=50, fill=Color.from_hex("#FF0000"))
        rectangle = Rectangle(x=200, y=200, width=100, height=50, fill=Color.from_hex("#00FF00"))
        group = Group()
        group = group.add_child(self.generator.create_random_ellipse())
        group = group.add_child(self.generator.create_random_line())
        
        layer = layer.add_child(circle)
        layer = layer.add_child(rectangle)
        layer = layer.add_child(group)
        drawing = drawing.add_child(layer)
        
        # Standard Pydantic JSON
        start_time = time.perf_counter()
        standard_json = drawing.model_dump_json(indent=2)
        standard_time = time.perf_counter() - start_time
        standard_size = len(standard_json)
        
        # Enhanced JSON with type discrimination
        start_time = time.perf_counter()
        enhanced_json = serialize_drawable(drawing, indent=2)
        enhanced_time = time.perf_counter() - start_time
        enhanced_size = len(enhanced_json)
        
        # Minimal JSON (no indentation, no version)
        start_time = time.perf_counter()
        minimal_json = json.dumps(drawing, cls=EnhancedJSONEncoder, include_version=False, separators=(',', ':'))
        minimal_time = time.perf_counter() - start_time
        minimal_size = len(minimal_json)
        
        print(f"\nStandard Pydantic JSON:")
        print(f"  Size: {standard_size:,} bytes")
        print(f"  Time: {standard_time*1000:.2f} ms")
        
        print(f"\nEnhanced JSON (with type info):")
        print(f"  Size: {enhanced_size:,} bytes ({(enhanced_size/standard_size-1)*100:+.1f}%)")
        print(f"  Time: {enhanced_time*1000:.2f} ms ({(enhanced_time/standard_time-1)*100:+.1f}%)")
        
        print(f"\nMinimal JSON (no indent/version):")
        print(f"  Size: {minimal_size:,} bytes ({(minimal_size/standard_size-1)*100:+.1f}%)")
        print(f"  Time: {minimal_time*1000:.2f} ms ({(minimal_time/standard_time-1)*100:+.1f}%)")
    
    def run_all_benchmarks(self) -> Dict[str, Any]:
        """Run all serialization benchmarks and return results."""
        print("\n🚀 Running Serialization Benchmarks")
        print("="*80)
        
        # Single shape serialization
        print("\n📝 Single Shape Serialization")
        for shape_type in ['circle', 'rectangle', 'ellipse', 'line']:
            result = self.benchmark_single_shape_serialization(shape_type, 1000)
            self.suite.add_result(result)
            print(f"  ✓ {result.name}: {result.execution_time:.3f}s")
        
        # Single shape deserialization
        print("\n📖 Single Shape Deserialization")
        for shape_type in ['circle', 'rectangle', 'ellipse', 'line']:
            result = self.benchmark_single_shape_deserialization(shape_type, 1000)
            self.suite.add_result(result)
            print(f"  ✓ {result.name}: {result.execution_time:.3f}s")
        
        # Complex drawing serialization
        print("\n🏗️ Complex Drawing Serialization")
        for layers, shapes_per_layer in [(3, 50), (5, 100), (10, 200)]:
            result = self.benchmark_complex_drawing_serialization(layers, shapes_per_layer)
            self.suite.add_result(result)
            print(f"  ✓ {result.name}: {result.execution_time:.3f}s")
        
        # Round-trip performance
        print("\n🔄 Round-trip Performance")
        for count in [50, 100, 200]:
            result = self.benchmark_round_trip_performance(count)
            self.suite.add_result(result)
            print(f"  ✓ {result.name}: {result.execution_time:.3f}s")
        
        # JSON format comparison
        self.benchmark_json_format_comparison()
        
        # Print summary
        self.suite.print_summary()
        
        # Return results
        return {
            'benchmarks': [r.to_dict() for r in self.suite.results],
            'summary': self.suite.get_summary()
        }


def main():
    """Run serialization benchmarks."""
    benchmarks = SerializationBenchmarks()
    results = benchmarks.run_all_benchmarks()
    
    # Save results
    import datetime
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"benchmark_results/serialization_{timestamp}.json"
    
    with open(filename, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\n💾 Results saved to: {filename}")


if __name__ == "__main__":
    main()