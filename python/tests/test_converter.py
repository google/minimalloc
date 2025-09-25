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

import minimalloc as mm


def test_to_csv() -> None:
    """Test basic CSV export functionality."""
    buffers = [
        mm.Buffer("0", mm.Lifespan(5, 10), size=15, hint=0),
        mm.Buffer(
            "1",
            mm.Lifespan(6, 12),
            size=18,
            alignment=2,
            gaps=[
                mm.Gap(mm.Lifespan(7, 8)),
                mm.Gap(mm.Lifespan(9, 10), mm.Window(1, 17)),
            ],
        ),
    ]
    problem = mm.Problem(buffers, capacity=40)

    expected_csv = "id,lower,upper,size,alignment,hint,gaps\n0,5,10,15,1,0,\n1,6,12,18,2,-1,7-8 9-10@1:17\n"

    assert mm.to_csv(problem) == expected_csv


def test_to_csv_without_alignment() -> None:
    """Test CSV export without alignment column."""
    buffers = [
        mm.Buffer("0", mm.Lifespan(5, 10), size=15, hint=0),
        mm.Buffer(
            "1",
            mm.Lifespan(6, 12),
            size=18,
            gaps=[
                mm.Gap(mm.Lifespan(7, 8)),
                mm.Gap(mm.Lifespan(9, 10)),
            ],
        ),
    ]
    problem = mm.Problem(buffers, capacity=40)

    expected_csv = (
        "id,lower,upper,size,hint,gaps\n0,5,10,15,0,\n1,6,12,18,-1,7-8 9-10\n"
    )

    assert mm.to_csv(problem) == expected_csv


def test_to_csv_without_hint() -> None:
    """Test CSV export without hint column."""
    buffers = [
        mm.Buffer("0", mm.Lifespan(5, 10), size=15),
        mm.Buffer(
            "1",
            mm.Lifespan(6, 12),
            size=18,
            alignment=2,
            gaps=[
                mm.Gap(mm.Lifespan(7, 8)),
                mm.Gap(mm.Lifespan(9, 10), mm.Window(1, 17)),
            ],
        ),
    ]
    problem = mm.Problem(buffers, capacity=40)

    expected_csv = "id,lower,upper,size,alignment,gaps\n0,5,10,15,1,\n1,6,12,18,2,7-8 9-10@1:17\n"

    assert mm.to_csv(problem) == expected_csv


def test_to_csv_without_gaps() -> None:
    """Test CSV export without gaps column."""
    buffers = [
        mm.Buffer("0", mm.Lifespan(5, 10), size=15, hint=0),
        mm.Buffer("1", mm.Lifespan(6, 12), size=18, alignment=2),
    ]
    problem = mm.Problem(buffers, capacity=40)

    expected_csv = (
        "id,lower,upper,size,alignment,hint\n0,5,10,15,1,0\n1,6,12,18,2,-1\n"
    )

    assert mm.to_csv(problem) == expected_csv


def test_to_csv_with_solution() -> None:
    """Test CSV export with solution offsets."""
    buffers = [
        mm.Buffer("0", mm.Lifespan(5, 10), size=15),
        mm.Buffer(
            "1",
            mm.Lifespan(6, 12),
            size=18,
            alignment=2,
            gaps=[
                mm.Gap(mm.Lifespan(7, 8)),
                mm.Gap(mm.Lifespan(9, 10)),
            ],
        ),
    ]
    problem = mm.Problem(buffers, capacity=40)
    solution = mm.Solution(offsets=[1, 21])

    expected_csv = "id,lower,upper,size,alignment,gaps,offset\n0,5,10,15,1,,1\n1,6,12,18,2,7-8 9-10,21\n"

    assert mm.to_csv(problem, solution) == expected_csv


def test_to_csv_weird_ids() -> None:
    """Test CSV export with numeric string IDs."""
    buffers = [
        mm.Buffer("10", mm.Lifespan(5, 10), size=15),
        mm.Buffer(
            "20",
            mm.Lifespan(6, 12),
            size=18,
            alignment=2,
            gaps=[
                mm.Gap(mm.Lifespan(7, 8)),
                mm.Gap(mm.Lifespan(9, 10)),
            ],
        ),
    ]
    problem = mm.Problem(buffers, capacity=40)

    expected_csv = "id,lower,upper,size,alignment,gaps\n10,5,10,15,1,\n20,6,12,18,2,7-8 9-10\n"

    assert mm.to_csv(problem) == expected_csv


