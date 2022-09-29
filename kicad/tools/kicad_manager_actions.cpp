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

TOOL_ACTION KICAD_MANAGER_ACTIONS::newProject( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.newProject" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'N' )
        .LegacyHotkeyName( "New Project" )
        .MenuText( _( "New Project..." ) )
        .Tooltip( _( "Create new blank project" ) )
        .Icon( BITMAPS::new_project ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::newFromTemplate( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.newFromTemplate" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'T' )
        .LegacyHotkeyName( "New Project From Template" )
        .MenuText( _( "New Project from Template..." ) )
        .Tooltip( _( "Create new project from template" ) )
        .Icon( BITMAPS::new_project_from_template ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::openDemoProject( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.openDemoProject" )
        .Scope( AS_GLOBAL )
        .LegacyHotkeyName( "Open Demo Project" )
        .MenuText( _( "Open Demo Project..." ) )
        .Tooltip( _( "Open a demo project" ) )
        .Icon( BITMAPS::open_project_demo ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::openProject( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.openProject" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'O' )
        .LegacyHotkeyName( "Open Project" )
        .MenuText( _( "Open Project..." ) )
        .Tooltip( _( "Open an existing project" ) )
        .Icon( BITMAPS::open_project ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::closeProject( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.closeProject" )
        .Scope( AS_GLOBAL )
        .LegacyHotkeyName( "Close Project" )
        .MenuText( _( "Close Project" ) )
        .Tooltip( _( "Close the current project" ) )
        .Icon( BITMAPS::project_close ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::loadProject( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.loadProject" )
        .Scope( AS_GLOBAL )
        .Parameter<wxString*>( nullptr ) );      // Default to no filename

TOOL_ACTION KICAD_MANAGER_ACTIONS::importNonKicadProj( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.importNonKicadProj" )
        .Scope( AS_GLOBAL )
        .Parameter<wxString*>( nullptr ) );      // Default to no filename

TOOL_ACTION KICAD_MANAGER_ACTIONS::viewDroppedGerbers( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.viewDroppedGerbers" )
        .Scope( AS_GLOBAL )
        .Parameter<wxString*>( nullptr ) );      // Default to no filename

TOOL_ACTION KICAD_MANAGER_ACTIONS::editSchematic( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.editSchematic" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'E' )
        .LegacyHotkeyName( "Run Eeschema" )
        .MenuText( _( "Schematic Editor" ) )
        .Tooltip( _( "Edit schematic" ) )
        .Icon( BITMAPS::icon_eeschema_24 )
        .Flags( AF_NONE )
        .Parameter( FRAME_SCH ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editSymbols( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.editSymbols" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'L' )
        .LegacyHotkeyName( "Run LibEdit" )
        .MenuText( _( "Symbol Editor" ) )
        .Tooltip( _( "Edit schematic symbols" ) )
        .Icon( BITMAPS::icon_libedit_24 )
        .Flags( AF_NONE )
        .Parameter( FRAME_SCH_SYMBOL_EDITOR ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editPCB( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.editPCB" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'P' )
        .LegacyHotkeyName( "Run Pcbnew" )
        .MenuText( _( "PCB Editor" ) )
        .Tooltip( _( "Edit PCB" ) )
        .Icon( BITMAPS::icon_pcbnew_24 )
        .Flags( AF_NONE )
        .Parameter( FRAME_PCB_EDITOR ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editFootprints( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.editFootprints" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'F' )
        .LegacyHotkeyName( "Run FpEditor" )
        .MenuText( _( "Footprint Editor" ) )
        .Tooltip( _( "Edit PCB footprints" ) )
        .Icon( BITMAPS::icon_modedit_24 )
        .Flags( AF_NONE )
        .Parameter( FRAME_FOOTPRINT_EDITOR ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::viewGerbers( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.viewGerbers" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'G' )
        .LegacyHotkeyName( "Run Gerbview" )
        .MenuText( _( "Gerber Viewer" ) )
        .Tooltip( _( "Preview Gerber output files" ) )
        .Icon( BITMAPS::icon_gerbview_24 )
        .Parameter<wxString*>( nullptr ) );      // Default to no filename

TOOL_ACTION KICAD_MANAGER_ACTIONS::convertImage( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.convertImage" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'B' )
        .LegacyHotkeyName( "Run Bitmap2Component" )
        .MenuText( _( "Image Converter" ) )
        .Tooltip( _( "Convert bitmap images to schematic or PCB components" ) )
        .Icon( BITMAPS::icon_bitmap2component_24 )
        .Parameter<wxString*>( nullptr ) );      // Default to no filename

TOOL_ACTION KICAD_MANAGER_ACTIONS::showCalculator( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.showCalculator" )
        .Scope( AS_GLOBAL )
        .LegacyHotkeyName( "Run PcbCalculator" )
        .MenuText( _( "Calculator Tools" ) )
        .Tooltip( _( "Run component calculations, track width calculations, etc." ) )
        .Icon( BITMAPS::icon_pcbcalculator_24 )
        .Parameter<wxString*>( nullptr ) );      // Default to no filename

TOOL_ACTION KICAD_MANAGER_ACTIONS::editDrawingSheet( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.editDrawingSheet" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'Y' )
        .LegacyHotkeyName( "Run PlEditor" )
        .MenuText( _( "Drawing Sheet Editor" ) )
        .Tooltip( _( "Edit drawing sheet borders and title block" ) )
        .Icon( BITMAPS::icon_pagelayout_editor_24 )
        .Parameter<wxString*>( nullptr ) );      // Default to no filename

TOOL_ACTION KICAD_MANAGER_ACTIONS::showPluginManager( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.pluginContentManager" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'M' )
        .MenuText( _( "Plugin and Content Manager" ) )
        .Tooltip( _( "Run Plugin and Content Manager" ) )
        .Icon( BITMAPS::icon_pcm_24 ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::openTextEditor( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.openTextEditor" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Open Text Editor" ) )
        .Tooltip( _( "Launch preferred text editor" ) )
        .Icon( BITMAPS::editor )
        .Parameter<wxString*>( nullptr ) );      // Default to no filename

TOOL_ACTION KICAD_MANAGER_ACTIONS::editOtherSch( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.editOtherSch" )
        .Scope( AS_GLOBAL )
        .Parameter<wxString*>( nullptr ) );      // Default to no filename

TOOL_ACTION KICAD_MANAGER_ACTIONS::editOtherPCB( TOOL_ACTION_ARGS()
        .Name( "kicad.Control.editOtherPCB" )
        .Scope( AS_GLOBAL )
        .Parameter<wxString*>( nullptr ) );      // Default to no filename
