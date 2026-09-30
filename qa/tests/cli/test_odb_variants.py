import zipfile
import re

import pytest

from conftest import KiTestFixture
import utils


@pytest.mark.parametrize("output_name", ["out-${VARIANT}.zip", "out.zip"])
def test_pcb_export_odb_multiple_variants(kitest: KiTestFixture, output_name: str):
    board = kitest.get_data_file_path("pcbnew/issue24735/issue24735.kicad_pcb")
    output_dir = kitest.get_output_path("cli/odb_variants/" + output_name.replace("${VARIANT}", "token"))
    target = output_dir / output_name
    command = [
        utils.kicad_cli(), "pcb", "export", "odb",
        "--variant", "var1", "--variant", "var2", "-o", str(target), str(board)
    ]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    assert exitcode == 0, stdout + stderr

    for name in ("var1", "var2"):
        archive = output_dir / f"out-{name}.zip"
        assert archive.exists()

        with zipfile.ZipFile(archive) as package:
            assert any(path.endswith("/matrix/matrix") for path in package.namelist())


def test_pcb_export_odb_mapped_part_name(kitest: KiTestFixture):
    board = kitest.get_data_file_path("pcbnew/issue17559.kicad_pcb")
    output_dir = kitest.get_output_path("cli/odb_mapped_part_name")
    archive = output_dir / "mapped.zip"
    command = [
        utils.kicad_cli(), "pcb", "export", "odb",
        "--bom-col-mfg-pn", "MPN", "-o", str(archive), str(board)
    ]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    assert exitcode == 0, stdout + stderr

    with zipfile.ZipFile(archive) as package:
        components = next(name for name in package.namelist()
                          if name.replace("\\", "/").endswith("/layers/comp_+_bot/components"))
        lines = package.read(components).decode("utf-8").splitlines()
        assert any(line.startswith("CMP ") and " IC5 AD8418AWBRZ" in line for line in lines)


def _export_options_archive(kitest: KiTestFixture, name: str, board_name: str, *options: str):
    board = kitest.get_data_file_path("pcbnew/" + board_name)
    archive = kitest.get_output_path("cli/odb_options/" + name) / "output.zip"
    command = [utils.kicad_cli(), "pcb", "export", "odb", *options,
               "-o", str(archive), str(board)]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    assert exitcode == 0, stdout + stderr
    return archive


def test_pcb_export_odb_origin_option(kitest: KiTestFixture):
    board = "issue5830.kicad_pcb"
    absolute = _export_options_archive(kitest, "origin_absolute", board)
    shifted = _export_options_archive(kitest, "origin_aux", board, "--origin", "aux")

    def points(archive):
        with zipfile.ZipFile(archive) as package:
            path = next(name for name in package.namelist()
                        if name.replace("\\", "/").endswith("/layers/f.cu/features"))
            return [tuple(map(float, line.split()[1:3]))
                    for line in package.read(path).decode("utf-8").splitlines()
                    if line.startswith("P ")]

    base_points, shifted_points = points(absolute), points(shifted)
    assert base_points and len(base_points) == len(shifted_points)
    assert all(abs(x - 86.0 - sx) < 0.000002 and abs(y + 13.0 - sy) < 0.000002
               for (x, y), (sx, sy) in zip(base_points, shifted_points))


def test_pcb_export_odb_product_name_option(kitest: KiTestFixture):
    archive = _export_options_archive(kitest, "product", "issue14130.kicad_pcb",
                                      "--product-name", "Customer Fab Rev A")
    with zipfile.ZipFile(archive) as package:
        names = {name.replace("\\", "/").split("/")[0] for name in package.namelist()}
        assert names == {"customer_fab_rev_a"}
        info = next(name for name in package.namelist() if name.endswith("/misc/info"))
        assert b"PRODUCT_MODEL_NAME=customer_fab_rev_a" in package.read(info)


def test_pcb_export_odb_data_set_option(kitest: KiTestFixture):
    archive = _export_options_archive(kitest, "assembly", "issue5830.kicad_pcb",
                                      "--data-set", "assembly")
    with zipfile.ZipFile(archive) as package:
        matrix = next(name for name in package.namelist() if name.endswith("/matrix/matrix"))
        data = package.read(matrix).decode("utf-8")
        assert "NAME=in1.cu" not in data and "NAME=in2.cu" not in data
        assert "NAME=f.cu" in data and "NAME=b.cu" in data


def test_pcb_export_odb_net_names_option(kitest: KiTestFixture):
    archive = _export_options_archive(kitest, "anonymous", "odbpp/net_name_collision.kicad_pcb",
                                      "--net-names", "anonymize")
    with zipfile.ZipFile(archive) as package:
        eda = next(name for name in package.namelist() if name.endswith("/eda/data"))
        records = [line for line in package.read(eda).decode("utf-8").splitlines()
                   if line.startswith("NET ") and not line.startswith("NET $NONE$")]
        assert records and all(re.match(r"NET N\d+(?:\s|$)", line) for line in records)