def test_to_csv_string_ids() -> None:
    """Test CSV export with descriptive string IDs."""
    buffers = [
        mm.Buffer("Little", mm.Lifespan(5, 10), size=15),
        mm.Buffer(
            "Big",
            mm.Lifespan(6, 12),
            size=18,
            alignment=2,
            gaps=[
                mm.Gap(mm.Lifespan(7, 8)),
                mm.Gap(mm.Lifespan(9, 10)),
            ],
        ),
    ]
    problem = mm.Problem(buffers, capacity=40)

    expected_csv = "id,lower,upper,size,alignment,gaps\nLittle,5,10,15,1,\nBig,6,12,18,2,7-8 9-10\n"

    assert mm.to_csv(problem) == expected_csv


def test_to_csv_old_format() -> None:
    """Test CSV export in old format with start/end columns."""
    buffers = [
        mm.Buffer("Little", mm.Lifespan(5, 10), size=15),
        mm.Buffer(
            "Big",
            mm.Lifespan(6, 12),
            size=18,
            alignment=2,
            gaps=[
                mm.Gap(mm.Lifespan(7, 8)),
                mm.Gap(mm.Lifespan(9, 10)),
            ],
        ),
    ]
    problem = mm.Problem(buffers, capacity=40)

    expected_csv = "id,start,end,size,alignment,gaps\nLittle,5,9,15,1,\nBig,6,11,18,2,7-7 9-9\n"

    assert mm.to_csv(problem, old_format=True) == expected_csv


def test_from_csv_problem_only() -> None:
    """Test CSV import with basic problem data."""
    csv_data = "lower,size,id,upper\n6,18,1,12\n5,15,0,10\n"

    problem = mm.from_csv(csv_data)

    expected_buffers = [
        mm.Buffer("1", mm.Lifespan(6, 12), size=18),
        mm.Buffer("0", mm.Lifespan(5, 10), size=15),
    ]

    assert len(problem.buffers) == 2
    assert problem.capacity == 0
    assert problem.buffers == expected_buffers


def test_from_csv_with_alignment() -> None:
    """Test CSV import with alignment column."""
    csv_data = "begin,size,buffer,upper,alignment\n6,18,1,12,2\n5,15,0,10,1\n"

    problem = mm.from_csv(csv_data)

    assert len(problem.buffers) == 2
    assert problem.buffers[0].id == "1"
    assert problem.buffers[0].alignment == 2
    assert problem.buffers[1].id == "0"
    assert problem.buffers[1].alignment == 1


def test_from_csv_with_hints() -> None:
    """Test CSV import with hints column."""
    csv_data = "begin,size,buffer,upper,alignment,hint\n6,18,1,12,2,0\n5,15,0,10,1,-1\n"

    problem = mm.from_csv(csv_data)

    assert len(problem.buffers) == 2
    assert problem.buffers[0].id == "1"
    assert problem.buffers[0].hint == 0
    assert problem.buffers[1].id == "0"
    assert problem.buffers[1].hint is None


def test_from_csv_with_empty_gaps() -> None:
    """Test CSV import with empty gaps column."""
    csv_data = "start,size,buffer_id,upper,alignment,gaps\n6,18,1,12,2,\n5,15,0,10,1,\n"

    problem = mm.from_csv(csv_data)

    assert len(problem.buffers) == 2
    assert problem.buffers[0].id == "1"
    assert len(problem.buffers[0].gaps) == 0
    assert problem.buffers[1].id == "0"
    assert len(problem.buffers[1].gaps) == 0


def test_from_csv_with_gaps() -> None:
    """Test CSV import with gaps data."""
    csv_data = "start,size,buffer,upper,alignment,gaps\n6,18,1,12,2,7-9 \n5,15,0,10,1,9-11 12-14@2:13\n"

    problem = mm.from_csv(csv_data)

    assert len(problem.buffers) == 2

    # Check buffer 1 gaps
    buffer1 = problem.buffers[0]  # First buffer in CSV order
    assert buffer1.id == "1"
    assert len(buffer1.gaps) == 1
    assert buffer1.gaps[0].lifespan.lower == 7
    assert buffer1.gaps[0].lifespan.upper == 9

    # Check buffer 0 gaps
    buffer0 = problem.buffers[1]  # Second buffer in CSV order
    assert buffer0.id == "0"
    assert len(buffer0.gaps) == 2
    assert buffer0.gaps[0].lifespan.lower == 9
    assert buffer0.gaps[0].lifespan.upper == 11
    assert buffer0.gaps[1].lifespan.lower == 12
    assert buffer0.gaps[1].lifespan.upper == 14
    assert buffer0.gaps[1].window.lower == 2
    assert buffer0.gaps[1].window.upper == 13


