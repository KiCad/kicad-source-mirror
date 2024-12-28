/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020-2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <regex>

#include <pgm_base.h>
#include <board.h>
#include <layer_ids.h>
#include <layer_range.h>
#include <gal/graphics_abstraction_layer.h>
#include <panel_pcbnew_color_settings.h>
#include <math/vector2wx.h>
#include <widgets/wx_panel.h>
#include <pcbnew_settings.h>
#include <settings/settings_manager.h>
#include <footprint_preview_panel.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <pcb_painter.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <wx/treebook.h>


std::string g_previewBoard =
        "(kicad_pcb (version 20230620) (generator pcbnew)\n"
        "\n"
        "  (general\n"
        "    (thickness 1.6)\n"
        "  )\n"
        "\n"
        "  (paper \"A4\")\n"
        "  (layers\n"
        "    (0 \"F.Cu\" signal)\n"
        "    (31 \"B.Cu\" signal)\n"
        "    (32 \"B.Adhes\" user \"B.Adhesive\")\n"
        "    (33 \"F.Adhes\" user \"F.Adhesive\")\n"
        "    (34 \"B.Paste\" user)\n"
        "    (35 \"F.Paste\" user)\n"
        "    (36 \"B.SilkS\" user \"B.Silkscreen\")\n"
        "    (37 \"F.SilkS\" user \"F.Silkscreen\")\n"
        "    (38 \"B.Mask\" user)\n"
        "    (39 \"F.Mask\" user)\n"
        "    (40 \"Dwgs.User\" user \"User.Drawings\")\n"
        "    (41 \"Cmts.User\" user \"User.Comments\")\n"
        "    (42 \"Eco1.User\" user \"User.Eco1\")\n"
        "    (43 \"Eco2.User\" user \"User.Eco2\")\n"
        "    (44 \"Edge.Cuts\" user)\n"
        "    (45 \"Margin\" user)\n"
        "    (46 \"B.CrtYd\" user \"B.Courtyard\")\n"
        "    (47 \"F.CrtYd\" user \"F.Courtyard\")\n"
        "    (48 \"B.Fab\" user)\n"
        "    (49 \"F.Fab\" user)\n"
        "    (50 \"User.1\" user)\n"
        "    (51 \"User.2\" user)\n"
        "    (52 \"User.3\" user)\n"
        "    (53 \"User.4\" user)\n"
        "    (54 \"User.5\" user)\n"
        "    (55 \"User.6\" user)\n"
        "    (56 \"User.7\" user)\n"
        "    (57 \"User.8\" user)\n"
        "    (58 \"User.9\" user)\n"
        "  )\n"
        "\n"
        "  (setup\n"
        "    (pad_to_mask_clearance 0)\n"
        "    (pcbplotparams\n"
        "      (layerselection 0x00010fc_ffffffff)\n"
        "      (plot_on_all_layers_selection 0x0000000_00000000)\n"
        "      (disableapertmacros false)\n"
        "      (usegerberextensions false)\n"
        "      (usegerberattributes true)\n"
        "      (usegerberadvancedattributes true)\n"
        "      (creategerberjobfile true)\n"
        "      (dashed_line_dash_ratio 12.000000)\n"
        "      (dashed_line_gap_ratio 3.000000)\n"
        "      (svgprecision 6)\n"
        "      (plotframeref false)\n"
        "      (viasonmask false)\n"
        "      (mode 1)\n"
        "      (useauxorigin false)\n"
        "      (hpglpennumber 1)\n"
        "      (hpglpenspeed 20)\n"
        "      (hpglpendiameter 15.000000)\n"
        "      (pdf_front_fp_property_popups true)\n"
        "      (pdf_back_fp_property_popups true)\n"
        "      (dxfpolygonmode true)\n"
        "      (dxfimperialunits true)\n"
        "      (dxfusepcbnewfont true)\n"
        "      (psnegative false)\n"
        "      (psa4output false)\n"
        "      (plotreference true)\n"
        "      (plotvalue true)\n"
        "      (plotinvisibletext false)\n"
        "      (plotpadnumbers false)\n"
        "      (sketchpadsonfab false)\n"
        "      (subtractmaskfromsilk false)\n"
        "      (outputformat 1)\n"
        "      (mirror false)\n"
        "      (drillshape 1)\n"
        "      (scaleselection 1)\n"
        "      (outputdirectory \"\")\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (net 0 \"\")\n"
        "  (net 1 \"GND\")\n"
        "\n"
        "  (footprint \"Wire_Pads:SolderWirePad_single_1-2mmDrill\" (layer \"F.Cu\")\n"
        "    (tstamp 196cc548-e42d-4d1f-b07f-f00a85d6308b)\n"
        "    (at 22.7 34.09)\n"
        "    (property \"Reference\" \"\" (at 0 -3.81 0) (layer \"F.SilkS\") hide (tstamp 6d50c232-866c-4b86-8aff-b6f4f27af92c)\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "    )\n"
        "    (property \"Value\" \"SolderWirePad_single_1-2mmDrill\" (at 11.4 2.81 0) (layer \"F.Fab\") (tstamp b7a15261-2581-4434-810f-55e348906d24)\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "    )\n"
        "    (property \"Footprint\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp a6b3c7f8-29ac-4f13-86a1-b0dd4581afbe)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Datasheet\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 0adf18bc-873a-45c5-ba4c-e9c8b29d0909)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Description\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 60e1a8b7-e355-4adf-b996-956d4525cff6)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (attr through_hole)\n"
        "    (pad \"1\" thru_hole circle (at 0 0) (size 3.50012 3.50012) (drill 1.19888) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (net 1 \"GND\")\n"
        "      (tstamp 907d213c-9e76-496e-8ff8-f5d804b4ebf3)\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (footprint \"Capacitors_THT:CP_Radial_D5.0mm_P2.00mm\" (layer \"F.Cu\")\n"
        "    (tstamp 380485fd-bd1d-4d52-80be-e1a4da6db230)\n"
        "    (at 21.675 27.9525)\n"
        "    (descr \"CP, Radial series, Radial, pin pitch=2.00mm, , diameter=5mm, Electrolytic Capacitor\")\n"
        "    (tags \"CP Radial series Radial pin pitch 2.00mm  diameter 5mm Electrolytic Capacitor\")\n"
        "    (property \"Reference\" \"C48\" (at 0.9525 -3.33375 0) (layer \"F.SilkS\") (tstamp dfc53a4c-91a2-4c4c-b399-a47b59d95339)\n"
        "      (effects (font (size 1 1.2) (thickness 0.22)))\n"
        "    )\n"
        "    (property \"Value\" \"47uF\" (at 4.775 -0.00125 90) (layer \"F.Fab\") (tstamp 2d0cb149-f436-43e7-b1eb-c65c4629a1f1)\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "    )\n"
        "    (property \"Footprint\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 1d00ae27-ea08-4947-a007-a9df95b34a71)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Datasheet\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp fc15f046-980f-4738-8b14-d405039da18e)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Description\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp e6704392-962c-47bc-b177-1ab07d70310f)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (attr through_hole)\n"
        "    (fp_line (start -2.2 0) (end -1 0)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 68db5686-fc0b-47b8-9e2e-19d88b41181f))\n"
        "    (fp_line (start -1.6 -0.65) (end -1.6 0.65)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 65bc7811-5705-4b18-beae-ab88ae6b9449))\n"
        "    (fp_arc (start -1.30558 -1.18) (mid 1.000156 -2.59) (end 3.305722 -1.179722)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 3b98e4c6-9ef9-48e5-a1aa-c32e87964514))\n"
        "    (fp_arc (start 3.30558 -1.18) (mid 3.59 -0.000156) (end 3.305722 1.179722)\n"
        "      (stroke (width 0.5) (type solid)) (layer \"F.SilkS\") (tstamp 42adcf5e-59b0-492b-a4c4-95141c0a39f5))\n"
        "    (fp_arc (start 3.305722 1.179722) (mid 1.000156 2.59) (end -1.30558 1.18)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp e1a2da0b-cc0a-4697-99c2-b5a0a71a0bb2))\n"
        "    (fp_line (start -1.85 -2.85) (end -1.85 2.85)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 74efa428-d26e-4ad0-9f18-0faeac9dc1d4))\n"
        "    (fp_line (start -1.85 2.85) (end 3.85 2.85)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 989e58cc-09a4-48ad-b9cc-9a273bfa6446))\n"
        "    (fp_line (start 3.85 -2.85) (end -1.85 -2.85)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 7622f7f7-21e8-4de5-ad60-3b06ccba2287))\n"
        "    (fp_line (start 3.85 2.85) (end 3.85 -2.85)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp d49a971f-1061-4efe-acba-573b8dc99906))\n"
        "    (fp_line (start -2.2 0) (end -1 0)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp b4f62f30-a5f3-4d20-a9ad-086b777a9ac1))\n"
        "    (fp_line (start -1.6 -0.65) (end -1.6 0.65)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 8b6277e9-17a1-4f65-80cc-586ab7b3f75b))\n"
        "    (fp_circle (center 1 0) (end 3.5 0)\n"
        "      (stroke (width 0.1) (type solid)) (fill none) (layer \"F.Fab\") (tstamp 2bce3e04-d9c6-4eff-84a1-edabbb01864d))\n"
        "    (pad \"1\" thru_hole rect (at 0 0) (size 1.4 1.4) (drill 0.8) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (net 1 \"GND\")\n"
        "      (tstamp 7920b7f9-55c4-4584-bf5c-436e8ef95e74)\n"
        "    )\n"
        "    (pad \"2\" thru_hole circle (at 2 0) (size 1.6 1.6) (drill 0.8) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (net 1 \"GND\")\n"
        "      (tstamp e4c29d51-7825-48f6-aae3-5e0ea3eb6e7a)\n"
        "    )\n"
        "    (model \"${KICAD7_3DMODEL_DIR}/Capacitors_THT.3dshapes/CP_Radial_D5.0mm_P2.00mm.wrl\"\n"
        "      (offset (xyz 0 0 0))\n"
        "      (scale (xyz 1 1 1))\n"
        "      (rotate (xyz 0 0 0))\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (footprint \"Wire_Pads:SolderWirePad_single_1mmDrill\" locked (layer \"F.Cu\")\n"
        "    (tstamp 473e3291-e4a8-4615-824b-1aa98b60e481)\n"
        "    (at 44.8525 24.46)\n"
        "    (property \"Reference\" \"Hidden RefDes\" (at -0.2835 7.47025 180) (layer \"F.SilkS\") hide (tstamp 02d2fb75-3428-4538-b599-cd811796b4d9)\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "    )\n"
        "    (property \"Value\" \"Hidden Value\" (at -0.2135 9.05025 0) (layer \"F.Fab\") hide (tstamp 2b366321-55c2-4e5a-8a51-2cf0a1c7c64e)\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "    )\n"
        "    (property \"Footprint\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp ca97624b-7df3-4ad8-a2a6-60d0a02d3efd)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Datasheet\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 62bc6f1f-e188-4e61-93bb-5273bf6d230c)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Description\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 5f3ff16c-b3ca-4ec3-a748-1d3730f9efd6)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (attr through_hole)\n"
        "    (fp_text user \"LED\" (at 0.15875 3.33375 90) (layer \"F.SilkS\") (tstamp f36fd79c-0dff-4a81-b4b4-9e222e82814f)\n"
        "      (effects (font (size 1 1.1) (thickness 0.25)))\n"
        "    )\n"
        "    (pad \"1\" thru_hole circle locked (at 0 0) (size 2.49936 2.49936) (drill 1.00076) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (tstamp ec712720-3eda-4621-ad13-dc832c80aef6)\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (footprint \"Wire_Pads:SolderWirePad_single_1mmDrill\" (layer \"F.Cu\")\n"
        "    (tstamp 621d7aa9-bc19-436a-9ebe-ef9cdda47103)\n"
        "    (at 44.8525 20.9675)\n"
        "    (property \"Reference\" \"\" (at 2.2225 0 90) (layer \"F.SilkS\") hide (tstamp dc982b23-a764-4ef4-924b-4c90cf7e51bc)\n"
        "      (effects (font (size 1.3 1.5) (thickness 0.25)))\n"
        "    )\n"
        "    (property \"Value\" \"Front Panel PWR\" (at 2.4575 1.72375 90) (layer \"F.Fab\") (tstamp a89dd8b8-81fc-41a9-a224-79f778e145ca)\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "    )\n"
        "    (property \"Footprint\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 274a983a-eb53-4c65-82c2-2d345192516a)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Datasheet\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 66d93d94-d821-4b6e-8c39-af797ff3e592)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Description\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 3c027815-7b5d-4456-994c-abd93403f141)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (attr through_hole)\n"
        "    (fp_text user \"12V\" (at 0.15875 -3.33375 90) (layer \"F.SilkS\") (tstamp 2ca6df83-f367-4aad-b4c2-1c3d5fe3b65e)\n"
        "      (effects (font (size 1 1.1) (thickness 0.25)))\n"
        "    )\n"
        "    (pad \"1\" thru_hole circle (at 0 0) (size 2.49936 2.49936) (drill 1.00076) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (net 1 \"GND\")\n"
        "      (tstamp 5c04f147-7717-4bbf-88a9-07e172a78a08)\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (footprint \"Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal\" (layer \"F.Cu\")\n"
        "    (tstamp 90b0a5fc-e8b0-4fa0-a8bd-198c1d756a29)\n"
        "    (at 28.9775 20.9675)\n"
        "    (descr \"Resistor, Axial_DIN0207 series, Axial, Horizontal, pin pitch=10.16mm, 0.25W = 1/4W, length*diameter=6.3*2.5mm^2, http://cdn-reichelt.de/documents/datenblatt/B400/1_4W%23YAG.pdf\")\n"
        "    (tags \"Resistor Axial_DIN0207 series Axial Horizontal pin pitch 10.16mm 0.25W = 1/4W length 6.3mm diameter 2.5mm\")\n"
        "    (property \"Reference\" \"R74\" (at 5.08 0 0) (layer \"F.SilkS\") (tstamp 9a6b9e83-9fea-4ae3-a69d-0de871c2078e)\n"
        "      (effects (font (size 1.4 1.6) (thickness 0.3)))\n"
        "    )\n"
        "    (property \"Value\" \"10K\" (at 12.2925 3.50375 90) (layer \"F.Fab\") (tstamp d73aa2bc-2e81-4c7a-8f14-4de04daa74aa)\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "    )\n"
        "    (property \"Footprint\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 8964928e-5ea8-4294-9c67-2d6d21142692)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Datasheet\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 11bd1bc4-f7c3-4227-93c2-84186af404bf)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Description\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 55bd9566-29a3-4408-88d8-16b844b1b553)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (attr through_hole)\n"
        "    (fp_line (start 0.98 0) (end 1.87 0)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 693536e9-30cc-41d2-aeb6-69447ddc61e2))\n"
        "    (fp_line (start 1.87 -1.31) (end 1.87 1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 4acaa78a-ffdc-4fff-b0bd-79bb54e1ac47))\n"
        "    (fp_line (start 1.87 1.31) (end 8.29 1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp ac3c2b1d-1f56-4ac7-9c5b-e93b60924ad7))\n"
        "    (fp_line (start 8.29 -1.31) (end 1.87 -1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 7f495b78-7217-4013-908a-2ff1dd367a10))\n"
        "    (fp_line (start 8.29 1.31) (end 8.29 -1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp fab7de09-20cf-4131-b969-27b695177867))\n"
        "    (fp_line (start 9.18 0) (end 8.29 0)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 26d8d0f7-c717-4cd1-aa43-446631d365b0))\n"
        "    (fp_line (start -1.05 -1.6) (end -1.05 1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 9dd0c083-9647-4d26-aebe-fabacbe66605))\n"
        "    (fp_line (start -1.05 1.6) (end 11.25 1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 3cb8ef09-2a19-4905-b082-c9534db84f47))\n"
        "    (fp_line (start 11.25 -1.6) (end -1.05 -1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp f602c95b-8818-4ea9-9f7a-b87bfa28f508))\n"
        "    (fp_line (start 11.25 1.6) (end 11.25 -1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp e01b3280-f982-4a3e-9f01-c1aaff70d915))\n"
        "    (fp_line (start 0 0) (end 1.93 0)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 4d964dda-4fec-485c-8618-dd88da973f8f))\n"
        "    (fp_line (start 1.93 -1.25) (end 1.93 1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 394e6f3e-8168-4351-b602-150b25a3e918))\n"
        "    (fp_line (start 1.93 1.25) (end 8.23 1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp cfc9a5ff-3c40-4d48-b7b6-8825e86fc710))\n"
        "    (fp_line (start 8.23 -1.25) (end 1.93 -1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 31a6e71b-e018-445e-80bf-4390f0da9d94))\n"
        "    (fp_line (start 8.23 1.25) (end 8.23 -1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 079bdccf-881c-4a76-bb47-fda88c1e8539))\n"
        "    (fp_line (start 10.16 0) (end 8.23 0)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 9985c0f0-7ff6-46bf-a22e-2fa42e34b9b1))\n"
        "    (pad \"1\" thru_hole circle (at 0 0) (size 1.8 1.8) (drill 0.85) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (net 1 \"GND\")\n"
        "      (tstamp def83b42-5ec2-442a-8f2b-ffd7b14105ce)\n"
        "    )\n"
        "    (pad \"2\" thru_hole circle (at 10.16 0) (size 1.8 1.8) (drill 0.85) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (tstamp 6386fdb7-f3d1-4839-9830-ab733b3e8d94)\n"
        "    )\n"
        "    (model \"${KICAD7_3DMODEL_DIR}/Resistors_THT.3dshapes/R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal.wrl\"\n"
        "      (offset (xyz 0 0 0))\n"
        "      (scale (xyz 0.393701 0.393701 0.393701))\n"
        "      (rotate (xyz 0 0 0))\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (footprint \"Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal\" (layer \"F.Cu\")\n"
        "    (tstamp c3f66ae9-669e-4889-915f-2cccae13db42)\n"
        "    (at 28.9775 27.9525)\n"
        "    (descr \"Resistor, Axial_DIN0207 series, Axial, Horizontal, pin pitch=10.16mm, 0.25W = 1/4W, length*diameter=6.3*2.5mm^2, http://cdn-reichelt.de/documents/datenblatt/B400/1_4W%23YAG.pdf\")\n"
        "    (tags \"Resistor Axial_DIN0207 series Axial Horizontal pin pitch 10.16mm 0.25W = 1/4W length 6.3mm diameter 2.5mm\")\n"
        "    (property \"Reference\" \"R75\" (at 5.08 0 0) (layer \"F.SilkS\") (tstamp ef6e671a-a5ac-4bc4-8077-708c1a429470)\n"
        "      (effects (font (size 1.4 1.6) (thickness 0.3)))\n"
        "    )\n"
        "    (property \"Value\" \"4K7\" (at 12.2925 0.06875 90) (layer \"F.Fab\") (tstamp 2c71cca7-0f57-4ce3-b3f4-64c98749588d)\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "    )\n"
        "    (property \"Footprint\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp ee2af228-77e9-4aea-8ccc-eabd18245aee)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Datasheet\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 71692634-84a4-4f4d-a683-67f68f7bd38d)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Description\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 44965976-d154-424e-b008-3052b4a3aede)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (attr through_hole)\n"
        "    (fp_line (start 0.98 0) (end 1.87 0)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 65f4508e-8af5-46fe-980c-f387540cb953))\n"
        "    (fp_line (start 1.87 -1.31) (end 1.87 1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 2376397b-6e25-4057-90ca-3ebd6d1da982))\n"
        "    (fp_line (start 1.87 1.31) (end 8.29 1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 997a5329-dd3e-4903-b5bf-b579c496c0c2))\n"
        "    (fp_line (start 8.29 -1.31) (end 1.87 -1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 1033538b-29a3-4da7-8f54-f17c12867391))\n"
        "    (fp_line (start 8.29 1.31) (end 8.29 -1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 79b31583-fa09-40ce-a54d-53e2344537e2))\n"
        "    (fp_line (start 9.18 0) (end 8.29 0)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 5796fca2-ca98-4d4b-bec4-d31bde85a650))\n"
        "    (fp_line (start -1.05 -1.6) (end -1.05 1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp da31697f-6c09-4894-9726-82e5965b76a9))\n"
        "    (fp_line (start -1.05 1.6) (end 11.25 1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 504e4184-74f3-4bb3-9268-148507ff437b))\n"
        "    (fp_line (start 11.25 -1.6) (end -1.05 -1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp c014700f-6b3e-4db2-9179-a63ca83ee80a))\n"
        "    (fp_line (start 11.25 1.6) (end 11.25 -1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp cf155cca-0e54-4902-9419-cd47ab3b7a6e))\n"
        "    (fp_line (start 0 0) (end 1.93 0)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 7ff54d88-df7e-42a5-9c8b-88764220ede8))\n"
        "    (fp_line (start 1.93 -1.25) (end 1.93 1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 495ba46e-e0f2-40df-bc2e-8273aef7c524))\n"
        "    (fp_line (start 1.93 1.25) (end 8.23 1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp dfab8391-a4f8-43c8-8cf8-6b7e6a977081))\n"
        "    (fp_line (start 8.23 -1.25) (end 1.93 -1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 3f4a794f-b45b-42fe-8e89-c86eb999b1ac))\n"
        "    (fp_line (start 8.23 1.25) (end 8.23 -1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 9d755f30-54b4-43f1-acea-b1a3ac435bd1))\n"
        "    (fp_line (start 10.16 0) (end 8.23 0)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp b999b5b2-327e-491b-9266-35b8482540c8))\n"
        "    (pad \"1\" thru_hole circle (at 0 0) (size 1.8 1.8) (drill 0.85) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (net 1 \"GND\")\n"
        "      (tstamp 5a702112-f16c-4b59-9902-914db0fa4360)\n"
        "    )\n"
        "    (pad \"2\" thru_hole circle (at 10.16 0) (size 1.8 1.8) (drill 0.85) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (tstamp b2b0d740-b086-433e-90f5-ffe119efdd07)\n"
        "    )\n"
        "    (model \"${KICAD7_3DMODEL_DIR}/Resistors_THT.3dshapes/R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal.wrl\"\n"
        "      (offset (xyz 0 0 0))\n"
        "      (scale (xyz 0.393701 0.393701 0.393701))\n"
        "      (rotate (xyz 0 0 0))\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (footprint \"Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal\" (layer \"F.Cu\")\n"
        "    (tstamp e072334e-f411-41c9-ade0-5444e440897b)\n"
        "    (at 39.1375 24.46 180)\n"
        "    (descr \"Resistor, Axial_DIN0207 series, Axial, Horizontal, pin pitch=10.16mm, 0.25W = 1/4W, length*diameter=6.3*2.5mm^2, http://cdn-reichelt.de/documents/datenblatt/B400/1_4W%23YAG.pdf\")\n"
        "    (tags \"Resistor Axial_DIN0207 series Axial Horizontal pin pitch 10.16mm 0.25W = 1/4W length 6.3mm diameter 2.5mm\")\n"
        "    (property \"Reference\" \"R73\" (at 5.08 0 180) (layer \"F.SilkS\") (tstamp 014cf222-f3a5-4b8e-8488-4309307a6bdc)\n"
        "      (effects (font (size 1.3 1.5) (thickness 0.25)))\n"
        "    )\n"
        "    (property \"Value\" \"10K\" (at -2.0825 3.53875 -90) (layer \"F.Fab\") (tstamp 2deddb35-a3f9-4993-a1ed-8c3208648d4e)\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "    )\n"
        "    (property \"Footprint\" \"\" (at 0 0 180 unlocked) (layer \"F.Fab\") hide (tstamp 5deb9256-369a-47c4-9483-b523df7a348d)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Datasheet\" \"\" (at 0 0 180 unlocked) (layer \"F.Fab\") hide (tstamp 1549b932-fa71-4a3d-b905-325ff0cc5e7b)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Description\" \"\" (at 0 0 180 unlocked) (layer \"F.Fab\") hide (tstamp 7e3b28e4-b9cb-43a6-a317-f239e8ff0ca9)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (attr through_hole)\n"
        "    (fp_line (start 9.18 0) (end 8.29 0)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 7007fd01-2893-48e3-8840-341e7cb4ec8d))\n"
        "    (fp_line (start 8.29 1.31) (end 8.29 -1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 5085ec5b-cfed-4f7c-bc9b-f7148c4b7222))\n"
        "    (fp_line (start 8.29 -1.31) (end 1.87 -1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 7a872980-3288-4161-8107-5d277e19ca53))\n"
        "    (fp_line (start 1.87 1.31) (end 8.29 1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 485d8c5a-a3d7-4c96-a3f3-f4b630b27e17))\n"
        "    (fp_line (start 1.87 -1.31) (end 1.87 1.31)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 0de0f2c4-5641-4214-9afa-5e4cbfe4ee44))\n"
        "    (fp_line (start 0.98 0) (end 1.87 0)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp a7b6dccc-6e78-4d89-adc7-7c6cfb560515))\n"
        "    (fp_line (start 11.25 1.6) (end 11.25 -1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 87f28b49-9648-446b-85a9-34010f350294))\n"
        "    (fp_line (start 11.25 -1.6) (end -1.05 -1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 2db3a7de-4ff9-4f7c-8023-a96c3233363e))\n"
        "    (fp_line (start -1.05 1.6) (end 11.25 1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 8b0e0021-dee0-4fba-85b0-dc1550b5b15c))\n"
        "    (fp_line (start -1.05 -1.6) (end -1.05 1.6)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 9324d473-26e5-4c7d-95c0-367ba971acd7))\n"
        "    (fp_line (start 10.16 0) (end 8.23 0)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 7529bced-6b8a-483c-9aee-d7c487d3a292))\n"
        "    (fp_line (start 8.23 1.25) (end 8.23 -1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 65eee186-e6fe-44b5-a7e4-202672bff890))\n"
        "    (fp_line (start 8.23 -1.25) (end 1.93 -1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp c0322b20-ac45-4f20-bbab-44368f60ac5d))\n"
        "    (fp_line (start 1.93 1.25) (end 8.23 1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 1af02505-0a10-49cf-8a95-d3c06be3152d))\n"
        "    (fp_line (start 1.93 -1.25) (end 1.93 1.25)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 73b1c134-2d49-42d8-b1ba-e81b8df404ba))\n"
        "    (fp_line (start 0 0) (end 1.93 0)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 001328a4-472c-4707-a7bf-46a7a15f91f6))\n"
        "    (pad \"1\" thru_hole circle (at 0 0 180) (size 1.8 1.8) (drill 0.85) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (tstamp 945fccb0-61fe-41e2-8f09-c8a33f294a3e)\n"
        "    )\n"
        "    (pad \"2\" thru_hole circle (at 10.16 0 180) (size 1.8 1.8) (drill 0.85) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (tstamp d8e786d3-14bf-403c-80db-810abc9eda53)\n"
        "    )\n"
        "    (model \"${KICAD7_3DMODEL_DIR}/Resistors_THT.3dshapes/R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal.wrl\"\n"
        "      (offset (xyz 0 0 0))\n"
        "      (scale (xyz 0.393701 0.393701 0.393701))\n"
        "      (rotate (xyz 0 0 0))\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (footprint \"Capacitors_THT:CP_Radial_D5.0mm_P2.00mm\" (layer \"F.Cu\")\n"
        "    (tstamp fa4fd0e8-6269-4cb7-a049-03c4e343895c)\n"
        "    (at 21.675 20.9675)\n"
        "    (descr \"CP, Radial series, Radial, pin pitch=2.00mm, , diameter=5mm, Electrolytic Capacitor\")\n"
        "    (tags \"CP Radial series Radial pin pitch 2.00mm  diameter 5mm Electrolytic Capacitor\")\n"
        "    (property \"Reference\" \"C47\" (at 0.9525 -3.33375 0) (layer \"F.SilkS\") (tstamp f13eb790-c841-466d-a0c1-07116ebc1073)\n"
        "      (effects (font (size 1 1.2) (thickness 0.22)))\n"
        "    )\n"
        "    (property \"Value\" \"47uF\" (at 4.775 -0.01625 90) (layer \"F.Fab\") (tstamp 027137f5-363d-41b8-8495-34c7a18a7117)\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "    )\n"
        "    (property \"Footprint\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 63aab932-9fb6-4e00-9306-6580e57bedf0)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Datasheet\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 0b3af576-09fa-4f29-a8d1-210aff851036)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Description\" \"\" (at 0 0 0 unlocked) (layer \"F.Fab\") hide (tstamp 054366ad-a326-49c2-a4f4-117002916f89)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (attr through_hole)\n"
        "    (fp_line (start -2.2 0) (end -1 0)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp d0ff0d50-cd0b-459a-8f2d-e84c2cb66ac5))\n"
        "    (fp_line (start -1.6 -0.65) (end -1.6 0.65)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 87ac9cb8-d215-4372-8cd3-4abaacc0bf33))\n"
        "    (fp_arc (start -1.30558 -1.18) (mid 1.000156 -2.59) (end 3.305722 -1.179722)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp 6fdf8300-e925-4956-a69a-91818d6e51c8))\n"
        "    (fp_arc (start 3.30558 -1.18) (mid 3.59 -0.000156) (end 3.305722 1.179722)\n"
        "      (stroke (width 0.5) (type solid)) (layer \"F.SilkS\") (tstamp 4404d1b4-9af5-4b29-a1e9-a89d90f6d256))\n"
        "    (fp_arc (start 3.305722 1.179722) (mid 1.000156 2.59) (end -1.30558 1.18)\n"
        "      (stroke (width 0.12) (type solid)) (layer \"F.SilkS\") (tstamp ad60c624-9f4d-4a30-a5f4-59f698eff831))\n"
        "    (fp_line (start -1.85 -2.85) (end -1.85 2.85)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 5ed611e7-0ad0-4da5-adb6-fcbf8512cd95))\n"
        "    (fp_line (start -1.85 2.85) (end 3.85 2.85)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 98571876-9364-410c-ac47-f290b1e46d98))\n"
        "    (fp_line (start 3.85 -2.85) (end -1.85 -2.85)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp 6b007cca-ff72-43ca-ba72-b26458c5a115))\n"
        "    (fp_line (start 3.85 2.85) (end 3.85 -2.85)\n"
        "      (stroke (width 0.05) (type solid)) (layer \"F.CrtYd\") (tstamp ba016114-5eb4-430b-ab2e-dbefa70d896a))\n"
        "    (fp_line (start -2.2 0) (end -1 0)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 68c156d0-784a-4c9c-b97c-a32d6c3aa3b9))\n"
        "    (fp_line (start -1.6 -0.65) (end -1.6 0.65)\n"
        "      (stroke (width 0.1) (type solid)) (layer \"F.Fab\") (tstamp 900d1364-baa6-4fae-a49a-97678ffcc675))\n"
        "    (fp_circle (center 1 0) (end 3.5 0)\n"
        "      (stroke (width 0.1) (type solid)) (fill none) (layer \"F.Fab\") (tstamp e572501a-055a-4591-8f6b-f3bf2e7ee86b))\n"
        "    (pad \"1\" thru_hole rect (at 0 0) (size 1.4 1.4) (drill 0.8) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (net 1 \"GND\")\n"
        "      (tstamp 7ef87fdc-802f-4acb-9867-18732d4a7e7a)\n"
        "    )\n"
        "    (pad \"2\" thru_hole circle (at 2 0) (size 1.6 1.6) (drill 0.8) (layers \"*.Cu\" \"*.Mask\")\n"
        "      (net 1 \"GND\")\n"
        "      (tstamp 1ad1f5f9-68f6-4131-9e09-bd1d361fc282)\n"
        "    )\n"
        "    (model \"${KICAD7_3DMODEL_DIR}/Capacitors_THT.3dshapes/CP_Radial_D5.0mm_P2.00mm.wrl\"\n"
        "      (offset (xyz 0 0 0))\n"
        "      (scale (xyz 1 1 1))\n"
        "      (rotate (xyz 0 0 0))\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (gr_rect (start 15.65 13.74) (end 52.5 38.8)\n"
        "    (stroke (width 0.05) (type solid)) (fill none) (layer \"Edge.Cuts\") (tstamp a879b274-bf89-4c34-8db0-6e2a7291f014))\n"
        "  (dimension (type aligned) (layer \"Margin\") (tstamp 6db437a2-8229-43a5-8bd8-1a58ddb201b9)\n"
        "    (pts (xy 44.88 24.5) (xy 44.88 20.9))\n"
        "    (height 9.59)\n"
        "    (gr_text \"0.1417 in\" (at 53.581 22.7 90) (layer \"Margin\") (tstamp 6db437a2-8229-43a5-8bd8-1a58ddb201b9)\n"
        "      (effects (font (size 0.762 0.762) (thickness 0.127)))\n"
        "    )\n"
        "    (format (prefix \"\") (suffix \"\") (units 0) (units_format 1) (precision 4))\n"
        "    (style (thickness 0.0508) (arrow_length 1.27) (text_position_mode 0) (extension_height 0.58642) (extension_offset 0) keep_text_aligned)\n"
        "  )\n"
        "\n"
        "  (segment (start 44.8525 24.46) (end 39.1375 24.46) (width 0.381) (layer \"F.Cu\") (net 0) (tstamp 7ca12133-777b-47ba-bbd3-cb8a544b2671))\n"
        "  (segment (start 33.105 32.74025) (end 33.105 27.9525) (width 0.381) (layer \"F.Cu\") (net 1) (tstamp 069bdf66-094e-4225-a62a-5e9ac38209e5))\n"
        "  (segment (start 33.105 27.9525) (end 28.9775 27.9525) (width 0.381) (layer \"F.Cu\") (net 1) (tstamp 6fd4daae-acd4-44ce-bc05-d098c9a39286))\n"
        "  (segment (start 34.0575 32.74025) (end 34.0575 20.9675) (width 0.381) (layer \"F.Cu\") (net 1) (tstamp 730eecf7-4831-459b-b926-557f809e0c1c))\n"
        "  (segment (start 35.01 19.22125) (end 35.01 32.74025) (width 0.381) (layer \"F.Cu\") (net 1) (tstamp c69ca14f-e0e6-485f-a878-4f0a683c3915))\n"
        "  (segment (start 34.0575 20.9675) (end 28.9775 20.9675) (width 0.381) (layer \"F.Cu\") (net 1) (tstamp e5e766f6-987a-4192-9c34-760f17795c27))\n"
        "  (via (at 35.01 19.22125) (size 0.75) (drill 0.35) (layers \"F.Cu\" \"B.Cu\") (net 1) (tstamp a3ff2347-4667-4dde-8969-ef5af9da726e))\n"
        "  (segment (start 35.01 19.22125) (end 44.8525 19.22125) (width 0.762) (layer \"B.Cu\") (net 1) (tstamp 236da8c8-0850-4652-aecc-efa2304d6271))\n"
        "  (segment (start 44.8525 19.22125) (end 44.8525 20.9675) (width 0.762) (layer \"B.Cu\") (net 1) (tstamp 351a1a87-af11-41b0-80e3-ad556442a4d5))\n"
        "  (segment (start 21.68 27.9475) (end 21.675 27.9525) (width 0.762) (layer \"B.Cu\") (net 1) (tstamp 353c08b1-5615-4291-a96c-91924dfe8626))\n"
        "  (segment (start 35.01 19.22125) (end 21.68 19.22125) (width 0.762) (layer \"B.Cu\") (net 1) (tstamp 41941280-cdfb-4779-bd2b-633d67a413a7))\n"
        "  (segment (start 21.68 19.22125) (end 21.68 27.9475) (width 0.762) (layer \"B.Cu\") (net 1) (tstamp 5b37df1f-f5c1-4c27-b720-6f0e5bc0eb66))\n"
        "  (segment (start 28.9775 20.9675) (end 23.675 20.9675) (width 0.381) (layer \"B.Cu\") (net 1) (tstamp e365420e-bf4e-4c36-a39f-b4aac3c33817))\n"
        "  (segment (start 28.9775 27.9525) (end 23.675 27.9525) (width 0.381) (layer \"B.Cu\") (net 1) (tstamp fa7580a1-979a-4da5-a6ea-fde491f0a6aa))\n"
        "\n"
        "  (zone (net 1) (net_name \"GND\") (layer \"F.Cu\") (tstamp b32d7e64-2aaa-46f7-a99f-f3198188b35a) (hatch edge 0.508)\n"
        "    (connect_pads (clearance 0.508))\n"
        "    (min_thickness 0.254) (filled_areas_thickness no)\n"
        "    (fill yes (thermal_gap 0.508) (thermal_bridge_width 0.508))\n"
        "    (polygon\n"
        "      (pts\n"
        "        (xy 37.06 36.6)\n"
        "        (xy 19.8 36.7)\n"
        "        (xy 19.8 31.4)\n"
        "        (xy 37.06 31.4)\n"
        "      )\n"
        "    )\n"
        "    (filled_polygon\n"
        "      (layer \"F.Cu\")\n"
        "      (pts\n"
        "        (xy 36.982601 31.408667)\n"
        "        (xy 37.023803 31.436197)\n"
        "        (xy 37.051333 31.477399)\n"
        "        (xy 37.061 31.526)\n"
        "        (xy 37.061 36.474728)\n"
        "        (xy 37.051333 36.523329)\n"
        "        (xy 37.023803 36.564531)\n"
        "        (xy 36.982601 36.592061)\n"
        "        (xy 36.934736 36.601726)\n"
        "        (xy 19.926736 36.700266)\n"
        "        (xy 19.87808 36.69088)\n"
        "        (xy 19.836719 36.663589)\n"
        "        (xy 19.808951 36.622548)\n"
        "        (xy 19.799002 36.574004)\n"
        "        (xy 19.799 36.573268)\n"
        "        (xy 19.799 35.85909)\n"
        "        (xy 21.291534 35.85909)\n"
        "        (xy 21.301092 35.867696)\n"
        "        (xy 21.54849 36.037092)\n"
        "        (xy 21.816124 36.172285)\n"
        "        (xy 22.099282 36.27089)\n"
        "        (xy 22.392989 36.331179)\n"
        "        (xy 22.692107 36.352097)\n"
        "        (xy 22.991346 36.33327)\n"
        "        (xy 23.285483 36.275029)\n"
        "        (xy 23.569315 36.178405)\n"
        "        (xy 23.837882 36.045087)\n"
        "        (xy 24.086467 35.877414)\n"
        "        (xy 24.107826 35.858451)\n"
        "        (xy 22.7 34.450625)\n"
        "        (xy 21.291534 35.85909)\n"
        "        (xy 19.799 35.85909)\n"
        "        (xy 19.799 34.034731)\n"
        "        (xy 20.438564 34.034731)\n"
        "        (xy 20.451121 34.334312)\n"
        "        (xy 20.503187 34.629592)\n"
        "        (xy 20.593847 34.91539)\n"
        "        (xy 20.721516 35.186698)\n"
        "        (xy 20.883939 35.438728)\n"
        "        (xy 20.932999 35.496375)\n"
        "        (xy 22.339374 34.09)\n"
        "        (xy 22.339373 34.089999)\n"
        "        (xy 23.060625 34.089999)\n"
        "        (xy 24.467405 35.49678)\n"
        "        (xy 24.506601 35.451371)\n"
        "        (xy 24.67078 35.200481)\n"
        "        (xy 24.800337 34.930074)\n"
        "        (xy 24.892992 34.644914)\n"
        "        (xy 24.947116 34.35002)\n"
        "        (xy 24.957135 33.940065)\n"
        "        (xy 24.917481 33.642876)\n"
        "        (xy 24.838866 33.353527)\n"
        "        (xy 24.722672 33.077112)\n"
        "        (xy 24.570949 32.818507)\n"
        "        (xy 24.466196 32.684428)\n"
        "        (xy 23.060625 34.089999)\n"
        "        (xy 22.339373 34.089999)\n"
        "        (xy 20.931778 32.682403)\n"
        "        (xy 20.77745 32.897964)\n"
        "        (xy 20.636682 33.162708)\n"
        "        (xy 20.532168 33.443739)\n"
        "        (xy 20.465738 33.736132)\n"
        "        (xy 20.438564 34.034731)\n"
        "        (xy 19.799 34.034731)\n"
        "        (xy 19.799 32.323753)\n"
        "        (xy 21.294378 32.323753)\n"
        "        (xy 22.699999 33.729374)\n"
        "        (xy 24.106226 32.323147)\n"
        "        (xy 23.932032 32.192833)\n"
        "        (xy 23.670303 32.046557)\n"
        "        (xy 23.391518 31.936179)\n"
        "        (xy 23.100581 31.86364)\n"
        "        (xy 22.802613 31.830217)\n"
        "        (xy 22.502848 31.836496)\n"
        "        (xy 22.206534 31.882368)\n"
        "        (xy 21.918888 31.967027)\n"
        "        (xy 21.644986 32.088976)\n"
        "        (xy 21.389596 32.246094)\n"
        "        (xy 21.294378 32.323753)\n"
        "        (xy 19.799 32.323753)\n"
        "        (xy 19.799 31.526)\n"
        "        (xy 19.808667 31.477399)\n"
        "        (xy 19.836197 31.436197)\n"
        "        (xy 19.877399 31.408667)\n"
        "        (xy 19.926 31.399)\n"
        "        (xy 36.934 31.399)\n"
        "      )\n"
        "    )\n"
        "  )\n"
        ")\n";

