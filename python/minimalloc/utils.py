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

from . import _minimalloc as mm


def from_csv_file(path: str) -> mm.Problem:
    """Load a Problem instance from a CSV file."""
    with open(path) as f:
        return mm.from_csv(f.read())


def to_csv_file(problem: mm.Problem, path: str) -> None:
    """Save a Problem instance to a CSV file."""
    with open(path, "w") as f:
        f.write(mm.to_csv(problem))
