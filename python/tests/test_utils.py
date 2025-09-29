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

import tempfile

import minimalloc as mm


def test_csv_roundtrip() -> None:
    """Test roundtrip conversion of Problem instance to/from CSV."""
    original = mm.Problem(
        buffers=[
            mm.Buffer("a", mm.Lifespan(0, 2), size=4),
            mm.Buffer("b", mm.Lifespan(1, 3), size=5),
            mm.Buffer("c", mm.Lifespan(3, 5), size=6),
        ],
        capacity=10,
    )

    # Convert to CSV and back
    with tempfile.NamedTemporaryFile(mode="w+", delete=True) as tmpfile:
        mm.to_csv_file(original, tmpfile.name)
        tmpfile.flush()
        result = mm.from_csv_file(tmpfile.name)

    assert result.capacity == 0  # Capacity is not stored in CSV
    for b1, b2 in zip(result.buffers, original.buffers, strict=True):
        assert b1.id == b2.id
        assert b1.lifespan == b2.lifespan
        assert b1.size == b2.size