std::set<int> g_excludedLayers =
        {
            LAYER_VIAS,
            LAYER_VIA_THROUGH,
            LAYER_VIA_BBLIND,
            LAYER_VIA_MICROVIA,
            LAYER_FOOTPRINTS_FR,
            LAYER_FOOTPRINTS_BK,
            LAYER_FP_VALUES,
            LAYER_FP_REFERENCES,
            LAYER_TRACKS,
            LAYER_FP_TEXT,
            GAL_LAYER_ID_START + 6,     // where LAYER_MOD_TEXT_BK (deprecated) used to be
            GAL_LAYER_ID_START + 7,     // where LAYER_HIDDEN_TEXT (deprecated) used to be
            GAL_LAYER_ID_START + 9,    // where LAYER_PADS_SMD_FR (deprecated) used to be
            GAL_LAYER_ID_START + 10,    // where LAYER_PADS_SMD_BK (deprecated) used to be
            GAL_LAYER_ID_START + 14,    // where LAYER_NO_CONNECTS (deprecated) used to be
            GAL_LAYER_ID_START + 20,    // where LAYER_PADS_TH (deprecated) used to be
            LAYER_PAD_PLATEDHOLES,
            LAYER_PAD_HOLEWALLS,
            LAYER_GP_OVERLAY,
            LAYER_DRAW_BITMAPS,
            LAYER_MARKER_SHADOWS
        };


