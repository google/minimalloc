# MiniMalloc Python Bindings

Python bindings for MiniMalloc, a lightweight memory allocator designed for hardware-accelerated machine learning workloads.

## Installation

### From Source

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/google/minimalloc.git
cd minimalloc

# Install Python dependencies
pip install pybind11>=2.6.0

# Build and install
pip install .
```

### Development Installation

```bash
# Install in development mode
pip install -e .

# Install with development dependencies
pip install -e .[dev]
```

## Quick Start

```python
import minimalloc

# Create buffers with lifespans and sizes
buffers = [
    minimalloc.Buffer("b1", lower=0, upper=3, size=4),
    minimalloc.Buffer("b2", lower=3, upper=9, size=4),
    minimalloc.Buffer("b3", lower=0, upper=9, size=4),
]

# Create allocation problem
problem = minimalloc.Problem(buffers, capacity=12)

# Solve
solver = minimalloc.Solver()
solution = solver.solve(problem)

print(f"Solution: {solution}")
# Output: Solution([8, 8, 4])
```

## CSV Interface

```python
# Load from CSV
csv_data = """id,lower,upper,size
b1,0,3,4
b2,3,9,4
b3,0,9,4"""

# Quick solve
solution = minimalloc.solve_from_csv(csv_data, capacity=12)

# Or step-by-step
problem = minimalloc.Problem.from_csv(csv_data, capacity=12)
solver = minimalloc.Solver()
solution = solver.solve(problem)

# Export results
output_csv = problem.to_csv(solution)
print(output_csv)
```

## API Reference

### Core Classes

#### `Buffer`
Represents a memory buffer with a lifespan.

```python
buffer = minimalloc.Buffer(
    id="buffer_name",           # Unique identifier
    lower=0,                    # Start time
    upper=10,                   # End time (exclusive)
    size=8,                     # Size in bytes
    alignment=1,                # Memory alignment (optional)
    offset=None,                # Fixed offset (optional)
    hint=None,                  # Placement hint (optional)
    gaps=[]                     # Inactive periods (optional)
)
```

#### `Gap` 
Represents an inactive period within a buffer's lifespan.

```python
gap = minimalloc.Gap(
    lower=5,                    # Gap start time
    upper=7,                    # Gap end time
    window_lower=None,          # Window start (optional)
    window_upper=None           # Window end (optional)
)
```

#### `Problem`
Defines a memory allocation problem.

```python
problem = minimalloc.Problem(buffers, capacity=1024)

# From CSV
problem = minimalloc.Problem.from_csv(csv_string, capacity=1024)
```

#### `Solution`
Contains the allocated offsets for each buffer.

```python
solution = minimalloc.Solution([0, 8, 16])  # Offsets for each buffer
print(solution[0])                          # Access by index
print(len(solution))                        # Number of buffers
```

#### `Solver`
Solves memory allocation problems.

```python
# Default solver
solver = minimalloc.Solver()

# With custom parameters
params = minimalloc.SolverParams(
    timeout_seconds=10.0,
    canonical_only=True,
    dynamic_ordering=True
)
solver = minimalloc.Solver(params)

solution = solver.solve(problem)
print(f"Backtracks: {solver.backtracks}")
```

#### `SolverParams`
Configuration for the solver algorithm.

```python
params = minimalloc.SolverParams(
    timeout_seconds=None,           # Time limit (None = no limit)
    canonical_only=True,            # Use canonical solutions only
    section_inference=True,         # Enable section inference
    dynamic_ordering=True,          # Dynamic buffer ordering
    check_dominance=True,           # Dominance checking
    unallocated_floor=True,         # Use unallocated floor bounds
    static_preordering=True,        # Static buffer preordering
    dynamic_decomposition=True,     # Dynamic decomposition
    monotonic_floor=True,           # Monotonic floor constraint
    hatless_pruning=True,           # Hatless pruning optimization
    preordering_heuristics=["WAT", "TAW", "TWA"]  # Heuristic order
)
```

### Utility Functions

#### `validate_solution(problem, solution)`
Validates that a solution is correct.

```python
is_valid = minimalloc.validate_solution(problem, solution)
assert is_valid == minimalloc.ValidationResult.GOOD
```

#### `solve_from_csv(csv_content, capacity, **solver_params)`
Convenience function for CSV-based solving.

```python
solution = minimalloc.solve_from_csv(
    csv_content, 
    capacity=1024,
    timeout_seconds=30.0,
    dynamic_ordering=False
)
```

## Advanced Features

### Buffers with Gaps

```python
# Buffer that's inactive from time 5-7
gap = minimalloc.Gap(5, 7)
buffer = minimalloc.Buffer("complex", 0, 10, 8, gaps=[gap])
```

### Fixed Offsets

```python
# Buffer placed at a specific offset
buffer = minimalloc.Buffer("fixed", 0, 5, 4, offset=64)
```

### Placement Hints

```python
# Suggest placement location to solver
buffer = minimalloc.Buffer("hinted", 0, 5, 4, hint=32)
```

## Examples

See the `python/examples/` directory for complete examples:

- `basic_usage.py` - Basic API usage
- `benchmark_example.py` - Using benchmark data

## Testing

```bash
# Run tests
pytest python/tests/

# Run with coverage
pytest python/tests/ --cov=minimalloc
```

## Performance Notes

- The Python bindings provide direct access to the C++ solver with minimal overhead
- For large problems, the solver time dominates any Python/C++ interface costs
- Memory usage is optimized through direct C++ object access

## License

Apache License 2.0 - see the [LICENSE](../LICENSE) file for details.