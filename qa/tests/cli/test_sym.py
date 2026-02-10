#
# This Aogram source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#

from pathlib import Path
import pytest
import re

from conftest import KiTestFixture
import utils


def detect_indent(file_content: str) -> str:

    indent_regex = re.compile(r"^(?P<indent>\s+)", re.MULTILINE)
    match = indent_regex.search(file_content)
    if match:
        return match.group("indent")

    # Default to a tab if no symbols are found (or if the file is empty)
    return "\t"


def count_symbols_in_kicad_sym_file(file_path: Path) -> int:
    """
    Count the number of symbols in a .kicad_sym file by counting occurrences of "(symbol " at the start of lines.
    This is a simple heuristic that should work for normally-formatted .kicad_sym files.

    A more robust approach would be to parse the file as an S-expression and
    count the number of "symbol" entries.
    """
    with open(file_path, "r", encoding="utf-8") as f:
        data = f.read()

    indent = detect_indent(data)
    sym_regex = re.compile(rf'^{indent}\(symbol "', re.MULTILINE)
    return len(sym_regex.findall(data))


def assert_files_are_identical(file1: Path, file2: Path) -> None:
    with open(file1, "r", encoding="utf-8") as f1, open(
        file2, "r", encoding="utf-8"
    ) as f2:
        assert f1.read() == f2.read()


def get_sym_upgrade_cmd(input: str, output: str) -> list[str]:
    return [
        utils.kicad_cli(),
        "sym",
        "upgrade",
        "--force",
        str(input),
        "--output",
        str(output),
    ]


@pytest.mark.parametrize(
    "test_file,output_dir,num_syms",
    [
        # This is a v9-formatted library with 3 symbols
        (
            "cli/sym_lib_test/Amplifier_Video.v9.kicad_sym",
            "sym_lib_test",
            3,
        ),
    ],
)
def test_sym_upgrade_lib_pack_roundtrip(
    kitest: KiTestFixture,
    test_file: str,
    output_dir: str,
    num_syms: int,
) -> None:
    """
    Perform a roundtrip test of the "sym upgrade" command:
      - Unpack a .kicad_sym to a directory
      - Repack the directory to a .kicad_sym
      - Check the repacked .kicad_sym against the original
      - Upgradew the original .kicad_sym directly to another .kicad_sym and check that it's identical to the repacked one
      - Unpack the unpacked directory to another directory and check that the files are identical
    """

    input_file = kitest.get_data_file_path(test_file)
    output_path = kitest.get_output_path(f"cli/{output_dir}/")

    unpacked_lib_path = Path(output_path) / (
        Path(input_file).stem + ".unpacked.kicad_symdir"
    )

    # Create output directory if it doesn't exist
    unpacked_lib_path.parent.mkdir(exist_ok=True)

    unpack_cmd = get_sym_upgrade_cmd(
        str(input_file),
        str(unpacked_lib_path) + "/",
    )
    stdout, stderr, exitcode = utils.run_and_capture(unpack_cmd)

    assert exitcode == 0
    assert stderr == ""
    assert stdout is not None

    # Check that the output is a directory
    assert unpacked_lib_path.is_dir()

    # Check that the number of symbols in the unpacked directory matches the expected count
    sym_files = list(unpacked_lib_path.glob("*.kicad_sym"))
    assert len(sym_files) == num_syms

    repacked_lib_path = Path(output_path) / (
        Path(input_file).stem + ".repacked.kicad_sym"
    )

    if repacked_lib_path.exists():
        repacked_lib_path.unlink()

    repack_cmd = get_sym_upgrade_cmd(
        str(unpacked_lib_path),
        str(repacked_lib_path),
    )
    stdout, stderr, exitcode = utils.run_and_capture(repack_cmd)

    assert exitcode == 0
    assert stderr == ""
    assert stdout is not None

    # Check that the repacked file exists as a file
    assert repacked_lib_path.is_file()

    num_syms_in_repacked = count_symbols_in_kicad_sym_file(repacked_lib_path)
    assert num_syms_in_repacked == num_syms

    # Now, check that the packed file can be upgraded directly to another packed file
    packed_to_packed_output_path = Path(output_path) / (
        Path(input_file).stem + ".packed_to_packed.kicad_sym"
    )

    if packed_to_packed_output_path.exists():
        packed_to_packed_output_path.unlink()

    packed_to_packed_command = get_sym_upgrade_cmd(
        str(input_file),
        str(packed_to_packed_output_path),
    )
    stdout, stderr, exitcode = utils.run_and_capture(packed_to_packed_command)

    assert exitcode == 0
    assert stderr == ""
    assert stdout is not None

    assert packed_to_packed_output_path.is_file()

    # This file should be identical to the repacked file
    assert_files_are_identical(repacked_lib_path, packed_to_packed_output_path)

    # Finally, check unpacking to unpacked lib again

    unpacked_to_unpacked_output_path = Path(output_path) / (
        Path(input_file).stem + ".unpacked_to_unpacked.kicad_symdir"
    )

    unpacked_to_unpacked_cmd = get_sym_upgrade_cmd(
        str(unpacked_lib_path),
        str(unpacked_to_unpacked_output_path) + "/",
    )
    stdout, stderr, exitcode = utils.run_and_capture(unpacked_to_unpacked_cmd)

    assert exitcode == 0
    assert stderr == ""
    assert stdout is not None

    assert unpacked_to_unpacked_output_path.is_dir()

    pytest.skip("The following checks are currently failing: https://gitlab.com/kicad/code/kicad/-/issues/22988.")

    dir_a_files = sorted(unpacked_lib_path.glob("*.kicad_sym"))
    dir_b_files = sorted(unpacked_to_unpacked_output_path.glob("*.kicad_sym"))

    assert len(dir_a_files) == len(dir_b_files) == num_syms

    for file_a, file_b in zip(dir_a_files, dir_b_files):
        assert_files_are_identical(file_a, file_b)


@pytest.mark.parametrize(
    "input_lib,output_dir,num_units",
    [
        # This is a v9-formatted library with 3 symbols
        (
            "cli/sym_lib_test/Amplifier_Video.v9.kicad_sym",
            "sym_lib_test",
            4 + 1 + 1,
        ),
    ],
)
def test_sym_export_svg(
    kitest: KiTestFixture,
    input_lib: str,
    output_dir: str,
    num_units: int,
) -> None:
    """
    Perform an export of a footprint library to SVG.

    Note that much of the functionality of the "fp export svg" command is tested by the
    PCB "pcb export svg" tests, since they share the same underlying code.
    """

    input_file = kitest.get_data_file_path(input_lib)
    output_path = kitest.get_output_path(f"cli/{output_dir}/svg/")

    output_path.mkdir(parents=True, exist_ok=True)

    export_cmd = [
        utils.kicad_cli(),
        "sym",
        "export",
        "svg",
        str(input_file),
        "--output",
        str(output_path),
    ]

    stdout, stderr, exitcode = utils.run_and_capture(export_cmd)

    assert exitcode == 0
    assert stderr == ""
    assert stdout is not None

    # Check the number of footprints in the exported directory
    fp_files = list(Path(output_path).glob("*.svg"))
    assert len(fp_files) == num_units