PANEL_PCBNEW_COLOR_SETTINGS::PANEL_PCBNEW_COLOR_SETTINGS( wxWindow* aParent, BOARD* aBoard ) :
        PANEL_COLOR_SETTINGS( aParent ),
        UNITS_PROVIDER( pcbIUScale, EDA_UNITS::MILLIMETRES ),
        m_preview( nullptr ),
        m_page( nullptr ),
        m_titleBlock( nullptr ),
        m_board( aBoard )
{
    m_colorNamespace = "board";

    SETTINGS_MANAGER& mgr     = Pgm().GetSettingsManager();
    PCBNEW_SETTINGS*  cfg     = mgr.GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );
    COLOR_SETTINGS*   current = mgr.GetColorSettings( cfg->m_ColorTheme );

    // Saved theme doesn't exist?  Reset to default
    if( current->GetFilename() != cfg->m_ColorTheme )
        cfg->m_ColorTheme = current->GetFilename();

    createThemeList( cfg->m_ColorTheme );

    // Currently this only applies to eeschema
    m_optOverrideColors->Hide();

    m_currentSettings = new COLOR_SETTINGS( *current );

    for( int id = GAL_LAYER_ID_START; id < GAL_LAYER_ID_BITMASK_END; id++ )
    {
        if( g_excludedLayers.count( id ) )
            continue;

        m_validLayers.push_back( id );
    }

    // These layers are not in GAL_LAYER_ID_START ... GAL_LAYER_ID_BITMASK_END
    m_validLayers.push_back( LAYER_LOCKED_ITEM_SHADOW );
    m_validLayers.push_back( LAYER_CONFLICTS_SHADOW );
    m_validLayers.push_back( LAYER_PAGE_LIMITS );
    m_validLayers.push_back( LAYER_DRC_WARNING );
    m_validLayers.push_back( LAYER_DRC_EXCLUSION );
    m_validLayers.push_back( NETNAMES_LAYER_ID_START );
    m_validLayers.push_back( LAYER_PAD_NETNAMES );

    // NOTE: Main board layers are added by createSwatches()

    m_backgroundLayer = LAYER_PCB_BACKGROUND;
}


