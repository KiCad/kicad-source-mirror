#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

import re

from conftest import KiTestFixture
import utils


def _export(kitest: KiTestFixture, fmt: str, name: str, *options: str):
    board = kitest.get_data_file_path("pcbnew/issue5830.kicad_pcb")
    target = kitest.get_output_path("cli/fab_export_content/") / name
    # The output directory outlives a run, and a rejected export must leave no file
    target.unlink(missing_ok=True)
    command = [utils.kicad_cli(), "pcb", "export", fmt, *options, "-o", str(target), str(board)]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    return target, stdout + stderr, exitcode


def _ipc_text(path):
    # The export time is the only content that differs between two runs
    return re.sub(r'(origination|lastChange|datetime)="[^"]*"', "", path.read_text(encoding="utf-8"))


def test_pcb_export_ipc2581_mode_is_data_set(kitest: KiTestFixture):
    data_set, output, exitcode = _export(kitest, "ipc2581", "data_set.xml", "--data-set", "fabrication")
    assert exitcode == 0, output
    mode, output, exitcode = _export(kitest, "ipc2581", "mode.xml", "--mode", "FABRICATION")
    assert exitcode == 0, output

    assert 'mode="FABRICATION"' in data_set.read_text(encoding="utf-8")
    assert _ipc_text(mode) == _ipc_text(data_set)


def test_pcb_export_rejects_unknown_data_set(kitest: KiTestFixture):
    target, output, exitcode = _export(kitest, "ipc2581", "bogus.xml", "--data-set", "bogus")
    assert exitcode != 0
    assert "Unsupported IPC-2581 data set" in output
    assert not target.exists()

    # ODB++ has no BOM-only product
    target, output, exitcode = _export(kitest, "odb", "bom.zip", "--data-set", "bom")
    assert exitcode != 0
    assert "Unsupported ODB++ data set" in output
    assert not target.exists()


def test_pcb_export_odb_rejects_unknown_section_key(kitest: KiTestFixture):
    target, output, exitcode = _export(kitest, "odb", "sections.zip", "--sections", "K?")
    assert exitcode != 0
    assert "Unknown ODB++ section key" in output
    # The key is checked before the plugin starts to write
    assert "Error generating ODB++ files" not in output
    assert not target.exists()
