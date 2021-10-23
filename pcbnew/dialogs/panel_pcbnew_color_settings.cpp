/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <board.h>
#include <gal/gal_display_options.h>
#include <layer_ids.h>
#include <panel_pcbnew_color_settings.h>
#include <pcbnew_settings.h>
#include <pcb_edit_frame.h>
#include <settings/settings_manager.h>
#include <footprint_preview_panel.h>
#include <widgets/appearance_controls.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <pcb_painter.h>
#include <plugins/kicad/kicad_plugin.h>
#include <wx/treebook.h>


std::string g_previewBoard =
        "(kicad_pcb (version 20200724) (host pcbnew \"(5.99.0-2577-gd32bcd569-dirty)\")\n"
        "\n"
        "  (net 0 \"\")\n"
        "  (net 1 \"GND\")\n"
        "\n"
        "  (module \"Wire_Pads:SolderWirePad_single_1-2mmDrill\" (layer \"F.Cu\") (tedit 5F2C4AD9) (tstamp 196cc548-e42d-4d1f-b07f-f00a85d6308b)\n"
        "    (at 22.7 34.09)\n"
        "    (fp_text reference \"\" (at 0 -3.81) (layer \"F.SilkS\") hide\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "      (tstamp 6d50c232-866c-4b86-8aff-b6f4f27af92c)\n"
        "    )\n"
        "    (fp_text value \"SolderWirePad_single_1-2mmDrill\" (at 11.4 2.81) (layer \"F.Fab\")\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "      (tstamp b7a15261-2581-4434-810f-55e348906d24)\n"
        "    )\n"
        "    (pad \"1\" thru_hole circle (at 0 0) (size 3.50012 3.50012) (drill 1.19888) (layers *.Cu *.Mask)\n"
        "      (net 1 \"GND\") (tstamp 907d213c-9e76-496e-8ff8-f5d804b4ebf3))\n"
        "  )\n"
        "\n"
        "  (module \"Capacitors_THT:CP_Radial_D5.0mm_P2.00mm\" (layer \"F.Cu\") (tedit 5C22DE58) (tstamp 380485fd-bd1d-4d52-80be-e1a4da6db230)\n"
        "    (at 21.675 27.9525)\n"
        "    (descr \"CP, Radial series, Radial, pin pitch=2.00mm, , diameter=5mm, Electrolytic Capacitor\")\n"
        "    (tags \"CP Radial series Radial pin pitch 2.00mm  diameter 5mm Electrolytic Capacitor\")\n"
        "    (fp_text reference \"C48\" (at 0.9525 -3.33375) (layer \"F.SilkS\")\n"
        "      (effects (font (size 1 1.2) (thickness 0.22)))\n"
        "      (tstamp dfc53a4c-91a2-4c4c-b399-a47b59d95339)\n"
        "    )\n"
        "    (fp_text value \"47uF\" (at 4.775 -0.00125 90) (layer \"F.Fab\")\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "      (tstamp 2d0cb149-f436-43e7-b1eb-c65c4629a1f1)\n"
        "    )\n"
        "    (fp_line (start -1.6 -0.65) (end -1.6 0.65) (layer \"F.SilkS\") (width 0.12) (tstamp 65bc7811-5705-4b18-beae-ab88ae6b9449))\n"
        "    (fp_line (start -2.2 0) (end -1 0) (layer \"F.SilkS\") (width 0.12) (tstamp 68db5686-fc0b-47b8-9e2e-19d88b41181f))\n"
        "    (fp_arc (start 1 0) (end -1.30558 -1.18) (angle 125.8) (layer \"F.SilkS\") (width 0.12) (tstamp 3b98e4c6-9ef9-48e5-a1aa-c32e87964514))\n"
        "    (fp_arc (start 1 0) (end 3.30558 -1.18) (angle 54.2) (layer \"F.SilkS\") (width 0.5) (tstamp 42adcf5e-59b0-492b-a4c4-95141c0a39f5))\n"
        "    (fp_arc (start 1 0) (end -1.30558 1.18) (angle -125.8) (layer \"F.SilkS\") (width 0.12) (tstamp e1a2da0b-cc0a-4697-99c2-b5a0a71a0bb2))\n"
        "    (fp_line (start -1.85 -2.85) (end -1.85 2.85) (layer \"F.CrtYd\") (width 0.05) (tstamp 74efa428-d26e-4ad0-9f18-0faeac9dc1d4))\n"
        "    (fp_line (start 3.85 -2.85) (end -1.85 -2.85) (layer \"F.CrtYd\") (width 0.05) (tstamp 7622f7f7-21e8-4de5-ad60-3b06ccba2287))\n"
        "    (fp_line (start -1.85 2.85) (end 3.85 2.85) (layer \"F.CrtYd\") (width 0.05) (tstamp 989e58cc-09a4-48ad-b9cc-9a273bfa6446))\n"
        "    (fp_line (start 3.85 2.85) (end 3.85 -2.85) (layer \"F.CrtYd\") (width 0.05) (tstamp d49a971f-1061-4efe-acba-573b8dc99906))\n"
        "    (fp_line (start -1.6 -0.65) (end -1.6 0.65) (layer \"F.Fab\") (width 0.1) (tstamp 8b6277e9-17a1-4f65-80cc-586ab7b3f75b))\n"
        "    (fp_line (start -2.2 0) (end -1 0) (layer \"F.Fab\") (width 0.1) (tstamp b4f62f30-a5f3-4d20-a9ad-086b777a9ac1))\n"
        "    (fp_circle (center 1 0) (end 3.5 0) (layer \"F.Fab\") (width 0.1) (tstamp 2bce3e04-d9c6-4eff-84a1-edabbb01864d))\n"
        "    (pad \"1\" thru_hole rect (at 0 0) (size 1.4 1.4) (drill 0.8) (layers *.Cu *.Mask)\n"
        "      (net 1 \"GND\") (tstamp 7920b7f9-55c4-4584-bf5c-436e8ef95e74))\n"
        "    (pad \"2\" thru_hole circle (at 2 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n"
        "      (net 1 \"GND\") (tstamp e4c29d51-7825-48f6-aae3-5e0ea3eb6e7a))\n"
        "    (model \"${KICAD6_3DMODEL_DIR}/Capacitors_THT.3dshapes/CP_Radial_D5.0mm_P2.00mm.wrl\"\n"
        "      (offset (xyz 0 0 0))\n"
        "      (scale (xyz 1 1 1))\n"
        "      (rotate (xyz 0 0 0))\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (module \"Wire_Pads:SolderWirePad_single_1mmDrill\" (layer \"F.Cu\") (tedit 0) (tstamp 473e3291-e4a8-4615-824b-1aa98b60e481)\n"
        "    (at 44.8525 24.46)\n"
        "    (fp_text reference \"Hidden RefDes\" (at -0.2835 7.47025 180) (layer \"F.SilkS\") hide\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "      (tstamp 02d2fb75-3428-4538-b599-cd811796b4d9)\n"
        "    )\n"
        "    (fp_text value \"Hidden Value\" (at -0.2135 9.05025) (layer \"F.Fab\") hide\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "      (tstamp 2b366321-55c2-4e5a-8a51-2cf0a1c7c64e)\n"
        "    )\n"
        "    (fp_text user \"LED\" (at 0.15875 3.33375 90) (layer \"F.SilkS\")\n"
        "      (effects (font (size 1 1.1) (thickness 0.25)))\n"
        "      (tstamp f36fd79c-0dff-4a81-b4b4-9e222e82814f)\n"
        "    )\n"
        "    (pad \"1\" thru_hole circle (at 0 0) (size 2.49936 2.49936) (drill 1.00076) (layers *.Cu *.Mask) (tstamp ec712720-3eda-4621-ad13-dc832c80aef6))\n"
        "  )\n"
        "\n"
        "  (module \"Wire_Pads:SolderWirePad_single_1mmDrill\" (layer \"F.Cu\") (tedit 0) (tstamp 621d7aa9-bc19-436a-9ebe-ef9cdda47103)\n"
        "    (at 44.8525 20.9675)\n"
        "    (fp_text reference \"\" (at 2.2225 0 90) (layer \"F.SilkS\") hide\n"
        "      (effects (font (size 1.3 1.5) (thickness 0.25)))\n"
        "      (tstamp dc982b23-a764-4ef4-924b-4c90cf7e51bc)\n"
        "    )\n"
        "    (fp_text value \"Front Panel PWR\" (at 2.4575 1.72375 90) (layer \"F.Fab\")\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "      (tstamp a89dd8b8-81fc-41a9-a224-79f778e145ca)\n"
        "    )\n"
        "    (fp_text user \"12V\" (at 0.15875 -3.33375 90) (layer \"F.SilkS\")\n"
        "      (effects (font (size 1 1.1) (thickness 0.25)))\n"
        "      (tstamp 2ca6df83-f367-4aad-b4c2-1c3d5fe3b65e)\n"
        "    )\n"
        "    (pad \"1\" thru_hole circle (at 0 0) (size 2.49936 2.49936) (drill 1.00076) (layers *.Cu *.Mask)\n"
        "      (net 1 \"GND\") (tstamp 5c04f147-7717-4bbf-88a9-07e172a78a08))\n"
        "  )\n"
        "\n"
        "  (module \"Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal\" (layer \"F.Cu\") (tedit 5A1048BD) (tstamp 90b0a5fc-e8b0-4fa0-a8bd-198c1d756a29)\n"
        "    (at 28.9775 20.9675)\n"
        "    (descr \"Resistor, Axial_DIN0207 series, Axial, Horizontal, pin pitch=10.16mm, 0.25W = 1/4W, length*diameter=6.3*2.5mm^2, http://cdn-reichelt.de/documents/datenblatt/B400/1_4W%23YAG.pdf\")\n"
        "    (tags \"Resistor Axial_DIN0207 series Axial Horizontal pin pitch 10.16mm 0.25W = 1/4W length 6.3mm diameter 2.5mm\")\n"
        "    (fp_text reference \"R74\" (at 5.08 0) (layer \"F.SilkS\")\n"
        "      (effects (font (size 1.4 1.6) (thickness 0.3)))\n"
        "      (tstamp 9a6b9e83-9fea-4ae3-a69d-0de871c2078e)\n"
        "    )\n"
        "    (fp_text value \"10K\" (at 12.2925 3.50375 90) (layer \"F.Fab\")\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "      (tstamp d73aa2bc-2e81-4c7a-8f14-4de04daa74aa)\n"
        "    )\n"
        "    (fp_line (start 9.18 0) (end 8.29 0) (layer \"F.SilkS\") (width 0.12) (tstamp 26d8d0f7-c717-4cd1-aa43-446631d365b0))\n"
        "    (fp_line (start 1.87 -1.31) (end 1.87 1.31) (layer \"F.SilkS\") (width 0.12) (tstamp 4acaa78a-ffdc-4fff-b0bd-79bb54e1ac47))\n"
        "    (fp_line (start 0.98 0) (end 1.87 0) (layer \"F.SilkS\") (width 0.12) (tstamp 693536e9-30cc-41d2-aeb6-69447ddc61e2))\n"
        "    (fp_line (start 8.29 -1.31) (end 1.87 -1.31) (layer \"F.SilkS\") (width 0.12) (tstamp 7f495b78-7217-4013-908a-2ff1dd367a10))\n"
        "    (fp_line (start 1.87 1.31) (end 8.29 1.31) (layer \"F.SilkS\") (width 0.12) (tstamp ac3c2b1d-1f56-4ac7-9c5b-e93b60924ad7))\n"
        "    (fp_line (start 8.29 1.31) (end 8.29 -1.31) (layer \"F.SilkS\") (width 0.12) (tstamp fab7de09-20cf-4131-b969-27b695177867))\n"
        "    (fp_line (start -1.05 1.6) (end 11.25 1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp 3cb8ef09-2a19-4905-b082-c9534db84f47))\n"
        "    (fp_line (start -1.05 -1.6) (end -1.05 1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp 9dd0c083-9647-4d26-aebe-fabacbe66605))\n"
        "    (fp_line (start 11.25 1.6) (end 11.25 -1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp e01b3280-f982-4a3e-9f01-c1aaff70d915))\n"
        "    (fp_line (start 11.25 -1.6) (end -1.05 -1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp f602c95b-8818-4ea9-9f7a-b87bfa28f508))\n"
        "    (fp_line (start 8.23 1.25) (end 8.23 -1.25) (layer \"F.Fab\") (width 0.1) (tstamp 079bdccf-881c-4a76-bb47-fda88c1e8539))\n"
        "    (fp_line (start 8.23 -1.25) (end 1.93 -1.25) (layer \"F.Fab\") (width 0.1) (tstamp 31a6e71b-e018-445e-80bf-4390f0da9d94))\n"
        "    (fp_line (start 1.93 -1.25) (end 1.93 1.25) (layer \"F.Fab\") (width 0.1) (tstamp 394e6f3e-8168-4351-b602-150b25a3e918))\n"
        "    (fp_line (start 0 0) (end 1.93 0) (layer \"F.Fab\") (width 0.1) (tstamp 4d964dda-4fec-485c-8618-dd88da973f8f))\n"
        "    (fp_line (start 10.16 0) (end 8.23 0) (layer \"F.Fab\") (width 0.1) (tstamp 9985c0f0-7ff6-46bf-a22e-2fa42e34b9b1))\n"
        "    (fp_line (start 1.93 1.25) (end 8.23 1.25) (layer \"F.Fab\") (width 0.1) (tstamp cfc9a5ff-3c40-4d48-b7b6-8825e86fc710))\n"
        "    (pad \"1\" thru_hole circle (at 0 0) (size 1.8 1.8) (drill 0.85) (layers *.Cu *.Mask)\n"
        "      (net 1 \"GND\") (tstamp def83b42-5ec2-442a-8f2b-ffd7b14105ce))\n"
        "    (pad \"2\" thru_hole circle (at 10.16 0) (size 1.8 1.8) (drill 0.85) (layers *.Cu *.Mask) (tstamp 6386fdb7-f3d1-4839-9830-ab733b3e8d94))\n"
        "    (model \"${KICAD6_3DMODEL_DIR}/Resistors_THT.3dshapes/R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal.wrl\"\n"
        "      (offset (xyz 0 0 0))\n"
        "      (scale (xyz 0.393701 0.393701 0.393701))\n"
        "      (rotate (xyz 0 0 0))\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (module \"Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal\" (layer \"F.Cu\") (tedit 5A1048BD) (tstamp c3f66ae9-669e-4889-915f-2cccae13db42)\n"
        "    (at 28.9775 27.9525)\n"
        "    (descr \"Resistor, Axial_DIN0207 series, Axial, Horizontal, pin pitch=10.16mm, 0.25W = 1/4W, length*diameter=6.3*2.5mm^2, http://cdn-reichelt.de/documents/datenblatt/B400/1_4W%23YAG.pdf\")\n"
        "    (tags \"Resistor Axial_DIN0207 series Axial Horizontal pin pitch 10.16mm 0.25W = 1/4W length 6.3mm diameter 2.5mm\")\n"
        "    (fp_text reference \"R75\" (at 5.08 0) (layer \"F.SilkS\")\n"
        "      (effects (font (size 1.4 1.6) (thickness 0.3)))\n"
        "      (tstamp ef6e671a-a5ac-4bc4-8077-708c1a429470)\n"
        "    )\n"
        "    (fp_text value \"4K7\" (at 12.2925 0.06875 90) (layer \"F.Fab\")\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "      (tstamp 2c71cca7-0f57-4ce3-b3f4-64c98749588d)\n"
        "    )\n"
        "    (fp_line (start 8.29 -1.31) (end 1.87 -1.31) (layer \"F.SilkS\") (width 0.12) (tstamp 1033538b-29a3-4da7-8f54-f17c12867391))\n"
        "    (fp_line (start 1.87 -1.31) (end 1.87 1.31) (layer \"F.SilkS\") (width 0.12) (tstamp 2376397b-6e25-4057-90ca-3ebd6d1da982))\n"
        "    (fp_line (start 9.18 0) (end 8.29 0) (layer \"F.SilkS\") (width 0.12) (tstamp 5796fca2-ca98-4d4b-bec4-d31bde85a650))\n"
        "    (fp_line (start 0.98 0) (end 1.87 0) (layer \"F.SilkS\") (width 0.12) (tstamp 65f4508e-8af5-46fe-980c-f387540cb953))\n"
        "    (fp_line (start 8.29 1.31) (end 8.29 -1.31) (layer \"F.SilkS\") (width 0.12) (tstamp 79b31583-fa09-40ce-a54d-53e2344537e2))\n"
        "    (fp_line (start 1.87 1.31) (end 8.29 1.31) (layer \"F.SilkS\") (width 0.12) (tstamp 997a5329-dd3e-4903-b5bf-b579c496c0c2))\n"
        "    (fp_line (start -1.05 1.6) (end 11.25 1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp 504e4184-74f3-4bb3-9268-148507ff437b))\n"
        "    (fp_line (start 11.25 -1.6) (end -1.05 -1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp c014700f-6b3e-4db2-9179-a63ca83ee80a))\n"
        "    (fp_line (start 11.25 1.6) (end 11.25 -1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp cf155cca-0e54-4902-9419-cd47ab3b7a6e))\n"
        "    (fp_line (start -1.05 -1.6) (end -1.05 1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp da31697f-6c09-4894-9726-82e5965b76a9))\n"
        "    (fp_line (start 8.23 -1.25) (end 1.93 -1.25) (layer \"F.Fab\") (width 0.1) (tstamp 3f4a794f-b45b-42fe-8e89-c86eb999b1ac))\n"
        "    (fp_line (start 1.93 -1.25) (end 1.93 1.25) (layer \"F.Fab\") (width 0.1) (tstamp 495ba46e-e0f2-40df-bc2e-8273aef7c524))\n"
        "    (fp_line (start 0 0) (end 1.93 0) (layer \"F.Fab\") (width 0.1) (tstamp 7ff54d88-df7e-42a5-9c8b-88764220ede8))\n"
        "    (fp_line (start 8.23 1.25) (end 8.23 -1.25) (layer \"F.Fab\") (width 0.1) (tstamp 9d755f30-54b4-43f1-acea-b1a3ac435bd1))\n"
        "    (fp_line (start 10.16 0) (end 8.23 0) (layer \"F.Fab\") (width 0.1) (tstamp b999b5b2-327e-491b-9266-35b8482540c8))\n"
        "    (fp_line (start 1.93 1.25) (end 8.23 1.25) (layer \"F.Fab\") (width 0.1) (tstamp dfab8391-a4f8-43c8-8cf8-6b7e6a977081))\n"
        "    (pad \"1\" thru_hole circle (at 0 0) (size 1.8 1.8) (drill 0.85) (layers *.Cu *.Mask)\n"
        "      (net 1 \"GND\") (tstamp 5a702112-f16c-4b59-9902-914db0fa4360))\n"
        "    (pad \"2\" thru_hole circle (at 10.16 0) (size 1.8 1.8) (drill 0.85) (layers *.Cu *.Mask) (tstamp b2b0d740-b086-433e-90f5-ffe119efdd07))\n"
        "    (model \"${KICAD6_3DMODEL_DIR}/Resistors_THT.3dshapes/R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal.wrl\"\n"
        "      (offset (xyz 0 0 0))\n"
        "      (scale (xyz 0.393701 0.393701 0.393701))\n"
        "      (rotate (xyz 0 0 0))\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (module \"Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal\" (layer \"F.Cu\") (tedit 5A1048BD) (tstamp e072334e-f411-41c9-ade0-5444e440897b)\n"
        "    (at 39.1375 24.46 180)\n"
        "    (descr \"Resistor, Axial_DIN0207 series, Axial, Horizontal, pin pitch=10.16mm, 0.25W = 1/4W, length*diameter=6.3*2.5mm^2, http://cdn-reichelt.de/documents/datenblatt/B400/1_4W%23YAG.pdf\")\n"
        "    (tags \"Resistor Axial_DIN0207 series Axial Horizontal pin pitch 10.16mm 0.25W = 1/4W length 6.3mm diameter 2.5mm\")\n"
        "    (fp_text reference \"R73\" (at 5.08 0 180) (layer \"F.SilkS\")\n"
        "      (effects (font (size 1.3 1.5) (thickness 0.25)))\n"
        "      (tstamp 014cf222-f3a5-4b8e-8488-4309307a6bdc)\n"
        "    )\n"
        "    (fp_text value \"10K\" (at -2.0825 3.53875 270) (layer \"F.Fab\")\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "      (tstamp 2deddb35-a3f9-4993-a1ed-8c3208648d4e)\n"
        "    )\n"
        "    (fp_line (start 1.87 -1.31) (end 1.87 1.31) (layer \"F.SilkS\") (width 0.12) (tstamp 0de0f2c4-5641-4214-9afa-5e4cbfe4ee44))\n"
        "    (fp_line (start 1.87 1.31) (end 8.29 1.31) (layer \"F.SilkS\") (width 0.12) (tstamp 485d8c5a-a3d7-4c96-a3f3-f4b630b27e17))\n"
        "    (fp_line (start 8.29 1.31) (end 8.29 -1.31) (layer \"F.SilkS\") (width 0.12) (tstamp 5085ec5b-cfed-4f7c-bc9b-f7148c4b7222))\n"
        "    (fp_line (start 9.18 0) (end 8.29 0) (layer \"F.SilkS\") (width 0.12) (tstamp 7007fd01-2893-48e3-8840-341e7cb4ec8d))\n"
        "    (fp_line (start 8.29 -1.31) (end 1.87 -1.31) (layer \"F.SilkS\") (width 0.12) (tstamp 7a872980-3288-4161-8107-5d277e19ca53))\n"
        "    (fp_line (start 0.98 0) (end 1.87 0) (layer \"F.SilkS\") (width 0.12) (tstamp a7b6dccc-6e78-4d89-adc7-7c6cfb560515))\n"
        "    (fp_line (start 11.25 -1.6) (end -1.05 -1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp 2db3a7de-4ff9-4f7c-8023-a96c3233363e))\n"
        "    (fp_line (start 11.25 1.6) (end 11.25 -1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp 87f28b49-9648-446b-85a9-34010f350294))\n"
        "    (fp_line (start -1.05 1.6) (end 11.25 1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp 8b0e0021-dee0-4fba-85b0-dc1550b5b15c))\n"
        "    (fp_line (start -1.05 -1.6) (end -1.05 1.6) (layer \"F.CrtYd\") (width 0.05) (tstamp 9324d473-26e5-4c7d-95c0-367ba971acd7))\n"
        "    (fp_line (start 0 0) (end 1.93 0) (layer \"F.Fab\") (width 0.1) (tstamp 001328a4-472c-4707-a7bf-46a7a15f91f6))\n"
        "    (fp_line (start 1.93 1.25) (end 8.23 1.25) (layer \"F.Fab\") (width 0.1) (tstamp 1af02505-0a10-49cf-8a95-d3c06be3152d))\n"
        "    (fp_line (start 8.23 1.25) (end 8.23 -1.25) (layer \"F.Fab\") (width 0.1) (tstamp 65eee186-e6fe-44b5-a7e4-202672bff890))\n"
        "    (fp_line (start 1.93 -1.25) (end 1.93 1.25) (layer \"F.Fab\") (width 0.1) (tstamp 73b1c134-2d49-42d8-b1ba-e81b8df404ba))\n"
        "    (fp_line (start 10.16 0) (end 8.23 0) (layer \"F.Fab\") (width 0.1) (tstamp 7529bced-6b8a-483c-9aee-d7c487d3a292))\n"
        "    (fp_line (start 8.23 -1.25) (end 1.93 -1.25) (layer \"F.Fab\") (width 0.1) (tstamp c0322b20-ac45-4f20-bbab-44368f60ac5d))\n"
        "    (pad \"1\" thru_hole circle (at 0 0 180) (size 1.8 1.8) (drill 0.85) (layers *.Cu *.Mask) (tstamp 945fccb0-61fe-41e2-8f09-c8a33f294a3e))\n"
        "    (pad \"2\" thru_hole circle (at 10.16 0 180) (size 1.8 1.8) (drill 0.85) (layers *.Cu *.Mask) (tstamp d8e786d3-14bf-403c-80db-810abc9eda53))\n"
        "    (model \"${KICAD6_3DMODEL_DIR}/Resistors_THT.3dshapes/R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal.wrl\"\n"
        "      (offset (xyz 0 0 0))\n"
        "      (scale (xyz 0.393701 0.393701 0.393701))\n"
        "      (rotate (xyz 0 0 0))\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (module \"Capacitors_THT:CP_Radial_D5.0mm_P2.00mm\" (layer \"F.Cu\") (tedit 5C22DE41) (tstamp fa4fd0e8-6269-4cb7-a049-03c4e343895c)\n"
        "    (at 21.675 20.9675)\n"
        "    (descr \"CP, Radial series, Radial, pin pitch=2.00mm, , diameter=5mm, Electrolytic Capacitor\")\n"
        "    (tags \"CP Radial series Radial pin pitch 2.00mm  diameter 5mm Electrolytic Capacitor\")\n"
        "    (fp_text reference \"C47\" (at 0.9525 -3.33375) (layer \"F.SilkS\")\n"
        "      (effects (font (size 1 1.2) (thickness 0.22)))\n"
        "      (tstamp f13eb790-c841-466d-a0c1-07116ebc1073)\n"
        "    )\n"
        "    (fp_text value \"47uF\" (at 4.775 -0.01625 90) (layer \"F.Fab\")\n"
        "      (effects (font (size 1 1) (thickness 0.15)))\n"
        "      (tstamp 027137f5-363d-41b8-8495-34c7a18a7117)\n"
        "    )\n"
        "    (fp_line (start -1.6 -0.65) (end -1.6 0.65) (layer \"F.SilkS\") (width 0.12) (tstamp 87ac9cb8-d215-4372-8cd3-4abaacc0bf33))\n"
        "    (fp_line (start -2.2 0) (end -1 0) (layer \"F.SilkS\") (width 0.12) (tstamp d0ff0d50-cd0b-459a-8f2d-e84c2cb66ac5))\n"
        "    (fp_arc (start 1 0) (end 3.30558 -1.18) (angle 54.2) (layer \"F.SilkS\") (width 0.5) (tstamp 4404d1b4-9af5-4b29-a1e9-a89d90f6d256))\n"
        "    (fp_arc (start 1 0) (end -1.30558 -1.18) (angle 125.8) (layer \"F.SilkS\") (width 0.12) (tstamp 6fdf8300-e925-4956-a69a-91818d6e51c8))\n"
        "    (fp_arc (start 1 0) (end -1.30558 1.18) (angle -125.8) (layer \"F.SilkS\") (width 0.12) (tstamp ad60c624-9f4d-4a30-a5f4-59f698eff831))\n"
        "    (fp_line (start -1.85 -2.85) (end -1.85 2.85) (layer \"F.CrtYd\") (width 0.05) (tstamp 5ed611e7-0ad0-4da5-adb6-fcbf8512cd95))\n"
        "    (fp_line (start 3.85 -2.85) (end -1.85 -2.85) (layer \"F.CrtYd\") (width 0.05) (tstamp 6b007cca-ff72-43ca-ba72-b26458c5a115))\n"
        "    (fp_line (start -1.85 2.85) (end 3.85 2.85) (layer \"F.CrtYd\") (width 0.05) (tstamp 98571876-9364-410c-ac47-f290b1e46d98))\n"
        "    (fp_line (start 3.85 2.85) (end 3.85 -2.85) (layer \"F.CrtYd\") (width 0.05) (tstamp ba016114-5eb4-430b-ab2e-dbefa70d896a))\n"
        "    (fp_line (start -2.2 0) (end -1 0) (layer \"F.Fab\") (width 0.1) (tstamp 68c156d0-784a-4c9c-b97c-a32d6c3aa3b9))\n"
        "    (fp_line (start -1.6 -0.65) (end -1.6 0.65) (layer \"F.Fab\") (width 0.1) (tstamp 900d1364-baa6-4fae-a49a-97678ffcc675))\n"
        "    (fp_circle (center 1 0) (end 3.5 0) (layer \"F.Fab\") (width 0.1) (tstamp e572501a-055a-4591-8f6b-f3bf2e7ee86b))\n"
        "    (pad \"1\" thru_hole rect (at 0 0) (size 1.4 1.4) (drill 0.8) (layers *.Cu *.Mask)\n"
        "      (net 1 \"GND\") (tstamp 7ef87fdc-802f-4acb-9867-18732d4a7e7a))\n"
        "    (pad \"2\" thru_hole circle (at 2 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n"
        "      (net 1 \"GND\") (tstamp 1ad1f5f9-68f6-4131-9e09-bd1d361fc282))\n"
        "    (model \"${KICAD6_3DMODEL_DIR}/Capacitors_THT.3dshapes/CP_Radial_D5.0mm_P2.00mm.wrl\"\n"
        "      (offset (xyz 0 0 0))\n"
        "      (scale (xyz 1 1 1))\n"
        "      (rotate (xyz 0 0 0))\n"
        "    )\n"
        "  )\n"
        "\n"
        "  (gr_rect (start 15.65 13.74) (end 52.5 38.8) (layer \"Edge.Cuts\") (width 0.05) (tstamp a879b274-bf89-4c34-8db0-6e2a7291f014))\n"
        "  (dimension 3.6 (width 0.0508) (layer \"Margin\") (tstamp 6db437a2-8229-43a5-8bd8-1a58ddb201b9)\n"
        "    (gr_text \"0.1417 in\" (at 55.72 22.7 270) (layer \"Margin\") (tstamp 6db437a2-8229-43a5-8bd8-1a58ddb201b9)\n"
        "      (effects (font (size 0.762 0.762) (thickness 0.127)))\n"
        "    )\n"
        "    (feature1 (pts (xy 44.88 24.5) (xy 55.056421 24.5)))\n"
        "    (feature2 (pts (xy 44.88 20.9) (xy 55.056421 20.9)))\n"
        "    (crossbar (pts (xy 54.47 20.9) (xy 54.47 24.5)))\n"
        "    (arrow1a (pts (xy 54.47 24.5) (xy 53.883579 23.373496)))\n"
        "    (arrow1b (pts (xy 54.47 24.5) (xy 55.056421 23.373496)))\n"
        "    (arrow2a (pts (xy 54.47 20.9) (xy 53.883579 22.026504)))\n"
        "    (arrow2b (pts (xy 54.47 20.9) (xy 55.056421 22.026504)))\n"
        "  )\n"
        "\n"
        "  (segment (start 44.8525 24.46) (end 39.1375 24.46) (width 0.381) (layer \"F.Cu\") (net 0) (tstamp 7ca12133-777b-47ba-bbd3-cb8a544b2671) (status 30))\n"
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
        "    (min_thickness 0.254)\n"
        "    (fill yes (thermal_gap 0.508) (thermal_bridge_width 0.508))\n"
        "    (polygon\n"
        "      (pts\n"
        "        (xy 37.06 36.6)(xy 19.8 36.7)(xy 19.8 31.4)(xy 37.06 31.4)\n"
        "      )\n"
        "    )\n"
        "    (filled_polygon\n"
        "      (layer F.Cu)\n"
        "      (pts\n"
        "        (xy 36.934 36.474728)(xy 19.926 36.573268)(xy 19.926 35.808311)(xy 21.162708 35.808311)\n"
        "        (xy 21.165543 35.916543)(xy 21.219269 35.964917)(xy 21.225838 35.970086)(xy 21.480245 36.144282)\n"
        "        (xy 21.48744 36.148537)(xy 21.762651 36.287557)(xy 21.770346 36.290824)(xy 22.061526 36.392223)\n"
        "        (xy 22.069586 36.394443)(xy 22.371619 36.456441)(xy 22.379901 36.457575)(xy 22.68748 36.479084)\n"
        "        (xy 22.69584 36.479113)(xy 23.003562 36.459753)(xy 23.011852 36.458676)(xy 23.314311 36.398787)\n"
        "        (xy 23.322386 36.396624)(xy 23.614266 36.29726)(xy 23.621984 36.294047)(xy 23.898159 36.156953)\n"
        "        (xy 23.905384 36.152748)(xy 24.161002 35.980331)(xy 24.167608 35.975207)(xy 24.233615 35.916601)\n"
        "        (xy 24.236842 35.807861)(xy 22.752191 34.32321)(xy 22.647809 34.32321)(xy 21.162708 35.808311)\n"
        "        (xy 19.926 35.808311)(xy 19.926 34.035807)(xy 20.311498 34.035807)(xy 20.32441 34.343867)\n"
        "        (xy 20.325312 34.352178)(xy 20.378853 34.655824)(xy 20.380847 34.663942)(xy 20.474077 34.95784)\n"
        "        (xy 20.477127 34.965623)(xy 20.608409 35.244609)(xy 20.612461 35.251921)(xy 20.779486 35.511092)\n"
        "        (xy 20.78447 35.517803)(xy 20.872749 35.621533)(xy 20.983019 35.625962)(xy 22.46679 34.142191)\n"
        "        (xy 22.46679 34.037809)(xy 22.93321 34.037809)(xy 22.93321 34.142191)(xy 24.417196 35.626177)\n"
        "        (xy 24.526956 35.622152)(xy 24.605514 35.531142)(xy 24.610545 35.524466)(xy 24.779376 35.266467)\n"
        "        (xy 24.78348 35.259183)(xy 24.916705 34.98112)(xy 24.919809 34.973358)(xy 25.015089 34.680119)\n"
        "        (xy 25.017139 34.672015)(xy 25.072939 34.367992)(xy 25.073956 34.358143)(xy 25.08422 33.938144)\n"
        "        (xy 25.083685 33.928258)(xy 25.042804 33.621872)(xy 25.041152 33.613677)(xy 24.960311 33.316133)\n"
        "        (xy 24.95759 33.308228)(xy 24.838107 33.023991)(xy 24.834364 33.016516)(xy 24.678339 32.750577)\n"
        "        (xy 24.67364 32.743663)(xy 24.530633 32.560622)(xy 24.417327 32.553692)(xy 22.93321 34.037809)\n"
        "        (xy 22.46679 34.037809)(xy 20.979485 32.550504)(xy 20.863171 32.560067)(xy 20.671713 32.82749)\n"
        "        (xy 20.667308 32.834595)(xy 20.522555 33.106835)(xy 20.519128 33.11446)(xy 20.411653 33.403453)\n"
        "        (xy 20.409265 33.411464)(xy 20.340955 33.712132)(xy 20.339647 33.720389)(xy 20.311703 34.02745)\n"
        "        (xy 20.311498 34.035807)(xy 19.926 34.035807)(xy 19.926 32.373199)(xy 21.164218 32.373199)\n"
        "        (xy 22.647809 33.85679)(xy 22.752191 33.85679)(xy 24.237543 32.371437)(xy 24.229314 32.256623)\n"
        "        (xy 24.00471 32.088598)(xy 23.997699 32.084045)(xy 23.728551 31.933623)(xy 23.720999 31.930037)\n"
        "        (xy 23.43432 31.816533)(xy 23.426361 31.813978)(xy 23.12719 31.739386)(xy 23.118962 31.737905)\n"
        "        (xy 22.812553 31.703536)(xy 22.804202 31.703157)(xy 22.495939 31.709614)(xy 22.487612 31.710343)\n"
        "        (xy 22.18291 31.757513)(xy 22.174752 31.759336)(xy 21.878966 31.846391)(xy 21.871121 31.849277)\n"
        "        (xy 21.589447 31.974686)(xy 21.582052 31.978585)(xy 21.319439 32.140147)(xy 21.312624 32.144989)\n"
        "        (xy 21.169881 32.261408)(xy 21.164218 32.373199)(xy 19.926 32.373199)(xy 19.926 31.526)\n"
        "        (xy 36.934 31.526)\n"
        "      )\n"
        "    )\n"
        "  )"
        ")";