PANEL_PCBNEW_COLOR_SETTINGS::~PANEL_PCBNEW_COLOR_SETTINGS()
{
    delete m_page;
    delete m_titleBlock;
    delete m_currentSettings;
}


bool PANEL_PCBNEW_COLOR_SETTINGS::TransferDataFromWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    PCBNEW_SETTINGS*  cfg = mgr.GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

    cfg->m_ColorTheme = m_currentSettings->GetFilename();

    return true;
}


bool PANEL_PCBNEW_COLOR_SETTINGS::TransferDataToWindow()
{
    zoomFitPreview();
    return true;
}


void PANEL_PCBNEW_COLOR_SETTINGS::createSwatches()
{
    std::sort( m_validLayers.begin(), m_validLayers.end(),
               []( int a, int b )
               {
                   return LayerName( a ) < LayerName( b );
               } );

    // Don't sort aBoard layers by name
    size_t i = 0;

    for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, MAX_CU_LAYERS ) )
        m_validLayers.insert( m_validLayers.begin() + i++, layer );

    for( PCB_LAYER_ID layer : LSET::AllNonCuMask().TechAndUserUIOrder() )
        m_validLayers.insert( m_validLayers.begin() + i++, layer );

    for( int layer : m_validLayers )
    {
        wxString name = LayerName( layer );

        if( m_board && layer >= PCBNEW_LAYER_ID_START && layer < PCB_LAYER_ID_COUNT )
            name = m_board->GetLayerName( static_cast<PCB_LAYER_ID>( layer ) );

        createSwatch( layer, name );
    }

    m_preview = FOOTPRINT_PREVIEW_PANEL::New( nullptr, m_panel1, this );
    m_preview->GetGAL()->SetAxesEnabled( false );

    m_previewPanelSizer->Add( m_preview, 1, wxEXPAND, 5 );

    createPreviewItems();
    Layout();
    updatePreview();
    zoomFitPreview();
}


