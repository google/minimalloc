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

from typing import TYPE_CHECKING

from ._minimalloc import (
    Buffer,
    BufferData,
    Duration,
    Gap,
    Interval,
    Lifespan,
    Overlap,
    Partition,
    PreorderData,
    PreorderingComparator,
    Problem,
    SectionRange,
    SectionSpan,
    Solution,
    Solver,
    SolverParams,
    SweepPoint,
    SweepPointType,
    SweepResult,
    ValidationResult,
    Window,
    create_points,
    from_csv,
    sweep,
    to_csv,
    validate,
)
from .utils import from_csv_file, to_csv_file

__version__ = "0.1.0"

if TYPE_CHECKING:
    from typing import NewType

    BufferIdx = NewType("BufferIdx", int)
    Capacity = NewType("Capacity", int)
    Offset = NewType("Offset", int)
    TimeValue = NewType("TimeValue", int)
    Area = NewType("Area", int)
    SectionIdx = NewType("SectionIdx", int)
    CutCount = NewType("CutCount", int)


else:
    BufferIdx = int
    Capacity = int
    Offset = int
    TimeValue = int
    Area = int
    SectionIdx = int
    CutCount = int


__all__ = [
    "Area",
    "Buffer",
    "BufferData",
    "BufferIdx",
    "Capacity",
    "CutCount",
    "Duration",
    "Gap",
    "Interval",
    "Lifespan",
    "Offset",
    "Overlap",
    "Partition",
    "PreorderData",
    "PreorderingComparator",
    "Problem",
    "SectionIdx",
    "SectionRange",
    "SectionSpan",
    "Solution",
    "Solver",
    "SolverParams",
    "SweepPoint",
    "SweepPointType",
    "SweepResult",
    "TimeValue",
    "ValidationResult",
    "Window",
    "create_points",
    "from_csv",
    "from_csv_file",
    "sweep",
    "to_csv",
    "to_csv_file",
    "validate",
]