def test_from_csv_with_end_column() -> None:
    """Test CSV import with end column (old format)."""
    csv_data = "start,size,buffer,end,alignment,gaps\n6,18,1,11,2,7-8 \n5,15,0,9,1,9-10 12-13\n"

    problem = mm.from_csv(csv_data)

    assert len(problem.buffers) == 2

    # Check that end column is converted to upper (end + 1)
    buffer1 = problem.buffers[0]
    assert buffer1.id == "1"
    assert buffer1.lifespan.upper == 12  # end=11 -> upper=12

    buffer0 = problem.buffers[1]
    assert buffer0.id == "0"
    assert buffer0.lifespan.upper == 10  # end=9 -> upper=10


def test_from_csv_with_solution() -> None:
    """Test CSV import with solution offsets."""
    csv_data = "start,size,offset,buffer,upper\n6,18,21,1,12\n5,15,1,0,10\n"

    problem = mm.from_csv(csv_data)

    assert len(problem.buffers) == 2
    assert problem.buffers[0].id == "1"
    assert problem.buffers[0].offset == 21
    assert problem.buffers[1].id == "0"
    assert problem.buffers[1].offset == 1


def test_from_csv_buffer_id() -> None:
    """Test CSV import with buffer_id column."""
    csv_data = "start,size,buffer_id,upper\n6,18,1,12\n5,15,0,10\n"

    problem = mm.from_csv(csv_data)

    assert len(problem.buffers) == 2
    assert problem.buffers[0].id == "1"
    assert problem.buffers[1].id == "0"


def test_from_csv_weird_ids() -> None:
    """Test CSV import with numeric string IDs."""
    csv_data = "start,size,buffer,upper\n6,18,20,12\n5,15,10,10\n"

    problem = mm.from_csv(csv_data)

    assert len(problem.buffers) == 2
    assert problem.buffers[0].id == "20"
    assert problem.buffers[1].id == "10"


def test_from_csv_string_ids() -> None:
    """Test CSV import with descriptive string IDs."""
    csv_data = "start,size,buffer,upper\n6,18,Big,12\n5,15,Little,10\n"

    problem = mm.from_csv(csv_data)

    assert len(problem.buffers) == 2
    assert problem.buffers[0].id == "Big"
    assert problem.buffers[1].id == "Little"


def test_from_csv_bogus_inputs() -> None:
    """Test CSV import with invalid numeric data."""
    csv_data = "start,size,buffer,upper\na,b,c,d\ne,f,g,h\n"

    assert mm.from_csv(csv_data) is None


def test_from_csv_bogus_offsets() -> None:
    """Test CSV import with invalid offset data."""
    csv_data = "start,size,offset,buffer,upper\n6,18,a,1,12\n5,15,b,0,10\n"

    assert mm.from_csv(csv_data) is None


def test_from_csv_bogus_gaps() -> None:
    """Test CSV import with invalid gaps format."""
    csv_data = "start,size,buffer,upper,gaps\n6,18,1,12,1-2-3\n5,15,0,10,\n"

    assert mm.from_csv(csv_data) is None


def test_from_csv_more_bogus_gaps() -> None:
    """Test CSV import with non-numeric gaps."""
    csv_data = "start,size,buffer,upper,gaps\n6,18,1,12,A-B\n5,15,0,10,\n"

    assert mm.from_csv(csv_data) is None


def test_from_csv_missing_column() -> None:
    """Test CSV import with missing required column."""
    csv_data = "start,size,upper\n6,18,1,12\n5,15,10\n"

    assert mm.from_csv(csv_data) is None


def test_from_csv_duplicate_column() -> None:
    """Test CSV import with duplicate column names."""
    csv_data = (
        "start,size,offset,buffer,upper,upper\n6,18,21,1,12\n5,15,1,0,10\n"
    )

    assert mm.from_csv(csv_data) is None


def test_from_csv_extra_fields() -> None:
    """Test CSV import with extra fields in data rows."""
    csv_data = "start,size,offset,buffer,upper\n6,18,21,1,12\n5,15,1,0,10,100\n"

    assert mm.from_csv(csv_data) is None