void PANEL_PCBNEW_COLOR_SETTINGS::onNewThemeSelected()
{
    updatePreview();
}


void PANEL_PCBNEW_COLOR_SETTINGS::createPreviewItems()
{
    m_page       = new PAGE_INFO( PAGE_INFO::Custom );
    m_titleBlock = new TITLE_BLOCK;
    m_titleBlock->SetTitle( _( "Color Preview" ) );
    m_titleBlock->SetDate( wxDateTime::Now().FormatDate() );

    m_page->SetHeightMils( 5000 );
    m_page->SetWidthMils( 6000 );

    STRING_LINE_READER reader( g_previewBoard, wxT( "preview" ) );
    PCB_IO_KICAD_SEXPR         pi;

    try
    {
        pi.DoLoad( reader, m_preview->GetBoard(), nullptr, nullptr, 0 );
    }
    catch( const IO_ERROR& )
    {
        return;
    }

    m_preview->UpdateColors();
    m_preview->DisplayBoard( m_preview->GetBoard() );

    DS_PROXY_VIEW_ITEM* drawingSheet = new DS_PROXY_VIEW_ITEM( pcbIUScale, m_page, nullptr,
                                                               m_titleBlock, nullptr );
    drawingSheet->SetIsFirstPage( true );
    drawingSheet->SetColorLayer( LAYER_DRAWINGSHEET );
    drawingSheet->SetPageBorderColorLayer( LAYER_PAGE_LIMITS );
    m_preview->SetDrawingSheet( drawingSheet );

    zoomFitPreview();
}


