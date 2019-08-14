/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <tool/tool_action.h>
#include <bitmaps.h>
#include <tools/kicad_manager_actions.h>
#include <frame_type.h>


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

TOOL_ACTION KICAD_MANAGER_ACTIONS::newProject( "kicad.Control.newProject",
        AS_GLOBAL,
        MD_CTRL + 'N', LEGACY_HK_NAME( "New Project" ),
        _( "New Project..." ), _( "Create new blank project" ),
        new_project_xpm );

TOOL_ACTION KICAD_MANAGER_ACTIONS::newFromTemplate( "kicad.Control.newFromTemplate",
        AS_GLOBAL,
        MD_CTRL + 'T', LEGACY_HK_NAME( "New Project From Template" ),
        _( "New Project from Template..." ), _( "Create new project from template" ),
        new_project_with_template_xpm );

TOOL_ACTION KICAD_MANAGER_ACTIONS::openProject( "kicad.Control.openProject",
        AS_GLOBAL,
        MD_CTRL + 'O', LEGACY_HK_NAME( "Open Project" ),
        _( "Open Project..." ), _( "Open an existing project" ),
        open_project_xpm );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editSchematic( "kicad.Control.editSchematic",
        AS_GLOBAL,
        MD_CTRL + 'E', LEGACY_HK_NAME( "Run Eeschema" ),
        _( "Edit Schematic" ), _( "Edit Schematic" ),
        eeschema_xpm, AF_NONE, (void*) FRAME_SCH );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editSymbols( "kicad.Control.editSymbols",
        AS_GLOBAL,
        MD_CTRL + 'L', LEGACY_HK_NAME( "Run LibEdit" ),
        _( "Edit Schematic Symbols" ), _( "Edit Schematic Symbols" ),
        libedit_xpm, AF_NONE, (void*) FRAME_SCH_LIB_EDITOR );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editPCB( "kicad.Control.editPCB",
        AS_GLOBAL,
        MD_CTRL + 'P', LEGACY_HK_NAME( "Run Pcbnew" ),
        _( "Edit PCB" ), _( "Edit PCB" ),
        pcbnew_xpm, AF_NONE, (void*) FRAME_PCB );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editFootprints( "kicad.Control.editFootprints",
        AS_GLOBAL,
        MD_CTRL + 'F', LEGACY_HK_NAME( "Run FpEditor" ),
        _( "Edit PCB Footprints" ), _( "Edit PCB Footprints" ),
        module_editor_xpm, AF_NONE, (void*) FRAME_PCB_MODULE_EDITOR );

TOOL_ACTION KICAD_MANAGER_ACTIONS::viewGerbers( "kicad.Control.viewGerbers",
        AS_GLOBAL,
        MD_CTRL + 'G', LEGACY_HK_NAME( "Run Gerbview" ),
        _( "View Gerber Files" ), _( "View Gerber Files" ),
        icon_gerbview_small_xpm );

TOOL_ACTION KICAD_MANAGER_ACTIONS::convertImage( "kicad.Control.convertImage",
        AS_GLOBAL,
        MD_CTRL + 'B', LEGACY_HK_NAME( "Run Bitmap2Component" ),
        _( "Convert Image" ), _( "Convert bitmap images to schematic or PCB components" ),
        bitmap2component_xpm );

TOOL_ACTION KICAD_MANAGER_ACTIONS::showCalculator( "kicad.Control.showCalculator",
        AS_GLOBAL,
        MD_CTRL + 'A', LEGACY_HK_NAME( "Run PcbCalculator" ),
        _( "Calculator Tools" ), _( "Run component calculations, track width calculations, etc." ),
        calculator_xpm );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editWorksheet( "kicad.Control.editWorksheet",
        AS_GLOBAL,
        MD_CTRL + 'Y', LEGACY_HK_NAME( "Run PlEditor" ),
        _( "Edit Worksheet" ), _( "Edit worksheet graphics and text" ),
        pagelayout_load_xpm );

TOOL_ACTION KICAD_MANAGER_ACTIONS::openTextEditor( "kicad.Control.openTextEditor",
        AS_GLOBAL,
        0, "",
        _( "Open Text Editor" ), _( "Launch preferred text editor" ),
        editor_xpm );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editOtherSch( "kicad.Control.editOtherSch",
        AS_GLOBAL );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editOtherPCB( "kicad.Control.editOtherPCB",
        AS_GLOBAL );


