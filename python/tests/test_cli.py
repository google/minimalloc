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

import shlex
from pathlib import Path

import pytest
from minimalloc import cli


@pytest.fixture
def input_csv_example(tmp_path: Path) -> Path:
    csv_file = tmp_path / "input_example.csv"
    csv_file.write_text(
        "id,lower,upper,size\nb1,0,3,4\nb2,3,9,4\nb3,0,9,4\nb4,9,21,4\nb5,0,21,4\n"
    )
    return csv_file


@pytest.fixture
def input_csv_invalid(tmp_path: Path) -> Path:
    csv_file = tmp_path / "input_invalid.csv"
    csv_file.write_text("invalid_header1,invalid_header2\nvalue1,value2\n")
    return csv_file


def test_success(tmp_path: Path, input_csv_example: Path, capsys) -> None:  # type: ignore
    output_file = tmp_path / "output.csv"
    command = (
        f"--input {input_csv_example} --output {output_file} --capacity 12"
    )

    exit_code = cli.main(shlex.split(command))
    captured = capsys.readouterr()

    assert exit_code == 0
    assert "Elapsed time:" in captured.err
    assert output_file.exists()


def test_validation(tmp_path: Path, input_csv_example: Path, capsys) -> None:  # type: ignore
    output_file = tmp_path / "output.csv"
    command = f"--input {input_csv_example} --output {output_file} --capacity 12 --validate"

    exit_code = cli.main(shlex.split(command))
    captured = capsys.readouterr()

    assert exit_code == 0
    assert "PASS" in captured.err or "FAIL" in captured.err


def test_invalid_csv(tmp_path: Path, input_csv_invalid: Path, capsys) -> None:  # type: ignore
    output_file = tmp_path / "output.csv"
    command = (
        f"--input {input_csv_invalid} --output {output_file} --capacity 12"
    )

    exit_code = cli.main(shlex.split(command))
    captured = capsys.readouterr()

    assert exit_code != 0
    assert "Error" in captured.err
    assert not output_file.exists()


def test_invalid_capacity(  # type: ignore
    tmp_path: Path,
    input_csv_example: Path,
    capsys,
) -> None:
    output_file = tmp_path / "output.csv"
    command = f"--input {input_csv_example} --output {output_file} --capacity 1"

    exit_code = cli.main(shlex.split(command))
    captured = capsys.readouterr()

    assert exit_code != 0
    assert "Error" in captured.err
    assert not output_file.exists()


def test_minimize_capacity(  # type: ignore
    tmp_path: Path,
    input_csv_example: Path,
    capsys,
) -> None:
    output_file = tmp_path / "output.csv"
    command = f"--input {input_csv_example} --output {output_file} --minimize-capacity"

    exit_code = cli.main(shlex.split(command))
    captured = capsys.readouterr()

    assert exit_code == 0
    assert "Elapsed time:" in captured.err
    assert output_file.exists()