void PANEL_PCBNEW_COLOR_SETTINGS::onColorChanged()
{
    updatePreview();
}


void PANEL_PCBNEW_COLOR_SETTINGS::ResetPanel()
{
    PANEL_COLOR_SETTINGS::ResetPanel();
    updatePreview();
}


void PANEL_PCBNEW_COLOR_SETTINGS::updatePreview()
{
    if( !m_preview )
        return;

    KIGFX::VIEW* view = m_preview->GetView();
    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );
    settings->LoadColors( m_currentSettings );

    m_preview->GetGAL()->SetClearColor( settings->GetBackgroundColor() );

    view->UpdateAllItems( KIGFX::COLOR );
    auto rect = m_preview->GetScreenRect();
    m_preview->Refresh( true, &rect );
}


void PANEL_PCBNEW_COLOR_SETTINGS::zoomFitPreview()
{
    if( m_preview )
    {
        KIGFX::VIEW* view = m_preview->GetView();
        BOX2I        bBox = m_preview->GetBoard()->GetBoundingBox();
        BOX2I        defaultBox = m_preview->GetDefaultViewBBox();

        view->SetScale( 1.0 );
        VECTOR2D screenSize = view->ToWorld( ToVECTOR2D( m_preview->GetClientSize() ), false );

        if( bBox.GetWidth() == 0 || bBox.GetHeight() == 0 )
            bBox = defaultBox;

        VECTOR2D vsize = bBox.GetSize();
        double scale = view->GetScale() / std::max( fabs( vsize.x / screenSize.x ),
                                                    fabs( vsize.y / screenSize.y ) );

        view->SetScale( scale / 1.1 );
        view->SetCenter( bBox.Centre() );
        m_preview->ForceRefresh();
    }
}


void PANEL_PCBNEW_COLOR_SETTINGS::OnSize( wxSizeEvent& aEvent )
{
    zoomFitPreview();
    aEvent.Skip();
}


