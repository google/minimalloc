#!/usr/bin/env python3

"""
Basic usage example for MiniMalloc Python bindings.

This example demonstrates how to:
1. Create buffers with lifespans
2. Set up a memory allocation problem  
3. Solve the allocation
4. Validate and inspect results
"""

import minimalloc

def main():
    print("=== MiniMalloc Python API Example ===\n")
    
    # Create buffers manually
    print("1. Creating buffers:")
    buffers = [
        minimalloc.Buffer("b1", lower=0, upper=3, size=4),
        minimalloc.Buffer("b2", lower=3, upper=9, size=4), 
        minimalloc.Buffer("b3", lower=0, upper=9, size=4),
        minimalloc.Buffer("b4", lower=9, upper=21, size=4),
        minimalloc.Buffer("b5", lower=0, upper=21, size=4),
    ]
    
    for buffer in buffers:
        print(f"  {buffer}")
    
    # Create problem with capacity constraint
    print(f"\n2. Creating problem with capacity=12:")
    problem = minimalloc.Problem(buffers, capacity=12)
    print(f"  {problem}")
    
    # Configure solver parameters
    print("\n3. Configuring solver:")
    params = minimalloc.SolverParams(
        timeout_seconds=10.0,
        canonical_only=True,
        dynamic_ordering=True
    )
    solver = minimalloc.Solver(params)
    print("  Solver configured with default parameters")
    
    # Solve the allocation problem
    print("\n4. Solving allocation:")
    try:
        solution = solver.solve(problem)
        print(f"  Solution found after {solver.backtracks} backtracks")
        print(f"  {solution}")
        
        # Show detailed allocation
        print("\n5. Detailed allocation:")
        for i, buffer in enumerate(problem.buffers):
            offset = solution.offsets[i]
            print(f"  {buffer.id}: offset={offset}, lifespan=[{buffer.lifespan.lower}, {buffer.lifespan.upper}), size={buffer.size}")
        
        # Validate solution
        print("\n6. Validating solution:")
        is_valid = minimalloc.validate_solution(problem, solution)
        print(f"  Solution is {'valid' if is_valid == minimalloc.ValidationResult.GOOD else 'invalid'}")
        
    except RuntimeError as e:
        print(f"  Failed to solve: {e}")

def csv_example():
    print("\n=== CSV Interface Example ===\n")
    
    # Example CSV data (same as benchmarks/examples/input.12.csv)
    csv_data = """id,lower,upper,size
b1,0,3,4
b2,3,9,4
b3,0,9,4
b4,9,21,4
b5,0,21,4"""
    
    print("1. Loading problem from CSV:")
    print(csv_data)
    
    # Parse CSV and solve
    print("\n2. Solving from CSV:")
    try:
        solution = minimalloc.solve_from_csv(csv_data, capacity=12)
        print(f"  {solution}")
        
        # Convert back to CSV for output
        problem = minimalloc.Problem.from_csv(csv_data, capacity=12)
        output_csv = problem.to_csv(solution)
        print("\n3. Output CSV:")
        print(output_csv)
        
    except RuntimeError as e:
        print(f"  Failed to solve: {e}")

def advanced_example():
    print("\n=== Advanced Features Example ===\n")
    
    # Buffer with gaps (inactive periods)
    print("1. Creating buffer with gaps:")
    gap = minimalloc.Gap(lower=5, upper=7)  # Inactive from time 5-7
    buffer_with_gap = minimalloc.Buffer(
        "complex", lower=0, upper=10, size=8, gaps=[gap]
    )
    print(f"  {buffer_with_gap}")
    
    # Buffer with fixed offset
    print("\n2. Creating buffer with fixed offset:")
    fixed_buffer = minimalloc.Buffer(
        "fixed", lower=0, upper=5, size=4, offset=8
    )
    print(f"  {fixed_buffer}")
    
    # Buffer with placement hint
    print("\n3. Creating buffer with placement hint:")
    hinted_buffer = minimalloc.Buffer(
        "hinted", lower=5, upper=10, size=4, hint=4
    )
    print(f"  {hinted_buffer}")

if __name__ == "__main__":
    main()
    csv_example() 
    advanced_example()