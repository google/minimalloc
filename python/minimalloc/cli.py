#!/usr/bin/env python3
"""
Copyright 2023 Google LLC
Copyright 2025 Axelera AI

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

import argparse
import sys
import time
from pathlib import Path

import minimalloc as mm


def _parse_duration(duration_str: str) -> float | None:
    """Parse a duration string (e.g., '10s', '5m', '1h') to seconds."""
    if not duration_str:
        return None

    duration_str = duration_str.strip().lower()

    if duration_str in ["inf", "infinite", "infinity"]:
        return None

    try:
        return float(duration_str)
    except ValueError:
        pass

    multipliers = {"s": 1, "m": 60, "h": 3600}

    for suffix, mult in multipliers.items():
        if duration_str.endswith(suffix):
            try:
                return float(duration_str[: -len(suffix)]) * mult
            except ValueError:
                pass

    raise ValueError(f"Invalid duration format: {duration_str}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="MiniMalloc: State-of-the-art static memory allocation solver"
    )

    parser.add_argument(
        "--capacity", type=int, default=0, help="The maximum memory capacity"
    )
    parser.add_argument(
        "--input",
        type=str,
        required=True,
        help="The path to the input CSV file",
    )
    parser.add_argument(
        "--output",
        type=str,
        required=True,
        help="The path to the output CSV file",
    )
    parser.add_argument(
        "--timeout",
        type=str,
        default="",
        help="The time limit for the solver (e.g., '10s', '5m', '1h', 'inf')",
    )
    parser.add_argument(
        "--validate", action="store_true", help="Validates the solver's output"
    )

    parser.add_argument(
        "--canonical-only",
        action="store_true",
        default=True,
        help="Explores canonical solutions only (default: True)",
    )
    parser.add_argument(
        "--no-canonical-only",
        dest="canonical_only",
        action="store_false",
        help="Disable canonical solutions exploration",
    )
    parser.add_argument(
        "--section-inference",
        action="store_true",
        default=True,
        help="Performs advanced inference (default: True)",
    )
    parser.add_argument(
        "--no-section-inference",
        dest="section_inference",
        action="store_false",
        help="Disable section inference",
    )
    parser.add_argument(
        "--dynamic-ordering",
        action="store_true",
        default=True,
        help="Dynamically orders buffers (default: True)",
    )
    parser.add_argument(
        "--no-dynamic-ordering",
        dest="dynamic_ordering",
        action="store_false",
        help="Disable dynamic ordering",
    )
    parser.add_argument(
        "--check-dominance",
        action="store_true",
        default=True,
        help="Checks for dominated solutions that leave gaps (default: True)",
    )
    parser.add_argument(
        "--no-check-dominance",
        dest="check_dominance",
        action="store_false",
        help="Disable dominance checking",
    )
    parser.add_argument(
        "--unallocated-floor",
        action="store_true",
        default=True,
        help="Uses min offsets for lower bounds on section floors (default: True)",
    )
    parser.add_argument(
        "--no-unallocated-floor",
        dest="unallocated_floor",
        action="store_false",
        help="Disable unallocated floor optimization",
    )
    parser.add_argument(
        "--static-preordering",
        action="store_true",
        default=True,
        help="Statically preorders buffers (default: True)",
    )
    parser.add_argument(
        "--no-static-preordering",
        dest="static_preordering",
        action="store_false",
        help="Disable static preordering",
    )
    parser.add_argument(
        "--dynamic-decomposition",
        action="store_true",
        default=True,
        help="Dynamically decomposes buffers (default: True)",
    )
    parser.add_argument(
        "--no-dynamic-decomposition",
        dest="dynamic_decomposition",
        action="store_false",
        help="Disable dynamic decomposition",
    )
    parser.add_argument(
        "--monotonic-floor",
        action="store_true",
        default=True,
        help="Requires monotonic solution floor increase (default: True)",
    )
    parser.add_argument(
        "--no-monotonic-floor",
        dest="monotonic_floor",
        action="store_false",
        help="Disable monotonic floor requirement",
    )
    parser.add_argument(
        "--hatless-pruning",
        action="store_true",
        default=True,
        help="Prunes alternate solutions when buffer has nothing overhead (default: True)",
    )
    parser.add_argument(
        "--no-hatless-pruning",
        dest="hatless_pruning",
        action="store_false",
        help="Disable hatless pruning",
    )

    parser.add_argument(
        "--preordering-heuristics",
        type=str,
        default="WAT,TAW,TWA",
        help="Static preordering heuristics to attempt (comma-separated)",
    )

    args = parser.parse_args()

    try:
        problem = mm.from_csv_file(args.input)
    except Exception as e:
        print(f"Error reading input file: {e}", file=sys.stderr)
        return 1

    if args.capacity > 0:
        problem.capacity = args.capacity

    params = mm.SolverParams()
    params.timeout = _parse_duration(args.timeout)
    params.canonical_only = args.canonical_only
    params.section_inference = args.section_inference
    params.dynamic_ordering = args.dynamic_ordering
    params.check_dominance = args.check_dominance
    params.unallocated_floor = args.unallocated_floor
    params.static_preordering = args.static_preordering
    params.dynamic_decomposition = args.dynamic_decomposition
    params.monotonic_floor = args.monotonic_floor
    params.hatless_pruning = args.hatless_pruning

    preordering_heuristics = [
        h.strip() for h in args.preordering_heuristics.split(",") if h.strip()
    ]
    params.preordering_heuristics = preordering_heuristics

    start_time = time.time()
    try:
        solver = mm.Solver(params)
        solution = solver.solve(problem)
    except Exception as e:
        print(f"Solver failed: {e}", file=sys.stderr)
        return 1
    end_time = time.time()
    elapsed_time = end_time - start_time
    print(f"Elapsed time: {elapsed_time:.3f}s", file=sys.stderr)

    if args.validate:
        validation_result = mm.validate(problem, solution)
        result_str = (
            "PASS" if validation_result == mm.ValidationResult.GOOD else "FAIL"
        )
        print(result_str, file=sys.stderr)

    try:
        output_path = Path(args.output)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, "w") as f:
            f.write(mm.to_csv(problem, solution))
    except Exception as e:
        print(f"Error writing output file: {e}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
