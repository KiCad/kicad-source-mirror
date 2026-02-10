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

import shutil
from pathlib import Path
import pytest

from conftest import KiTestFixture
import utils


def get_fp_upgrade_cmd(input: str, output: str) -> list[str]:
    return [
        utils.kicad_cli(),
        "fp",
        "upgrade",
        "--force",
        str(input),
        "--output",
        str(output),
    ]


@pytest.mark.parametrize(
    "input_lib,output_dir,num_fps",
    [
        # This is a v9-formatted library with 2 footprints
        (
            "cli/fp_lib_test/Audio_Module.v9.pretty",
            "fp_lib_test",
            2,
        ),
    ],
)
def test_fp_upgrade(
    kitest: KiTestFixture,
    input_lib: str,
    output_dir: str,
    num_fps: int,
) -> None:
    """
    Perform an upgrade of a footprint library.
    """

    input_file = kitest.get_data_file_path(input_lib)
    output_path = kitest.get_output_path(f"cli/{output_dir}/upgrade/")

    shutil.rmtree(output_path, ignore_errors=True)
    output_path.mkdir(parents=True, exist_ok=True)


    upgraded_lib_path = Path(output_path) / (
        input_file.stem + ".upgraded.pretty"
    )

    upgrade_cmd = get_fp_upgrade_cmd(input_file, upgraded_lib_path)

    stdout, stderr, exitcode = utils.run_and_capture(upgrade_cmd)

    assert exitcode == 0
    assert stderr == ""
    assert stdout is not None

    assert upgraded_lib_path.is_dir()

    # Check the number of footprints in the upgraded library
    fp_files = list(upgraded_lib_path.glob("*.kicad_mod"))
    assert len(fp_files) == num_fps


@pytest.mark.parametrize(
    "input_lib,output_dir,num_fps",
    [
        # This is a v9-formatted library with 2 footprints
        (
            "cli/fp_lib_test/Audio_Module.v9.pretty",
            "fp_lib_test",
            2,
        ),
    ],
)
def test_fp_export_svg(
    kitest: KiTestFixture,
    input_lib: str,
    output_dir: str,
    num_fps: int,
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
        "fp",
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
    assert len(fp_files) == num_fps
