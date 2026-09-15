#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
# Copyright (C) 2023 KiCad Developers
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
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

import json
import re
import shutil
from pathlib import Path

import pytest
import utils

def test_version():
    command = [utils.kicad_cli(), "version"]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    assert exitcode == 0
    assert re.match("\\d+.\\d+.\\d+", stdout)
    assert stderr == ''

def test_version_plain():
    command = [utils.kicad_cli(), "version", "--format=plain"]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    assert exitcode == 0
    assert re.match("\\d+.\\d+.\\d+", stdout)
    assert stderr == ''

def test_version_commit():
    command = [utils.kicad_cli(), "version", "--format=commit"]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    assert exitcode == 0
    assert re.match("\\b[0-9a-f]{40}\\b", stdout)
    assert stderr == ''

def test_help():
    command = [utils.kicad_cli(), "help"]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    assert exitcode == 1
    assert stdout != ''
    assert stderr == ''


def test_jobset_run_relative_project_path(tmp_path):
    """Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24474

    Running a jobset with a relative project path used to free the project out from under the
    jobset runner when a kiface board loader reloaded it by its absolute path. The dangling
    project pointer then crashed while resolving ${PROJECTNAME} for the archive output path.
    """
    source_dir = Path(__file__).resolve().parent.parent.parent / "data" / "pcbnew" / "issue24474"
    work_dir = tmp_path / "issue24474"
    shutil.copytree(source_dir, work_dir)

    # Run only the archive destination, which is the one that resolves ${PROJECTNAME} in its
    # output path and triggered the crash. The project file is passed as a relative path.
    command = [
        utils.kicad_cli(),
        "jobset", "run",
        "--file", "common.kicad_jobset",
        "--output", "d289a4c3-15ec-4e02-92f4-a3700d8878f8",
        "issue24474.kicad_pro",
    ]

    stdout, stderr, exitcode = utils.run_and_capture(command, cwd=work_dir)

    # A crash would manifest as a non-zero/negative exit code; the archive must also be produced.
    assert exitcode == 0
    assert (work_dir / "output" / "issue24474-gerber.zip").exists()


def test_jobset_drc_uses_selected_variant(tmp_path):
    source_dir = Path(__file__).resolve().parent.parent.parent / "data" / "pcbnew" / "issue24900"
    work_dir = tmp_path / "issue24900"
    shutil.copytree(source_dir, work_dir)

    command = [
        utils.kicad_cli(),
        "jobset", "run",
        "--file", "TestProject.kicad_jobset",
        "--output", "f4454cb0-b93f-4f0c-bcdd-e3cd0a2dfc4a",
        "TestProject.kicad_pro",
    ]

    stdout, stderr, exitcode = utils.run_and_capture(command, cwd=work_dir)

    assert exitcode == 0, stdout + stderr

    report_path = work_dir / "output" / "TestProject-Variant1-drc.json"

    with report_path.open(encoding="utf-8") as report_file:
        report = json.load(report_file)

    parity_violations = report["schematic_parity"]
    assert len(parity_violations) == 2

    dnp_violation = next(v for v in parity_violations if "Do not populate" in v["description"])
    assert any("R1" in item["description"] for item in dnp_violation["items"])

    bom_violation = next(v for v in parity_violations if "bill of materials" in v["description"])
    assert any("R2" in item["description"] for item in bom_violation["items"])


def test_jobset_run_unknown_job_type(tmp_path):
    """A jobset saved by a newer KiCad can contain job types this version does not know.

    These used to deserialize to a null job and segfault as soon as the job list was
    printed. Now they load as placeholders and fail the run with a clear message.
    """
    source_dir = Path(__file__).resolve().parent.parent.parent / "data" / "cli" / "jobset_unknown_type"
    work_dir = tmp_path / "jobset_unknown_type"
    shutil.copytree(source_dir, work_dir)

    command = [
        utils.kicad_cli(),
        "jobset", "run",
        "--file", "jobset_unknown_type.kicad_jobset",
        "jobset_unknown_type.kicad_pro",
    ]

    stdout, stderr, exitcode = utils.run_and_capture(command, cwd=work_dir)

    # The run must fail gracefully: a positive exit code, not a signal (segfault gives a
    # negative returncode), and the unknown job reported by name.
    assert exitcode > 0
    assert "Unsupported job type 'special_from_the_future'" in stdout
