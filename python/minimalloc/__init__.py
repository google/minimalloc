"""
MiniMalloc: A lightweight memory allocator for hardware-accelerated machine learning.

This package provides Python bindings for the MiniMalloc C++ library, offering
efficient static memory allocation algorithms designed for ML compiler workloads.
"""

from ._core import (
    Buffer,
    # Gap,
    # Problem,
    # Solution,
    # Solver,
    # SolverParams,
    # validate_solution,
)

__version__ = "1.0.0"
__all__ = [
    "Buffer",
    # "Gap", 
    # "Problem",
    # "Solution",
    # "Solver",
    # "SolverParams",
    # "validate_solution",
    # "solve_from_csv",
]

# def solve_from_csv(csv_content: str, capacity: int, **solver_params) -> Solution:
#     """
#     Convenience function to solve memory allocation from CSV data.
    
#     Args:
#         csv_content: CSV string with columns: id,lower,upper,size
#         capacity: Maximum memory capacity
#         **solver_params: Optional solver parameters
    
#     Returns:
#         Solution object with allocated offsets
#     """
#     problem = Problem.from_csv(csv_content, capacity)
#     params = SolverParams(**solver_params) if solver_params else SolverParams()
#     solver = Solver(params)
#     return solver.solve(problem)