std::set<int> g_excludedLayers =
        {
            LAYER_VIAS,
            LAYER_VIA_HOLEWALLS,
            LAYER_MOD_FR,
            LAYER_MOD_BK,
            LAYER_PAD_FR,
            LAYER_PAD_BK,
            LAYER_MOD_VALUES,
            LAYER_MOD_REFERENCES,
            LAYER_TRACKS,
            LAYER_MOD_TEXT,
            LAYER_MOD_TEXT + 1,     // where LAYER_MOD_TEXT_BK used to be
            LAYER_PAD_PLATEDHOLES,
            LAYER_PAD_HOLEWALLS,
            LAYER_GP_OVERLAY,
            LAYER_DRAW_BITMAPS,
            LAYER_MARKER_SHADOWS
        };


PANEL_PCBNEW_COLOR_SETTINGS::PANEL_PCBNEW_COLOR_SETTINGS( PCB_EDIT_FRAME* aFrame,
                                                          wxWindow* aParent )
        : PANEL_COLOR_SETTINGS( aParent ),
          m_frame( aFrame ),
          m_preview( nullptr ),
          m_page( nullptr ),
          m_titleBlock( nullptr )
{
    m_colorNamespace = "board";

    SETTINGS_MANAGER* mgr          = m_frame->GetSettingsManager();
    PCBNEW_SETTINGS*  app_settings = mgr->GetAppSettings<PCBNEW_SETTINGS>();
    COLOR_SETTINGS*   current      = mgr->GetColorSettings( app_settings->m_ColorTheme );

    // Saved theme doesn't exist?  Reset to default
    if( current->GetFilename() != app_settings->m_ColorTheme )
        app_settings->m_ColorTheme = current->GetFilename();

    createThemeList( app_settings->m_ColorTheme );

    // Currently this only applies to eeschema
    m_optOverrideColors->Hide();

    m_currentSettings = new COLOR_SETTINGS( *current );

    for( int id = GAL_LAYER_ID_START; id < GAL_LAYER_ID_BITMASK_END; id++ )
    {
        if( g_excludedLayers.count( id ) )
            continue;

        m_validLayers.push_back( id );
    }

    // NOTE: Main board layers are added by createSwatches()

    m_backgroundLayer = LAYER_PCB_BACKGROUND;

    m_colorsMainSizer->Insert( 0, 10, 0, 0, wxEXPAND, 5 );

    createSwatches();

    m_preview = FOOTPRINT_PREVIEW_PANEL::New( &m_frame->Kiway(), this );
    m_preview->GetGAL()->SetAxesEnabled( false );

    m_colorsMainSizer->Add( 10, 0, 0, wxEXPAND, 5 );
    m_colorsMainSizer->Add( m_preview, 1, wxALL | wxEXPAND, 5 );
    m_colorsMainSizer->Add( 10, 0, 0, wxEXPAND, 5 );

    createPreviewItems();
    updatePreview();
    zoomFitPreview();
}


