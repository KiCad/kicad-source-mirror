"""Older-version export policy tests, using only the standard library.

Discovered by pytest, or run with:
KICAD_CLI=/path/to/kicad-cli python3 -m unittest discover -s qa/tests/cli -p test_downgrade.py
"""

# This program source code file is part of KiCad, a free EDA CAD application.
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET


GOLDEN = Path(__file__).resolve().parents[2] / "data/downgrade/golden"
REGRESSIONS = GOLDEN.parent / "regressions"
U1_UUID = "a4c99c12-ed9f-4373-b780-7fdce8d40657"
HATCH_FOOTPRINT = """(footprint "Hatch" (version 20260623) (generator "pcbnew")
  (layer "F.Cu")
  (fp_rect (start 0 0) (end 10 5) (stroke (width 0.2) (type default))
    (fill hatch) (layer "F.SilkS")))
"""
HATCH_SYMBOL = """(kicad_symbol_lib (version 20260629) (generator "eeschema")
  (symbol "Hatch"
    (property "Reference" "U" (at 0 2 0) (effects (font (size 1 1))))
    (property "Value" "Hatch" (at 0 -2 0) (effects (font (size 1 1))))
    (symbol "Hatch_0_1"
      (rectangle (start 0 0) (end 10 5)
        (stroke (width 0.2) (type default)) (fill (type hatch))))))
"""


