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

from pathlib import Path

import minimalloc as mm


def test_store_csv_file(tmp_path: Path) -> None:
    """Test storing a Problem instance to a CSV file."""
    problem = mm.Problem(
        buffers=[
            mm.Buffer("a", mm.Lifespan(0, 2), size=4),
            mm.Buffer("b", mm.Lifespan(1, 3), size=5),
            mm.Buffer("c", mm.Lifespan(3, 5), size=6),
        ],
        capacity=10,
    )

    csv_file = tmp_path / "test.csv"
    mm.to_csv_file(csv_file, problem)
    content = csv_file.read_text()

    expected_csv = "id,lower,upper,size\na,0,2,4\nb,1,3,5\nc,3,5,6\n"
    assert content == expected_csv


def test_read_csv_file(tmp_path: Path) -> None:
    """Test reading a Problem instance from a CSV file."""
    csv_content = "id,lower,upper,size\nx,10,20,100\ny,15,25,200\nz,30,40,150\n"

    csv_file = tmp_path / "input.csv"
    csv_file.write_text(csv_content)

    problem = mm.from_csv_file(csv_file)

    assert problem is not None
    assert len(problem.buffers) == 3

    assert problem.buffers[0].id == "x"
    assert problem.buffers[0].lifespan == mm.Lifespan(10, 20)
    assert problem.buffers[0].size == 100

    assert problem.buffers[1].id == "y"
    assert problem.buffers[1].lifespan == mm.Lifespan(15, 25)
    assert problem.buffers[1].size == 200

    assert problem.buffers[2].id == "z"
    assert problem.buffers[2].lifespan == mm.Lifespan(30, 40)
    assert problem.buffers[2].size == 150


def test_roundtrip_csv(tmp_path: Path) -> None:
    """Test roundtrip conversion of Problem instance to/from CSV."""
    original = mm.Problem(
        buffers=[
            mm.Buffer("a", mm.Lifespan(0, 2), size=4),
            mm.Buffer("b", mm.Lifespan(1, 3), size=5),
            mm.Buffer("c", mm.Lifespan(3, 5), size=6),
        ],
        capacity=10,
    )

    csv_file = tmp_path / "test.csv"
    mm.to_csv_file(csv_file, original)
    result = mm.from_csv_file(csv_file)

    assert result is not None
    assert result.capacity == 0  # Capacity is not stored in CSV
    for b1, b2 in zip(result.buffers, original.buffers, strict=True):
        assert b1.id == b2.id
        assert b1.lifespan == b2.lifespan
        assert b1.size == b2.size
