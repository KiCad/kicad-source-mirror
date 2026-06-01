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
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#

import pytest
import utils
import re
import shutil
from pathlib import Path

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
