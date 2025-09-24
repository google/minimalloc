#!/usr/bin/env python3

"""
Example using the benchmark data from the repository.
"""

import os
import time
import minimalloc

def run_benchmark_example():
    """Run the example from benchmarks/examples/input.12.csv"""
    
    # Read the example CSV file
    csv_path = "../../benchmarks/examples/input.12.csv"
    if not os.path.exists(csv_path):
        print(f"Benchmark file not found: {csv_path}")
        return
    
    with open(csv_path, 'r') as f:
        csv_data = f.read()
    
    print("=== MiniMalloc Benchmark Example ===\n")
    print("Input CSV:")
    print(csv_data)
    
    # Parse and solve
    print("\nSolving allocation problem...")
    start_time = time.time()
    
    try:
        # Create problem from CSV
        problem = minimalloc.Problem.from_csv(csv_data, capacity=12)
        print(f"Problem: {len(problem.buffers)} buffers, capacity={problem.capacity}")
        
        # Solve with default parameters
        solver = minimalloc.Solver()
        solution = solver.solve(problem)
        
        solve_time = time.time() - start_time
        
        print(f"\nSolution found in {solve_time:.3f} seconds")
        print(f"Backtracks: {solver.backtracks}")
        print(f"Solution: {solution}")
        
        # Validate
        validation = minimalloc.validate_solution(problem, solution)
        is_valid = validation == minimalloc.ValidationResult.GOOD
        print(f"Validation: {'PASS' if is_valid else 'FAIL'}")
        
        # Show detailed results
        print("\nDetailed allocation:")
        for i, buffer in enumerate(problem.buffers):
            offset = solution.offsets[i]
            end_offset = offset + buffer.size
            print(f"  {buffer.id}: [{buffer.lifespan.lower:2d}, {buffer.lifespan.upper:2d}) "
                  f"size={buffer.size} → offset={offset:2d} (ends at {end_offset})")
        
        # Generate output CSV
        output_csv = problem.to_csv(solution)
        print(f"\nOutput CSV:")
        print(output_csv)
        
    except Exception as e:
        print(f"Error: {e}")

def compare_solver_configurations():
    """Compare different solver configurations."""
    
    csv_data = """id,lower,upper,size
b1,0,3,4
b2,3,9,4
b3,0,9,4
b4,9,21,4
b5,0,21,4"""
    
    problem = minimalloc.Problem.from_csv(csv_data, capacity=12)
    
    configurations = [
        ("Default", minimalloc.SolverParams()),
        ("No Canonical", minimalloc.SolverParams(canonical_only=False)),
        ("No Dynamic Ordering", minimalloc.SolverParams(dynamic_ordering=False)),
        ("No Section Inference", minimalloc.SolverParams(section_inference=False)),
        ("Minimal Features", minimalloc.SolverParams(
            canonical_only=False, 
            dynamic_ordering=False,
            section_inference=False,
            check_dominance=False
        )),
    ]
    
    print("\n=== Solver Configuration Comparison ===\n")
    
    for name, params in configurations:
        print(f"Testing {name}:")
        try:
            solver = minimalloc.Solver(params)
            start_time = time.time()
            solution = solver.solve(problem)
            solve_time = time.time() - start_time
            
            print(f"  Time: {solve_time:.3f}s, Backtracks: {solver.backtracks}")
            
        except Exception as e:
            print(f"  Failed: {e}")
        print()

if __name__ == "__main__":
    run_benchmark_example()
    compare_solver_configurations()