class TestDropApproximations(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix="kicad_downgrade_policy_")
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.source = self.root / "source"
        self.source.mkdir()
        shutil.copyfile(GOLDEN / "current/body_styles.kicad_pro", self.source / "body_styles.kicad_pro")
        shutil.copyfile(GOLDEN / "current/body_styles.kicad_sch", self.source / "body_styles.kicad_sch")
        shutil.copyfile(GOLDEN / "current/graphics.kicad_pcb", self.source / "body_styles.kicad_pcb")
        self.env = os.environ.copy()
        self.env["KICAD_CONFIG_HOME"] = str(self.root / "config")
        self.env["LC_ALL"] = "C"

    def cli(self, *args):
        return subprocess.run(
            [os.environ.get("KICAD_CLI", "kicad-cli"), *map(str, args)],
            env=self.env, capture_output=True, text=True, timeout=60,
        )

    def require_success(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def source_snapshot(self):
        return {str(p.relative_to(self.source)): p.read_bytes()
                for p in self.source.rglob("*") if p.is_file()
                and p.suffix in {".kicad_pro", ".kicad_pcb", ".kicad_sch", ".kicad_mod", ".kicad_sym"}}

    def test_project_preview_report_and_export_use_same_policy(self):
        original = self.source_snapshot()
        for target in ("9.0", "10.0"):
            for drop in (False, True):
                with self.subTest(target=target, drop=drop):
                    option = ["--drop-approximations"] if drop else []
                    project = self.source / "body_styles.kicad_pro"
                    report_path = self.root / "report.json"
                    self.require_success(self.cli("project", "downgrade", project, "--target", target,
                                                  "--dry-run", "--report-json", report_path, *option))
                    report = json.loads(report_path.read_text())
                    self.assertEqual(report["drop_approximations"], drop)
                    lower = [e for e in report["entries"] if e["category"] == "lower"]
                    self.assertEqual(len(lower), 3 if target == "9.0" and not drop else 0)
                    self.assertFalse(report["blocked"])
                    output = self.root / f"project-{target}-{drop}"
                    self.assertFalse(output.exists())
                    if target == "9.0":
                        result = self.cli("project", "downgrade", project, "--target", target,
                                          "--output", output, *option)
                        self.assertNotEqual(result.returncode, 0)
                        self.assertFalse(output.exists())
                    self.require_success(self.cli("project", "downgrade", project, "--target", target,
                                                  "--force", "--output", output,
                                                  "--report-json", report_path, *option))
                    self.assertEqual(json.loads(report_path.read_text()), report)
                    pcb = (output / "body_styles.kicad_pcb").read_text()
                    sch = (output / "body_styles.kicad_sch").read_text()
                    self.assertEqual("gr_text_box" in pcb, target == "10.0" or not drop)
                    self.assertEqual("(fill hatch)" in pcb, target == "10.0")
                    self.assertEqual(U1_UUID in sch, target == "10.0" or not drop)
                    self.assertEqual("__KiCad9_body_" in sch, target == "9.0" and not drop)
                    self.assertEqual(self.source_snapshot(), original)

    def test_pcb_and_sch_commands_apply_option_and_preserve_source(self):
        original = self.source_snapshot()
        for kind, extension in (("pcb", ".kicad_pcb"), ("sch", ".kicad_sch")):
            for drop in (False, True):
                with self.subTest(kind=kind, drop=drop):
                    source = self.source / ("body_styles" + extension)
                    output = self.root / f"{kind}-{drop}"
                    if kind == "pcb":
                        output = output.with_suffix(extension)
                    option = ["--drop-approximations"] if drop else []
                    self.require_success(self.cli(kind, "downgrade", source, "--target", "9.0",
                                                  "--dry-run", *option))
                    self.assertFalse(output.exists())
                    result = self.cli(kind, "downgrade", source, "--target", "9.0",
                                      "--output", output, *option)
                    self.assertNotEqual(result.returncode, 0)
                    self.require_success(self.cli(kind, "downgrade", source, "--target", "9.0",
                                                  "--force", "--output", output, *option))
                    exported = output if kind == "pcb" else output / source.name
                    text = exported.read_text()
                    self.assertEqual("gr_text_box" in text if kind == "pcb" else U1_UUID in text, not drop)
                    self.assertEqual(self.source_snapshot(), original)

    def test_project_local_libraries_follow_policy(self):
        library = self.source / "Local.pretty"
        library.mkdir()
        (library / "Hatch.kicad_mod").write_text(HATCH_FOOTPRINT)
        (self.source / "Local.kicad_sym").write_text(HATCH_SYMBOL)
        original = self.source_snapshot()
        for drop in (False, True):
            with self.subTest(drop=drop):
                output = self.root / f"libraries-{drop}"
                option = ["--drop-approximations"] if drop else []
                self.require_success(self.cli("project", "downgrade", self.source / "body_styles.kicad_pro",
                                              "--target", "9.0", "--force", "--output", output, *option))
                footprint = (output / "Local.pretty/Hatch.kicad_mod").read_text()
                symbol = (output / "Local.kicad_sym").read_text()
                self.assertIn("fp_rect", footprint)
                self.assertIn("rectangle", symbol)
                self.assertIn("(fill no)" if drop else "(fill yes)", footprint)
                self.assertIn("(type none)" if drop else "(type outline)", symbol)
                self.assertEqual(self.source_snapshot(), original)

    def test_drop_option_and_force_cannot_bypass_blockers(self):
        library = self.source / "Local.kicad_sym"
        library.write_text(HATCH_SYMBOL.replace('(symbol "Hatch"', '(symbol "Hatch" (power local)', 1))
        original = self.source_snapshot()
        output = self.root / "blocked"
        report = self.root / "blocked.json"
        result = self.cli("project", "downgrade", self.source / "body_styles.kicad_pro", "--target", "9.0",
                          "--force", "--drop-approximations", "--report-json", report, "--output", output)
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(json.loads(report.read_text())["blocked"])
        self.assertFalse(output.exists())
        self.assertEqual(self.source_snapshot(), original)

    def test_json_report_cannot_replace_source_designs(self):
        original = self.source_snapshot()
        for extension in (".kicad_pro", ".kicad_sch", ".kicad_pcb"):
            for preview in (False, True):
                with self.subTest(extension=extension, preview=preview):
                    path = self.source / ("body_styles" + extension)
                    output = self.root / "report-collision-output"
                    mode = ["--dry-run"] if preview else ["--output", output]
                    result = self.cli("project", "downgrade", self.source / "body_styles.kicad_pro",
                                      "--target", "10.0", "--report-json", path, *mode)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(self.source_snapshot(), original)
                    self.assertFalse(output.exists())

    def test_json_report_cannot_replace_source_through_file_aliases(self):
        original = self.source_snapshot()
        source = self.source / "body_styles.kicad_pro"
        for kind in ("symlink", "hardlink"):
            with self.subTest(kind=kind):
                alias = self.root / (kind + "-report.json")
                try:
                    if kind == "symlink":
                        alias.symlink_to(source)
                    else:
                        os.link(source, alias)
                except OSError:
                    continue
                result = self.cli("project", "downgrade", source, "--target", "10.0",
                                  "--dry-run", "--report-json", alias)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(self.source_snapshot(), original)
                self.assertEqual(alias.read_bytes(), original["body_styles.kicad_pro"])

    def test_schematic_preview_and_invalid_target_create_no_output_directory(self):
        source = self.source / "body_styles.kicad_sch"
        original = self.source_snapshot()
        for target in ("10.0", "INVALID"):
            with self.subTest(target=target):
                output = self.root / ("preview-" + target)
                result = self.cli("sch", "downgrade", source, "--target", target,
                                  "--dry-run", "--output", output)
                if target == "10.0":
                    self.require_success(result)
                else:
                    self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output.exists())
                self.assertEqual(self.source_snapshot(), original)

    def test_barcode_export_resolves_updated_project_variables_before_cached_board_properties(self):
        fixture = REGRESSIONS / "project_barcode"
        copies = []
        for kind in ("variable", "literal"):
            copy = self.root / kind
            shutil.copytree(fixture, copy)
            project = copy / "project_barcode.kicad_pro"
            data = json.loads(project.read_text())
            data["text_variables"]["SERIAL"] = "UPDATED456"
            project.write_text(json.dumps(data), encoding="utf-8")
            if kind == "literal":
                board = copy / "project_barcode.kicad_pcb"
                text = board.read_text()
                self.assertIn('(text "${SERIAL}")', text)
                board.write_text(text.replace('(text "${SERIAL}")', '(text "UPDATED456")'), encoding="utf-8")
            copies.append(copy)

        svgs = []
        polygons = []
        for copy in copies:
            source = copy / "project_barcode.kicad_pcb"
            svg = copy / "native.svg"
            self.require_success(self.cli("pcb", "export", "svg", source, "--layers", "F.SilkS",
                                          "--exclude-drawing-sheet", "--output", svg))
            image = ET.fromstring(svg.read_bytes())
            title = image.find("{http://www.w3.org/2000/svg}title")
            self.assertIsNotNone(title)
            # Native SVG titles include the plotting timestamp, not drawing geometry.
            image.remove(title)
            svgs.append(ET.tostring(image))
            output = copy / "exported.kicad_pcb"
            self.require_success(self.cli("pcb", "downgrade", source, "--target", "9.0",
                                          "--force", "--output", output))
            text = output.read_text()
            nodes = re.findall(r"\(gr_poly\s+\(pts(.*?)\)\s+\(stroke", text, re.S)
            polygons.append(sorted(tuple(re.findall(r"\(xy\s+([^\s()]+)\s+([^\s()]+)\)", node))
                                   for node in nodes))
            self.assertGreater(len(polygons[-1]), 1)
            self.assertNotIn("\n\t(barcode", text)
        self.assertEqual(svgs[0], svgs[1])
        self.assertEqual(polygons[0], polygons[1])

    def test_nondefault_tuning_settings_require_consent_and_appear_in_preview_report(self):
        source = self.root / "tuning"
        shutil.copytree(REGRESSIONS / "tuning_model_settings", source)
        project = source / "tuning_model_settings.kicad_pro"
        original = {p.name: p.read_bytes() for p in source.iterdir() if p.is_file()}
        output = self.root / "tuning-export"
        report_path = self.root / "tuning-report.json"
        self.require_success(self.cli("project", "downgrade", project, "--target", "10.0",
                                      "--dry-run", "--report-json", report_path, "--output", output))
        preview = json.loads(report_path.read_text())
        drops = [entry for entry in preview["entries"] if entry["category"] == "drop"]
        self.assertEqual(sum(entry["count"] for entry in drops), 2)
        self.assertTrue(preview["lossy"])
        self.assertFalse(output.exists())
        result = self.cli("project", "downgrade", project, "--target", "10.0", "--output", output)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(output.exists())
        self.require_success(self.cli("project", "downgrade", project, "--target", "10.0", "--force",
                                      "--report-json", report_path, "--output", output))
        self.assertEqual(json.loads(report_path.read_text()), preview)
        converted = json.loads((output / project.name).read_text())["tuning_profiles"]
        self.assertEqual(converted["meta"]["version"], 0)
        profiles = converted["tuning_profiles_impedance_geometric"]
        self.assertEqual(len(profiles), 2)
        for profile in profiles:
            self.assertNotIn("frequency", profile)
            self.assertNotIn("model_solder_mask", profile)
            self.assertEqual(profile["target_impedance"], 50)
        self.assertEqual({p.name: p.read_bytes() for p in source.iterdir() if p.is_file()}, original)

    def test_current_project_schemas_and_drc_exclusions_are_downgraded(self):
        project = self.source / "body_styles.kicad_pro"
        document = json.loads(project.read_text())
        document["meta"]["version"] = 4
        settings = document.setdefault("board", {}).setdefault("design_settings", {})
        settings.setdefault("meta", {})
        settings["meta"]["version"] = 3
        settings["drc_exclusions"] = [{
            "marker": {
                "error_type": "DRCET_CLEARANCE",
                "position": {"x_nm": "123", "y_nm": "-456"},
                "items": [{"value": U1_UUID}],
            },
            "comment": "Accepted clearance exception",
        }]
        document["erc"] = {
            "meta": {"version": 1},
            "erc_exclusions": [{
                "marker": {
                    "error_type": "ERCET_PIN_NOT_CONNECTED",
                    "position": {"x_nm": "12300", "y_nm": "-45600"},
                    "items": [{"value": U1_UUID}],
                },
                "comment": "Accepted ERC exception",
            }],
        }
        project.write_text(json.dumps(document), encoding="utf-8")
        original = self.source_snapshot()

        for target in ("9.0", "10.0"):
            with self.subTest(target=target):
                output = self.root / ("schema-" + target)
                self.require_success(self.cli("project", "downgrade", project, "--target", target,
                                              "--force", "--output", output))
                converted = json.loads((output / project.name).read_text())
                self.assertEqual(converted["meta"]["version"], 3)
                board_settings = converted["board"]["design_settings"]
                self.assertEqual(board_settings["meta"]["version"], 2)
                self.assertEqual(board_settings["drc_exclusions"], [[
                    "clearance|123|-456|" + U1_UUID + "|00000000-0000-0000-0000-000000000000",
                    "Accepted clearance exception",
                ]])
                self.assertEqual(converted["erc"]["meta"]["version"], 0)
                self.assertEqual(converted["erc"]["erc_exclusions"], [[
                    "pin_not_connected|123|-456|" + U1_UUID + "|00000000-0000-0000-0000-000000000000|||",
                    "Accepted ERC exception",
                ]])
                self.assertEqual(self.source_snapshot(), original)

    def test_post_release_project_settings_are_reported_and_removed(self):
        source = self.root / "chain-settings"
        shutil.copytree(REGRESSIONS / "tuning_model_settings", source)
        project = source / "tuning_model_settings.kicad_pro"
        document = json.loads(project.read_text())
        document["meta"]["version"] = 4
        document["net_settings"]["net_chain_netclasses"] = {"clock": "Default"}
        document["net_settings"]["net_chain_classes"] = {"clock": "Fast"}
        settings = document["board"]["design_settings"]
        settings["meta"]["version"] = 3
        settings["via_stack_presets"] = [{"name": "Custom microvia stack"}]
        profiles = document["tuning_profiles"]
        profiles["meta"]["version"] = 2
        profiles["tuning_profiles_impedance_geometric"][0]["net_chain_bridge_prop_delay"] = 25
        project.write_text(json.dumps(document), encoding="utf-8")
        original = {p.name: p.read_bytes() for p in source.iterdir() if p.is_file()}
        output = self.root / "chain-settings-export"
        report_path = self.root / "chain-settings-report.json"
        self.require_success(self.cli("project", "downgrade", project, "--target", "10.0",
                                      "--dry-run", "--report-json", report_path))
        preview = json.loads(report_path.read_text())
        drops = [entry for entry in preview["entries"] if entry["category"] == "drop"]
        self.assertEqual(sum(entry["count"] for entry in drops), 6)
        result = self.cli("project", "downgrade", project, "--target", "10.0", "--output", output)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(output.exists())
        self.require_success(self.cli("project", "downgrade", project, "--target", "10.0",
                                      "--force", "--report-json", report_path, "--output", output))
        self.assertEqual(json.loads(report_path.read_text()), preview)
        converted = json.loads((output / project.name).read_text())
        self.assertEqual(converted["meta"]["version"], 3)
        self.assertEqual(converted["board"]["design_settings"]["meta"]["version"], 2)
        self.assertNotIn("net_chain_netclasses", converted["net_settings"])
        self.assertNotIn("net_chain_classes", converted["net_settings"])
        self.assertNotIn("via_stack_presets", converted["board"]["design_settings"])
        self.assertEqual(converted["tuning_profiles"]["meta"]["version"], 0)
        for profile in converted["tuning_profiles"]["tuning_profiles_impedance_geometric"]:
            self.assertNotIn("net_chain_bridge_prop_delay", profile)
        self.assertEqual({p.name: p.read_bytes() for p in source.iterdir() if p.is_file()}, original)

    def test_post_release_drc_rules_are_reported_in_conditions_and_assertions(self):
        project = self.source / "body_styles.kicad_pro"
        rules = self.source / "body_styles.kicad_dru"
        rules.write_text("""(version 1)
(rule "Keep literal" (condition "A.NetName == 'customProperty()'")
    (constraint clearance (min 0.2mm)))
(rule "Keep coordinates" (condition "A.Start_X > 0 && A.Start_Y > 0")
    (constraint clearance (min 0.2mm)))
(rule "Microvia aspect" (constraint microvia_aspect_ratio (max 1)))
(rule "Microvia depth" (constraint microvia_stack_depth (max 2)))
(rule "Keepout" (condition "A.intersectsKeepout('area')")
    (constraint clearance (min 0.2mm)))
(rule "Stacked" (constraint assertion "A.isStackedVia()"))
(rule "Property value" (constraint assertion "A.customProperty('key') == 'value'"))
(rule "Property presence" (constraint assertion "A.hasCustomProperty('key')"))
(rule "Area outlines" (condition "A.intersectsArea('area')")
    (constraint clearance (min 0.2mm)))
""", encoding="utf-8")
        original = rules.read_bytes()

        for target in ("9.0", "10.0"):
            with self.subTest(target=target):
                report_path = self.root / ("rules-" + target + ".json")
                output = self.root / ("rules-" + target)
                self.require_success(self.cli("project", "downgrade", project, "--target", target,
                                              "--dry-run", "--report-json", report_path))
                preview = json.loads(report_path.read_text())
                rule_drops = [entry for entry in preview["entries"]
                              if entry["category"] == "drop" and entry["feature"].startswith("Custom DRC rule '")]
                self.assertEqual(sum(entry["count"] for entry in rule_drops), 7)
                self.require_success(self.cli("project", "downgrade", project, "--target", target,
                                              "--force", "--report-json", report_path, "--output", output))
                self.assertEqual(json.loads(report_path.read_text()), preview)
                converted = (output / rules.name).read_text()
                self.assertIn('"Keep literal"', converted)
                self.assertIn('"Keep coordinates"', converted)
                self.assertEqual(converted.count("(rule "), 2)
                self.assertEqual(rules.read_bytes(), original)

    def test_post_target_drc_properties_require_consent_in_both_expressions(self):
        project = self.source / "body_styles.kicad_pro"
        rules = self.source / "body_styles.kicad_dru"
        new_properties = ("A.Start_Shape", "B.End_Width", "AB.Scale_X", "A.Major_Radius",
                          "A.Exclude_From_Simulation", "A.Automatically_Update_Net",
                          "A.Grid_Type", "A.start_shape")
        ten_properties = ("A.Corner_Radius", "A.Auto_Thickness", "A.Target_Delay")
        blocks = ["(version 1)",
                  '(rule "Keep literal" (condition "\'A.Start_Shape\' == \'A.Start_Shape\'") '
                  '(constraint clearance (min 0.1mm)))',
                  '(rule "Keep coordinates" (condition "A.Start_X >= 0 && B.End_Y >= 0 '
                  '&& A.Radius >= 0") (constraint clearance (min 0.1mm)))']
        for index, expression in enumerate((*new_properties, *ten_properties)):
            for kind in ("condition", "assertion"):
                value = expression + " == " + expression
                clause = ('(condition "' + value + '") (constraint clearance (min 0.1mm))'
                          if kind == "condition" else '(constraint assertion "' + value + '")')
                blocks.append(f'(rule "Property {index} {kind}" {clause})')
        blocks.extend([
            '(rule "Uppercase condition" (condition "A.ISSTACKEDVIA()") '
            '(constraint clearance (min 0.1mm)))',
            '(rule "Uppercase assertion" (constraint assertion "A.ISSTACKEDVIA()"))',
        ])
        rules.write_text("\n".join(blocks) + "\n", encoding="utf-8")
        original = self.source_snapshot()
        original_rules = rules.read_bytes()
        for target in ("9.0", "10.0"):
            with self.subTest(target=target):
                output = self.root / ("property-rules-" + target)
                report_path = self.root / ("property-rules-" + target + ".json")
                self.require_success(self.cli("project", "downgrade", project, "--target", target,
                                              "--dry-run", "--report-json", report_path))
                preview = json.loads(report_path.read_text())
                dropped = [entry for entry in preview["entries"]
                           if entry["feature"].startswith("Custom DRC rule '") and entry["category"] == "drop"]
                self.assertEqual(sum(entry["count"] for entry in dropped), 24 if target == "9.0" else 18)
                self.assertFalse(output.exists())
                result = self.cli("project", "downgrade", project, "--target", target, "--output", output)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output.exists())
                self.require_success(self.cli("project", "downgrade", project, "--target", target,
                                              "--force", "--output", output, "--report-json", report_path))
                self.assertEqual(json.loads(report_path.read_text()), preview)
                converted = (output / rules.name).read_text()
                self.assertIn('rule "Keep literal"', converted)
                self.assertIn('rule "Keep coordinates"', converted)
                self.assertNotIn('rule "Property 0 condition"', converted)
                self.assertEqual('rule "Property 8 assertion"' in converted, target == "10.0")
                self.assertEqual(self.source_snapshot(), original)
                self.assertEqual(rules.read_bytes(), original_rules)

    def test_new_bom_and_manufacturing_output_choices_require_consent(self):
        project = self.source / "body_styles.kicad_pro"
        document = json.loads(project.read_text())
        document["schematic"]["bom_settings"] = {
            "name": "Custom", "fields_ordered": [], "sort_field": "Reference", "sort_asc": True,
            "filter_string": "10k", "filter_scope": "all", "group_symbols": False,
            "exclude_dnp": False, "include_excluded_from_bom": False,
        }
        document["schematic"]["bom_fmt_settings"] = {
            "name": "Custom", "field_delimiter": ",", "string_delimiter": '"',
            "ref_delimiter": ",", "ref_range_delimiter": "", "keep_tabs": False,
            "keep_line_breaks": False, "include_byte_order_mark": True,
        }
        document.setdefault("board", {})["ipc2581"] = {"mode": "ASSEMBLY"}
        document["board"]["idf_export"] = {"part_number_field": "MPN"}
        project.write_text(json.dumps(document), encoding="utf-8")
        original = self.source_snapshot()
        report_path = self.root / "output-choices.json"
        output = self.root / "output-choices"
        self.require_success(self.cli("project", "downgrade", project, "--target", "10.0",
                                      "--dry-run", "--report-json", report_path))
        preview = json.loads(report_path.read_text())
        drops = [entry for entry in preview["entries"] if entry["category"] == "drop"]
        self.assertEqual(sum(entry["count"] for entry in drops), 4)
        result = self.cli("project", "downgrade", project, "--target", "10.0", "--output", output)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(output.exists())
        self.require_success(self.cli("project", "downgrade", project, "--target", "10.0",
                                      "--force", "--report-json", report_path, "--output", output))
        self.assertEqual(json.loads(report_path.read_text()), preview)
        converted = json.loads((output / project.name).read_text())
        self.assertNotIn("filter_scope", converted["schematic"]["bom_settings"])
        self.assertEqual(converted["schematic"]["bom_settings"]["filter_string"], "")
        self.assertNotIn("include_byte_order_mark", converted["schematic"]["bom_fmt_settings"])
        self.assertNotIn("mode", converted["board"]["ipc2581"])
        self.assertNotIn("idf_export", converted["board"])
        self.assertEqual(self.source_snapshot(), original)

    def test_future_project_schemas_block_without_modifying_source(self):
        project = self.source / "body_styles.kicad_pro"
        document = json.loads(project.read_text())
        document["meta"]["version"] = 5
        project.write_text(json.dumps(document), encoding="utf-8")
        original = self.source_snapshot()
        output = self.root / "future-schema"
        report_path = self.root / "future-schema.json"

        for preview in (False, True):
            with self.subTest(preview=preview):
                option = ["--dry-run"] if preview else []
                result = self.cli("project", "downgrade", project, "--target", "10.0",
                                  "--force", "--report-json", report_path, "--output", output, *option)
                self.assertTrue(json.loads(report_path.read_text())["blocked"])
                if not preview:
                    self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output.exists())
                self.assertEqual(self.source_snapshot(), original)

    def test_library_table_strings_keep_frozen_reader_meaning(self):
        (self.source / "Local.kicad_sym").write_text(HATCH_SYMBOL, encoding="utf-8")
        table = self.source / "sym-lib-table"
        table.write_text('(sym_lib_table (version 7) (lib (name "Local") (type "KiCad") '
                         '(uri "${KIPRJMOD}/Local.kicad_sym") (options "") '
                         '(descr "Line\\npath\\\\suffix")))\n', encoding="utf-8")
        original = table.read_bytes()

        for target in ("9.0", "10.0"):
            with self.subTest(target=target):
                output = self.root / ("table-" + target)
                self.require_success(self.cli("project", "downgrade", self.source / "body_styles.kicad_pro",
                                              "--target", target, "--force", "--output", output))
                text = (output / table.name).read_text()
                expected = '(descr "Line\\npath\\\\suffix")' if target == "9.0" else '(descr "Line\npath\\suffix")'
                self.assertIn(expected, text)
                self.assertEqual(table.read_bytes(), original)

    def test_unrepresentable_library_table_quoting_blocks_before_writes(self):
        (self.source / "Local.kicad_sym").write_text(HATCH_SYMBOL, encoding="utf-8")
        table = self.source / "sym-lib-table"
        table.write_text('(sym_lib_table (version 7) (lib (name "Local") (type "KiCad") '
                         '(uri "${KIPRJMOD}/Local.kicad_sym") (options "") '
                         '(descr "Two \\"words\\"")))\n', encoding="utf-8")
        original = table.read_bytes()
        report_path = self.root / "table-block.json"

        for preview in (False, True):
            with self.subTest(preview=preview):
                output = self.root / ("table-block-" + str(preview))
                option = ["--dry-run"] if preview else []
                result = self.cli("project", "downgrade", self.source / "body_styles.kicad_pro",
                                  "--target", "10.0", "--force", "--drop-approximations",
                                  "--report-json", report_path, "--output", output, *option)
                report = json.loads(report_path.read_text())
                self.assertTrue(report["blocked"])
                if not preview:
                    self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output.exists())
                self.assertEqual(table.read_bytes(), original)

        self.require_success(self.cli("project", "downgrade", self.source / "body_styles.kicad_pro",
                                      "--target", "9.0", "--force", "--output", self.root / "table-quote-nine"))
        self.assertEqual(table.read_bytes(), original)

    def test_nested_library_tables_block_only_the_unsupported_target(self):
        table = self.source / "sym-lib-table"
        nested = self.source / "sub"
        nested.mkdir()
        (nested / "sym-lib-table").write_text('(sym_lib_table (version 7))\n', encoding="utf-8")
        table.write_text('(sym_lib_table (version 7) (lib (name "nested") (type "Table") '
                         '(uri "${KIPRJMOD}/sub/sym-lib-table") (options "") (descr "")))\n', encoding="utf-8")
        original = table.read_bytes()
        report_path = self.root / "nested-table.json"
        output = self.root / "nested-nine"
        result = self.cli("project", "downgrade", self.source / "body_styles.kicad_pro", "--target", "9.0",
                          "--force", "--report-json", report_path, "--output", output)
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(json.loads(report_path.read_text())["blocked"])
        self.assertFalse(output.exists())
        output = self.root / "nested-ten"
        self.require_success(self.cli("project", "downgrade", self.source / "body_styles.kicad_pro",
                                      "--target", "10.0", "--force", "--output", output))
        self.assertIn('(type "Table")', (output / table.name).read_text())
        self.assertTrue((output / "sub/sym-lib-table").is_file())
        self.assertEqual(table.read_bytes(), original)

    def test_omitted_library_table_types_are_not_replaced_with_empty_strings(self):
        table = self.source / "sym-lib-table"
        table.write_text('(sym_lib_table (version 7) (lib (name "Local") '
                         '(uri "${KIPRJMOD}/Local.kicad_sym") (options "") (descr "")))\n', encoding="utf-8")
        (self.source / "Local.kicad_sym").write_text(HATCH_SYMBOL, encoding="utf-8")
        original = table.read_bytes()

        for target in ("9.0", "10.0"):
            with self.subTest(target=target):
                output = self.root / ("omitted-type-" + target)
                self.require_success(self.cli("project", "downgrade", self.source / "body_styles.kicad_pro",
                                              "--target", target, "--force", "--output", output))
                self.assertNotIn("(type ", (output / table.name).read_text())
                self.assertEqual(table.read_bytes(), original)


    def test_failed_design_export_does_not_modify_source_libraries(self):
        library = self.source / "Local.pretty"
        library.mkdir()
        (library / "Hatch.kicad_mod").write_text(HATCH_FOOTPRINT)
        (self.source / "Local.kicad_sym").write_text(HATCH_SYMBOL)
        original = self.source_snapshot()

        for kind, extension in (("pcb", ".kicad_pcb"), ("sch", ".kicad_sch")):
            with self.subTest(kind=kind):
                output_directory = self.root / (kind + "-read-only-output")
                output_directory.mkdir()
                existing_output = output_directory / ("body_styles" + extension)
                existing_output.write_bytes(b"previous output")
                output = existing_output if kind == "pcb" else output_directory
                libraries = library if kind == "pcb" else self.source

                try:
                    output_directory.chmod(0o555)
                    if os.access(output_directory, os.W_OK):
                        self.skipTest("This account can write to read-only directories")
                    result = self.cli(kind, "downgrade", self.source / ("body_styles" + extension),
                                      "--target", "9.0", "--force", "--libraries", libraries,
                                      "--output", output)
                finally:
                    output_directory.chmod(0o755)

                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(self.source_snapshot(), original)
                self.assertEqual(existing_output.read_bytes(), b"previous output")
                self.assertEqual(list(output_directory.iterdir()), [existing_output])
                self.assertFalse(any(".downgrade_tmp" in p.name or ".downgrade_backup" in p.name
                                     for p in self.source.rglob("*")))


    def test_project_variant_is_validated_during_preview(self):
        original = self.source_snapshot()
        project = self.source / "body_styles.kicad_pro"
        for target in ("9.0", "10.0"):
            for preview in (False, True):
                with self.subTest(target=target, preview=preview):
                    output = self.root / f"invalid-variant-{target}-{preview}"
                    report = self.root / f"invalid-variant-{target}-{preview}.json"
                    option = ["--dry-run"] if preview else ["--force"]
                    result = self.cli("project", "downgrade", project, "--target", target,
                                      "--variant", "missing-variant", "--output", output,
                                      "--report-json", report, *option)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertFalse(output.exists())
                    self.assertFalse(report.exists())
                    self.assertEqual(self.source_snapshot(), original)


    def test_project_variant_target_validation_matches_export(self):
        project = self.source / "body_styles.kicad_pro"
        document = json.loads(project.read_text())
        document["schematic"]["variants"] = [{"name": "Populated"}]
        project.write_text(json.dumps(document))
        original = self.source_snapshot()
        for preview in (False, True):
            with self.subTest(preview=preview):
                output = self.root / f"supported-variant-target-{preview}"
                report = self.root / f"supported-variant-target-{preview}.json"
                option = ["--dry-run"] if preview else ["--force"]
                result = self.cli("project", "downgrade", project, "--target", "10.0",
                                  "--variant", "Populated", "--output", output,
                                  "--report-json", report, *option)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output.exists())
                self.assertFalse(report.exists())
                self.assertEqual(self.source_snapshot(), original)


    def test_schematic_only_project_can_preview_and_export_variant(self):
        project = self.source / "body_styles.kicad_pro"
        document = json.loads(project.read_text())
        document["schematic"]["variants"] = [{"name": "Populated"}]
        project.write_text(json.dumps(document))
        (self.source / "body_styles.kicad_pcb").unlink()
        original = self.source_snapshot()
        output = self.root / "schematic-only-variant"
        preview_report = self.root / "schematic-only-preview.json"
        export_report = self.root / "schematic-only-export.json"
        self.require_success(self.cli("project", "downgrade", project, "--target", "9.0",
                                      "--variant", "Populated", "--output", output,
                                      "--dry-run", "--report-json", preview_report))
        self.assertFalse(output.exists())
        self.require_success(self.cli("project", "downgrade", project, "--target", "9.0",
                                      "--variant", "Populated", "--output", output,
                                      "--force", "--report-json", export_report))
        self.assertEqual(json.loads(preview_report.read_text()), json.loads(export_report.read_text()))
        self.assertTrue((output / "body_styles.kicad_sch").is_file())
        self.assertFalse((output / "body_styles.kicad_pcb").exists())
        self.assertEqual(self.source_snapshot(), original)


    def test_schematic_public_export_requires_output_even_with_force(self):
        source = self.source / "body_styles.kicad_sch"
        original = self.source_snapshot()
        for target in ("9.0", "10.0"):
            for drop in (False, True):
                for force in (False, True):
                    with self.subTest(target=target, drop=drop, force=force):
                        option = ["--drop-approximations"] if drop else []
                        if force:
                            option += ["--force"]
                        result = self.cli("sch", "downgrade", source, "--target", target, *option)
                        self.assertNotEqual(result.returncode, 0)
                        self.assertEqual(self.source_snapshot(), original)
                self.require_success(self.cli("sch", "downgrade", source, "--target", target,
                                              "--dry-run", *(["--drop-approximations"] if drop else [])))
                self.assertEqual(self.source_snapshot(), original)


    def test_schematic_existing_destinations_and_source_aliases_are_refused(self):
        source = self.source / "body_styles.kicad_sch"
        original = self.source_snapshot()
        for target in ("9.0", "10.0"):
            for drop in (False, True):
                for kind in ("empty-directory", "populated-directory", "directory-symlink",
                             "file-symlink", "file-hardlink", "input-symlink", "input-hardlink"):
                    with self.subTest(target=target, drop=drop, kind=kind):
                        output = self.root / f"existing-{target}-{drop}-{kind}"
                        input_file = source
                        if kind == "empty-directory":
                            output.mkdir()
                        elif kind == "populated-directory":
                            output.mkdir()
                            (output / "keep.txt").write_bytes(b"previous artifact")
                        elif kind == "directory-symlink":
                            output.symlink_to(self.source, target_is_directory=True)
                        elif kind == "file-symlink":
                            output.symlink_to(source)
                        elif kind == "file-hardlink":
                            os.link(source, output)
                        else:
                            alias_directory = output
                            alias_directory.mkdir()
                            input_file = alias_directory / source.name
                            if kind == "input-symlink":
                                input_file.symlink_to(source)
                            else:
                                os.link(source, input_file)
                            output = self.source
                        option = ["--drop-approximations"] if drop else []
                        result = self.cli("sch", "downgrade", input_file, "--target", target,
                                          "--force", "--output", output, *option)
                        self.assertNotEqual(result.returncode, 0)
                        self.assertEqual(self.source_snapshot(), original)
                        if kind == "empty-directory":
                            self.assertEqual(list(output.iterdir()), [])
                        elif kind == "populated-directory":
                            self.assertEqual(list(output.iterdir()), [output / "keep.txt"])
                            self.assertEqual((output / "keep.txt").read_bytes(), b"previous artifact")
                        elif kind.endswith("symlink"):
                            self.assertTrue((input_file if kind.startswith("input") else output).is_symlink())
                        self.assertFalse(any(".downgrade_tmp" in p.name or ".downgrade_backup" in p.name
                                             for p in self.root.rglob("*")))


    def test_schematic_libraries_are_copied_and_converted_under_output(self):
        libraries = self.source / "LocalSymbols"
        (libraries / "nested").mkdir(parents=True)
        (libraries / "nested/Hatch.kicad_sym").write_text(HATCH_SYMBOL)
        (libraries / "~Hatch.kicad_sym.lck").write_bytes(b"working artifact")
        original = self.source_snapshot()
        for target in ("9.0", "10.0"):
            for drop in (False, True):
                with self.subTest(target=target, drop=drop):
                    output = self.root / f"symbol-copy-{target}-{drop}"
                    option = ["--drop-approximations"] if drop else []
                    self.require_success(self.cli("sch", "downgrade", self.source / "body_styles.kicad_sch",
                                                  "--target", target, "--force", "--output", output,
                                                  "--libraries", libraries, *option))
                    exported = output / "LocalSymbols/nested/Hatch.kicad_sym"
                    self.assertTrue(exported.is_file())
                    text = exported.read_text()
                    self.assertIn("(version 20241209)" if target == "9.0" else "(version 20251024)", text)
                    self.assertIn("(type none)" if target == "9.0" and drop else
                                  "(type outline)" if target == "9.0" else "(type hatch)", text)
                    self.assertFalse((output / "LocalSymbols/~Hatch.kicad_sym.lck").exists())
                    self.assertEqual(self.source_snapshot(), original)


    def test_schematic_library_blockers_leave_no_output_or_source_changes(self):
        libraries = self.source / "UnsupportedSymbols"
        libraries.mkdir()
        (libraries / "Hatch.kicad_sym").write_text(
            HATCH_SYMBOL.replace('(property "Value" "Hatch"', '(property "Value" "${PROPERTY.Missing}"', 1))
        original = self.source_snapshot()
        for target in ("9.0", "10.0"):
            for drop in (False, True):
                with self.subTest(target=target, drop=drop):
                    output = self.root / f"symbol-block-{target}-{drop}"
                    option = ["--drop-approximations"] if drop else []
                    result = self.cli("sch", "downgrade", self.source / "body_styles.kicad_sch",
                                      "--target", target, "--force", "--output", output,
                                      "--libraries", libraries, *option)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertFalse(output.exists())
                    self.assertEqual(self.source_snapshot(), original)


    def native_default_project(self):
        directory = self.source / "native-default"
        directory.mkdir()
        project = directory / "project_barcode.kicad_pro"
        shutil.copyfile(REGRESSIONS / "project_barcode/project_barcode.kicad_pro", project)
        data = GOLDEN.parent.parent
        shutil.copyfile(data / "fuzz/pcb/sexpr/default.kicad_pcb",
                        directory / "project_barcode.kicad_pcb")
        shutil.copyfile(data / "libraries/test_project/test_project.kicad_sch",
                        directory / "project_barcode.kicad_sch")
        document = json.loads(project.read_text())
        document["meta"]["version"] = 4
        document["board"]["design_settings"]["meta"]["version"] = 3
        document["erc"] = {"meta": {"version": 1}, "erc_exclusions": []}
        document["tuning_profiles"]["meta"]["version"] = 2
        project.write_text(json.dumps(document))
        return project

    def test_failed_export_keeps_a_destination_the_user_already_had(self):
        # A stray board the export never converts makes the final backstop refuse, after the
        # copy has already put files in the destination.
        shutil.copyfile(GOLDEN / "current/geometry.kicad_pcb", self.source / "stray.kicad_pcb")
        original = self.source_snapshot()
        project = self.source / "body_styles.kicad_pro"

        existing = self.root / "MyExports"
        existing.mkdir()
        result = self.cli("project", "downgrade", project, "--target", "9.0", "--force",
                          "--output", existing)
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue(existing.is_dir(), "removed a folder the export did not create")
        self.assertEqual(sorted(p.name for p in existing.iterdir()), [],
                         "left its own output behind in the user's folder")
        self.assertEqual(self.source_snapshot(), original)

        made = self.root / "made" / "by-export"
        result = self.cli("project", "downgrade", project, "--target", "9.0", "--force",
                          "--output", made)
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse(made.exists(), "kept a folder the export created itself")
        self.assertEqual(self.source_snapshot(), original)

    def test_native_default_project_exports_without_force(self):
        project = self.native_default_project()
        original = self.source_snapshot()
        for target in ("9.0", "10.0"):
            for drop in (False, True):
                with self.subTest(target=target, drop=drop):
                    option = ["--drop-approximations"] if drop else []
                    output = self.root / f"native-default-{target}-{drop}"
                    preview_path = self.root / f"native-default-preview-{target}-{drop}.json"
                    export_path = self.root / f"native-default-export-{target}-{drop}.json"
                    self.require_success(self.cli("project", "downgrade", project, "--target", target,
                                                  "--dry-run", "--report-json", preview_path, *option))
                    preview = json.loads(preview_path.read_text())
                    self.assertFalse(preview["blocked"])
                    self.assertFalse(preview["lossy"], preview)
                    self.assertFalse(any(e["category"] in ("drop", "lower") for e in preview["entries"]))
                    self.require_success(self.cli("project", "downgrade", project, "--target", target,
                                                  "--output", output, "--report-json", export_path, *option))
                    self.assertEqual(json.loads(export_path.read_text()), preview)
                    exported = json.loads((output / project.name).read_text())
                    self.assertEqual(exported["meta"]["version"], 3)
                    self.assertEqual(exported["board"]["design_settings"]["meta"]["version"], 2)
                    self.assertEqual(exported["erc"]["meta"]["version"], 0)
                    if target == "9.0":
                        self.assertNotIn("tuning_profiles", exported)
                        self.assertNotIn("component_class_settings", exported)
                    else:
                        self.assertEqual(exported["tuning_profiles"]["meta"]["version"], 0)
                        self.assertEqual(exported["component_class_settings"]["assignments"], [])
                    self.assertTrue((output / "project_barcode.kicad_pcb").is_file())
                    self.assertTrue((output / "project_barcode.kicad_sch").is_file())
                    self.assertEqual(self.source_snapshot(), original)

    def test_saved_project_choices_still_require_force_for_kicad9(self):
        project = self.native_default_project()
        native_default = json.loads(project.read_text())
        saved = json.loads((REGRESSIONS / "tuning_model_settings/tuning_model_settings.kicad_pro").read_text())
        profile = saved["tuning_profiles"]["tuning_profiles_impedance_geometric"][0]
        profile["profile_name"] = "Default"
        for choice in ("unreferenced-profile", "selected-profile", "missing-profile-selection",
                       "assignment", "sheet-classes"):
            document = json.loads(json.dumps(native_default))
            if choice in ("unreferenced-profile", "selected-profile"):
                document["tuning_profiles"]["tuning_profiles_impedance_geometric"] = [profile]
            if choice in ("selected-profile", "missing-profile-selection"):
                document["net_settings"]["classes"][0]["tuning_profile"] = "Default"
            elif choice == "assignment":
                document["component_class_settings"]["assignments"] = [
                    {"component_class": "Fast", "conditions_operator": "ALL", "conditions": []}]
            elif choice == "sheet-classes":
                document["component_class_settings"]["sheet_component_classes"]["enabled"] = True
            project.write_text(json.dumps(document))
            original = self.source_snapshot()
            for target in ("9.0", "10.0"):
                with self.subTest(choice=choice, target=target):
                    report_path = self.root / f"choice-{choice}-{target}.json"
                    output = self.root / f"choice-{choice}-{target}"
                    self.require_success(self.cli("project", "downgrade", project, "--target", target,
                                                  "--dry-run", "--report-json", report_path))
                    preview = json.loads(report_path.read_text())
                    self.assertFalse(preview["blocked"])
                    self.assertEqual(preview["lossy"], target == "9.0")
                    drops = [e for e in preview["entries"] if e["category"] == "drop"]
                    self.assertEqual(len(drops), 1 if target == "9.0" else 0, preview)
                    result = self.cli("project", "downgrade", project, "--target", target,
                                      "--output", output, "--report-json", report_path)
                    if target == "9.0":
                        self.assertNotEqual(result.returncode, 0)
                        self.assertFalse(output.exists())
                        self.require_success(self.cli("project", "downgrade", project, "--target", target,
                                                      "--force", "--output", output,
                                                      "--report-json", report_path))
                    else:
                        self.require_success(result)
                    self.assertEqual(json.loads(report_path.read_text()), preview)
                    self.assertEqual(self.source_snapshot(), original)

    def test_future_component_class_schema_blocks_even_with_force(self):
        project = self.native_default_project()
        document = json.loads(project.read_text())
        document["component_class_settings"]["meta"]["version"] = 1
        project.write_text(json.dumps(document))
        original = self.source_snapshot()
        for target in ("9.0", "10.0"):
            for drop in (False, True):
                with self.subTest(target=target, drop=drop):
                    option = ["--drop-approximations"] if drop else []
                    output = self.root / f"future-components-{target}-{drop}"
                    report_path = self.root / f"future-components-{target}-{drop}.json"
                    result = self.cli("project", "downgrade", project, "--target", target,
                                      "--force", "--output", output, "--report-json", report_path, *option)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertTrue(json.loads(report_path.read_text())["blocked"])
                    self.assertFalse(output.exists())
                    self.assertEqual(self.source_snapshot(), original)


    def test_native_time_domain_tuning_keeps_copper_but_loses_v9_editability(self):
        native = Path(__file__).resolve().parents[2] / "data/pcbnew/issue24772/fw16_MCIO8i.kicad_pcb"
        board = self.source / "time_domain.kicad_pcb"
        project = self.source / "time_domain.kicad_pro"
        shutil.copyfile(native, board)
        shutil.copyfile(native.with_suffix(".kicad_pro"), project)
        original = self.source_snapshot()
        original_board = board.read_text()
        self.assertEqual(original_board.count("(type tuning_pattern)"), 16)
        self.assertEqual(original_board.count("(is_time_domain yes)"), 16)

        for target in ("9.0", "10.0"):
            for drop in (False, True):
                with self.subTest(target=target, drop=drop):
                    option = ["--drop-approximations"] if drop else []
                    output = self.root / f"time-domain-{target}-{drop}.kicad_pcb"
                    self.require_success(self.cli("pcb", "downgrade", board, "--target", target,
                                                  "--dry-run", *option))
                    self.assertFalse(output.exists())
                    command = ("pcb", "downgrade", board, "--target", target,
                               "--output", output, *option)
                    result = self.cli(*command)

                    if target == "9.0":
                        self.assertNotEqual(result.returncode, 0)
                        self.assertFalse(output.exists())
                        self.require_success(self.cli(*command, "--force"))
                    else:
                        self.require_success(result)

                    converted = output.read_text()
                    self.assertEqual(converted.count("(segment"), original_board.count("(segment"))
                    self.assertEqual(converted.count("(via"), original_board.count("(via"))
                    if target == "9.0":
                        self.assertNotIn("(type tuning_pattern)", converted)
                        self.assertNotIn("(is_time_domain", converted)
                        self.assertNotIn("(target_delay", converted)
                    else:
                        self.assertEqual(converted.count("(type tuning_pattern)"), 16)
                        self.assertEqual(converted.count("(is_time_domain yes)"), 16)
                    self.assertEqual(self.source_snapshot(), original)


    def test_board_embedded_worksheets_keep_released_checksums(self):
        fixture = Path(__file__).resolve().parents[2] / "data/pcbnew/issue22102.kicad_pcb"
        board = self.source / "embedded.kicad_pcb"
        shutil.copyfile(fixture, board)
        original = self.source_snapshot()
        released_hashes = set(re.findall(r'\(checksum "([0-9A-F]{32})"\)', board.read_text()))
        self.assertEqual(released_hashes, {
            "6003EC72C5978E83C182073E67278FCF",
            "0D054F65E3E0563620A9E16BBBB777DD",
        })

        for target in ("9.0", "10.0"):
            for drop in (False, True):
                with self.subTest(target=target, drop=drop):
                    output = self.root / f"embedded-{target}-{drop}.kicad_pcb"
                    option = ["--drop-approximations"] if drop else []
                    self.require_success(self.cli("pcb", "downgrade", board, "--target", target,
                                                  "--output", output, *option))
                    exported_hashes = set(re.findall(r'\(checksum "([0-9A-F]{32})"\)',
                                                   output.read_text()))
                    self.assertEqual(exported_hashes, released_hashes)
                    self.assertEqual(self.source_snapshot(), original)


    def test_schematic_embedded_worksheet_uses_released_checksum(self):
        fixture = REGRESSIONS / "embedded_files"
        names = ("embedded_files.kicad_sch", "embedded_files.kicad_pro", "child.kicad_sch")
        fixture_original = {name: (fixture / name).read_bytes() for name in names}
        for name in names:
            shutil.copyfile(fixture / name, self.source / name)
        original = self.source_snapshot()

        def worksheet_record(text):
            match = re.search(r'\(file\s*\(name "custom_ds\.kicad_wks"\)\s*'
                              r'\(type worksheet\)\s*\(data\s*\|(.*?)\|\s*\)\s*'
                              r'\(checksum "([0-9A-F]{32})"\)', text, re.S)
            self.assertIsNotNone(match)
            return "".join(match.group(1).split()), match.group(2)

        encoded, source_checksum = worksheet_record((self.source / names[0]).read_text())
        self.assertTrue(encoded)
        self.assertEqual(source_checksum, "B443EC325AC41F5D1FA8E96D51EE88DD")
        for target in ("9.0", "10.0"):
            for drop in (False, True):
                with self.subTest(target=target, drop=drop):
                    output = self.root / f"schematic-embedded-{target}-{drop}"
                    option = ["--drop-approximations"] if drop else []
                    self.require_success(self.cli("sch", "downgrade", self.source / names[0],
                                                  "--target", target, "--force", "--output", output,
                                                  *option))
                    exported_encoded, checksum = worksheet_record((output / names[0]).read_text())
                    self.assertEqual(checksum, "7594C3CC0E90420B2EEF650E4BACCB21")
                    self.assertEqual(exported_encoded, encoded)
                    self.assertTrue((output / names[2]).is_file())
                    self.assertEqual(self.source_snapshot(), original)
                    self.assertEqual({name: (fixture / name).read_bytes() for name in names},
                                     fixture_original)


    def test_pcb_existing_outputs_and_aliases_are_refused(self):
        source = self.source / "body_styles.kicad_pcb"
        original = self.source_snapshot()
        for target in ("9.0", "10.0"):
            for kind in ("existing", "directory", "symlink", "broken-symlink", "hardlink", "missing-parent-alias"):
                with self.subTest(target=target, kind=kind):
                    output = self.root / f"existing-pcb-{target}-{kind}.kicad_pcb"
                    if kind == "existing":
                        output.write_bytes(b"keep this file")
                    elif kind == "directory":
                        output.mkdir()
                    elif kind == "symlink":
                        output.symlink_to(source)
                    elif kind == "broken-symlink":
                        output.symlink_to(self.root / "missing.kicad_pcb")
                    elif kind == "hardlink":
                        os.link(source, output)
                    else:
                        output = self.source / "missing" / ".." / source.name
                    saved = output.read_bytes() if output.is_file() else None
                    result = self.cli("pcb", "downgrade", source, "--target", target,
                                      "--force", "--output", output)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(self.source_snapshot(), original)
                    if saved is not None:
                        self.assertEqual(output.read_bytes(), saved)
                    if kind.endswith("symlink"):
                        self.assertTrue(output.is_symlink())

    def test_pcb_libraries_are_copied_beside_the_output(self):
        libraries = self.source / "Parts.pretty"
        libraries.mkdir()
        (libraries / "Hatch.kicad_mod").write_text(HATCH_FOOTPRINT)
        (libraries / "model.step").write_bytes(b"library asset")
        original = self.source_snapshot()
        for target in ("9.0", "10.0"):
            for drop in (False, True):
                with self.subTest(target=target, drop=drop):
                    folder = self.root / f"pcb-library-{target}-{drop}"
                    output = folder / "board.kicad_pcb"
                    option = ["--drop-approximations"] if drop else []
                    self.require_success(self.cli("pcb", "downgrade", self.source / "body_styles.kicad_pcb",
                                                  "--target", target, "--force", "--output", output,
                                                  "--libraries", libraries, *option))
                    exported = folder / "Parts.pretty/Hatch.kicad_mod"
                    self.assertTrue(exported.is_file())
                    self.assertIn("(version " + ("20241229" if target == "9.0" else "20260206") + ")",
                                  exported.read_text())
                    self.assertEqual((folder / "Parts.pretty/model.step").read_bytes(), b"library asset")
                    self.assertEqual(self.source_snapshot(), original)

    def test_library_copy_rejects_directory_links_and_recursive_outputs(self):
        libraries = self.source / "Libraries"
        libraries.mkdir()
        (libraries / "Hatch.kicad_mod").write_text(HATCH_FOOTPRINT)
        (libraries / "Hatch.kicad_sym").write_text(HATCH_SYMBOL)
        linked = libraries / "linked"
        linked.symlink_to(self.source, target_is_directory=True)
        original = self.source_snapshot()
        for kind in ("pcb", "sch"):
            with self.subTest(kind=kind):
                output = self.root / f"linked-library-{kind}"
                if kind == "pcb":
                    output = output / "board.kicad_pcb"
                result = self.cli(kind, "downgrade", self.source / ("body_styles.kicad_" + kind),
                                  "--target", "10.0", "--force", "--output", output,
                                  "--libraries", libraries)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output.exists())
                self.assertEqual(self.source_snapshot(), original)
        linked.unlink()
        for kind in ("pcb", "sch"):
            with self.subTest(kind=kind):
                output = libraries / f"nested-{kind}"
                if kind == "pcb":
                    output = output / "board.kicad_pcb"
                result = self.cli(kind, "downgrade", self.source / ("body_styles.kicad_" + kind),
                                  "--target", "10.0", "--force", "--output", output,
                                  "--libraries", libraries)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output.exists())
                self.assertEqual(self.source_snapshot(), original)


    def test_schematic_library_parse_failure_removes_export_copy(self):
        libraries = self.source / "MalformedSymbols"
        libraries.mkdir()
        (libraries / "Good.kicad_sym").write_text(HATCH_SYMBOL)
        (libraries / "Malformed.kicad_sym").write_text("(kicad_symbol_lib (version 20260830)\n")
        original = self.source_snapshot()
        for target in ("9.0", "10.0"):
            for drop in (False, True):
                with self.subTest(target=target, drop=drop):
                    output = self.root / f"symbol-parse-failure-{target}-{drop}"
                    option = ["--drop-approximations"] if drop else []
                    result = self.cli("sch", "downgrade", self.source / "body_styles.kicad_sch",
                                      "--target", target, "--force", "--output", output,
                                      "--libraries", libraries, *option)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertFalse(output.exists())
                    self.assertEqual(self.source_snapshot(), original)


    def test_schematic_library_copy_tree_cannot_overlap_sheet_or_project_outputs(self):
        fixture = REGRESSIONS / "bus_alias_connectivity"
        for collision in ("child-tree", "root-sheet", "companion-project"):
            source_directory = self.root / ("hierarchy-" + collision)
            shutil.copytree(fixture, source_directory)
            source = source_directory / "prefix_bus_alias.kicad_sch"
            if collision == "child-tree":
                (source_directory / "children").mkdir()
                shutil.move(source_directory / "subsheet1.kicad_sch",
                            source_directory / "children/subsheet1.kicad_sch")
                source.write_text(source.read_text().replace(
                    '(property "Sheetfile" "subsheet1.kicad_sch"',
                    '(property "Sheetfile" "children/subsheet1.kicad_sch"', 1))
                basename = "children"
            else:
                basename = "prefix_bus_alias.kicad_sch" if collision == "root-sheet" else "prefix_bus_alias.kicad_pro"
            libraries = self.root / ("external-" + collision) / basename
            libraries.mkdir(parents=True)
            (libraries / "Hatch.kicad_sym").write_text(HATCH_SYMBOL)
            shutil.copyfile(fixture / "subsheet2.kicad_sch", libraries / "subsheet1.kicad_sch")
            original = {str(p.relative_to(self.root)): p.read_bytes()
                        for directory in (source_directory, libraries)
                        for p in directory.rglob("*") if p.is_file()}
            for target in ("9.0", "10.0"):
                for drop in (False, True):
                    with self.subTest(collision=collision, target=target, drop=drop):
                        output = self.root / f"overlapping-library-{collision}-{target}-{drop}"
                        option = ["--drop-approximations"] if drop else []
                        result = self.cli("sch", "downgrade", source, "--target", target, "--force",
                                          "--output", output, "--libraries", libraries, *option)
                        self.assertNotEqual(result.returncode, 0)
                        self.assertFalse(output.exists())
                        self.assertEqual({str(p.relative_to(self.root)): p.read_bytes()
                                          for directory in (source_directory, libraries)
                                          for p in directory.rglob("*") if p.is_file()}, original)


    def test_pcb_library_parse_failure_keeps_existing_output_parent(self):
        libraries = self.source / "Malformed.pretty"
        libraries.mkdir()
        (libraries / "Good.kicad_mod").write_text(HATCH_FOOTPRINT)
        (libraries / "Malformed.kicad_mod").write_text('(footprint "Malformed" (version 20260901)\n')
        original = self.source_snapshot()
        for target in ("9.0", "10.0"):
            with self.subTest(target=target):
                output_directory = self.root / f"existing-parent-{target}"
                output_directory.mkdir()
                sentinel = output_directory / "keep.txt"
                sentinel.write_bytes(b"existing user data")
                output = output_directory / "board.kicad_pcb"
                result = self.cli("pcb", "downgrade", self.source / "body_styles.kicad_pcb",
                                  "--target", target, "--force", "--output", output, "--libraries", libraries)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(list(output_directory.iterdir()), [sentinel])
                self.assertEqual(sentinel.read_bytes(), b"existing user data")
                self.assertEqual(self.source_snapshot(), original)


if __name__ == "__main__":
    unittest.main()
