#!/usr/bin/env python3

"""
Basic tests for MiniMalloc Python bindings.
"""

import pytest
import minimalloc

class TestBuffer:
    def test_buffer_creation(self):
        buffer = minimalloc.Buffer("test", lower=0, upper=10, size=8)
        assert buffer.id == "test"
        assert buffer.lifespan.lower == 0
        assert buffer.lifespan.upper == 10
        assert buffer.size == 8
        assert buffer.alignment == 1
        assert buffer.offset is None
        assert buffer.hint is None
        assert len(buffer.gaps) == 0
    
    def test_buffer_with_options(self):
        gap = minimalloc.Gap(5, 7)
        buffer = minimalloc.Buffer(
            "complex", 
            lower=0, upper=10, size=8,
            alignment=4, offset=16, hint=8,
            gaps=[gap]
        )
        assert buffer.alignment == 4
        assert buffer.offset == 16
        assert buffer.hint == 8
        assert len(buffer.gaps) == 1
        assert buffer.gaps[0].lifespan.lower == 5
        assert buffer.gaps[0].lifespan.upper == 7
    
    def test_buffer_area(self):
        buffer = minimalloc.Buffer("test", lower=0, upper=10, size=8)
        assert buffer.area() == 80  # size * lifespan_length
    
    def test_effective_size(self):
        buffer_a = minimalloc.Buffer("a", lower=0, upper=2, size=4)
        buffer_b = minimalloc.Buffer("b", lower=1, upper=3, size=5)
        
        # Overlapping buffers
        assert buffer_a.effective_size(buffer_b) == 4
        assert buffer_b.effective_size(buffer_a) == 5
        
        # Non-overlapping buffers
        buffer_c = minimalloc.Buffer("c", lower=3, upper=5, size=6)
        assert buffer_a.effective_size(buffer_c) is None

class TestGap:
    def test_gap_creation(self):
        gap = minimalloc.Gap(5, 10)
        assert gap.lifespan.lower == 5
        assert gap.lifespan.upper == 10
        assert gap.window is None
    
    def test_gap_with_window(self):
        gap = minimalloc.Gap(5, 10, window_lower=2, window_upper=6)
        assert gap.lifespan.lower == 5
        assert gap.lifespan.upper == 10
        assert gap.window is not None
        assert gap.window.lower == 2
        assert gap.window.upper == 6

class TestProblem:
    def test_problem_creation(self):
        buffers = [
            minimalloc.Buffer("b1", 0, 3, 4),
            minimalloc.Buffer("b2", 3, 9, 4),
        ]
        problem = minimalloc.Problem(buffers, capacity=12)
        assert len(problem.buffers) == 2
        assert problem.capacity == 12
        assert len(problem) == 2
        assert problem[0].id == "b1"
    
    def test_from_csv(self):
        csv_data = """id,lower,upper,size
b1,0,3,4
b2,3,9,4"""
        problem = minimalloc.Problem.from_csv(csv_data, capacity=12)
        assert len(problem.buffers) == 2
        assert problem.capacity == 12
        assert problem.buffers[0].id == "b1"
        assert problem.buffers[1].id == "b2"
    
    def test_invalid_csv(self):
        with pytest.raises(RuntimeError):
            minimalloc.Problem.from_csv("invalid,csv,data", capacity=12)

class TestSolution:
    def test_solution_creation(self):
        solution = minimalloc.Solution([0, 4, 8])
        assert len(solution) == 3
        assert solution[0] == 0
        assert solution[1] == 4
        assert solution[2] == 8
        assert solution.offsets == [0, 4, 8]
    
    def test_solution_indexing(self):
        solution = minimalloc.Solution([10, 20, 30])
        assert solution[0] == 10
        assert solution[1] == 20
        assert solution[2] == 30
        
        with pytest.raises(IndexError):
            _ = solution[3]

class TestSolver:
    def test_solver_creation(self):
        solver = minimalloc.Solver()
        assert solver.backtracks == 0
    
    def test_solver_with_params(self):
        params = minimalloc.SolverParams(
            timeout_seconds=5.0,
            canonical_only=False,
            dynamic_ordering=False
        )
        solver = minimalloc.Solver(params)
        assert solver.backtracks == 0
    
    def test_solve_simple_problem(self):
        # Create a simple problem that should be solvable
        buffers = [
            minimalloc.Buffer("b1", 0, 2, 4),
            minimalloc.Buffer("b2", 2, 4, 4),
        ]
        problem = minimalloc.Problem(buffers, capacity=8)
        
        solver = minimalloc.Solver()
        solution = solver.solve(problem)
        
        assert len(solution) == 2
        assert all(offset >= 0 for offset in solution.offsets)
        assert all(offset + buffers[i].size <= problem.capacity 
                  for i, offset in enumerate(solution.offsets))
    
    def test_solve_infeasible_problem(self):
        # Create a problem that cannot be solved (capacity too small)
        buffers = [
            minimalloc.Buffer("b1", 0, 10, 100),  # Very large buffer
            minimalloc.Buffer("b2", 0, 10, 100),  # Another large buffer
        ]
        problem = minimalloc.Problem(buffers, capacity=10)  # Too small capacity
        
        solver = minimalloc.Solver()
        with pytest.raises(RuntimeError):
            solver.solve(problem)

class TestValidation:
    def test_validate_good_solution(self):
        buffers = [
            minimalloc.Buffer("b1", 0, 2, 4),
            minimalloc.Buffer("b2", 2, 4, 4),
        ]
        problem = minimalloc.Problem(buffers, capacity=8)
        solution = minimalloc.Solution([0, 4])  # Valid non-overlapping allocation
        
        result = minimalloc.validate_solution(problem, solution)
        assert result == minimalloc.ValidationResult.GOOD

class TestHighLevelAPI:
    def test_solve_from_csv(self):
        csv_data = """id,lower,upper,size
b1,0,2,4
b2,2,4,4"""
        
        solution = minimalloc.solve_from_csv(csv_data, capacity=8)
        assert len(solution) == 2
        assert all(offset >= 0 for offset in solution.offsets)

if __name__ == "__main__":
    pytest.main([__file__])