PANEL_PCBNEW_COLOR_SETTINGS::~PANEL_PCBNEW_COLOR_SETTINGS()
{
    delete m_page;
    delete m_titleBlock;
    delete m_currentSettings;
}


bool PANEL_PCBNEW_COLOR_SETTINGS::TransferDataFromWindow()
{
    SETTINGS_MANAGER* settingsMgr = m_frame->GetSettingsManager();
    PCBNEW_SETTINGS* app_settings = settingsMgr->GetAppSettings<PCBNEW_SETTINGS>();
    app_settings->m_ColorTheme = m_currentSettings->GetFilename();

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

    // Don't sort board layers by name
    for( int i = PCBNEW_LAYER_ID_START; i <= User_9; ++i )
        m_validLayers.insert( m_validLayers.begin() + i, i );

    BOARD* board = m_frame->GetBoard();

    for( int layer : m_validLayers )
    {
        wxString name = LayerName( layer );

        if( board && layer >= PCBNEW_LAYER_ID_START && layer < PCB_LAYER_ID_COUNT )
            name = board->GetLayerName( static_cast<PCB_LAYER_ID>( layer ) );

        createSwatch( layer, name );
    }

    // Give a minimal width to m_colorsListWindow, in order to always having
    // a full row shown
    int min_width = m_colorsGridSizer->GetMinSize().x;
    const int margin = 20;  // A margin around the sizer
    m_colorsListWindow->SetMinSize( wxSize( min_width + margin, -1 ) );
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

    STRING_LINE_READER reader( g_previewBoard, "preview" );
    PCB_IO             pi;

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

    DS_PROXY_VIEW_ITEM* drawingSheet = new DS_PROXY_VIEW_ITEM( (int) IU_PER_MILS, m_page, nullptr,
                                                               m_titleBlock );
    drawingSheet->SetColorLayer( LAYER_DRAWINGSHEET );
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
    KIGFX::VIEW* view = m_preview->GetView();
    BOX2I        bBox = m_preview->GetBoard()->GetBoundingBox();
    BOX2I        defaultBox = m_preview->GetDefaultViewBBox();

    view->SetScale( 1.0 );
    VECTOR2D screenSize = view->ToWorld( m_preview->GetClientSize(), false );

    if( bBox.GetWidth() == 0 || bBox.GetHeight() == 0 )
        bBox = defaultBox;

    VECTOR2D vsize = bBox.GetSize();
    double scale = view->GetScale() / std::max( fabs( vsize.x / screenSize.x ),
                                                fabs( vsize.y / screenSize.y ) );

    view->SetScale( scale / 1.1 );
    view->SetCenter( bBox.Centre() );
    m_preview->ForceRefresh();
}


void PANEL_PCBNEW_COLOR_SETTINGS::OnSize( wxSizeEvent& aEvent )
{
    zoomFitPreview();
    aEvent.Skip();
}


