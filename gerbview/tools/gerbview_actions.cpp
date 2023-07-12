/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <tool/tool_action.h>
#include <tool/tool_manager.h>
#include <tools/gerbview_actions.h>


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s


// GERBVIEW_CONTROL
//
TOOL_ACTION GERBVIEW_ACTIONS::openAutodetected( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.openAutodetected" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Open Autodetected File(s)..." ) )
        .Tooltip( _( "Open Autodetected file(s) on a new layer." ) )
        .Icon( BITMAPS::load_gerber ) );

TOOL_ACTION GERBVIEW_ACTIONS::openGerber( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.openGerber" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Open Gerber Plot File(s)..." ) )
        .Tooltip( _( "Open Gerber plot file(s) on a new layer." ) )
        .Icon( BITMAPS::load_gerber ) );

TOOL_ACTION GERBVIEW_ACTIONS::openDrillFile( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.openDrillFile" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Open Excellon Drill File(s)..." ) )
        .Tooltip( _( "Open Excellon drill file(s) on a new layer." ) )
        .Icon( BITMAPS::load_drill ) );

TOOL_ACTION GERBVIEW_ACTIONS::openJobFile( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.openJobFile" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Open Gerber Job File..." ) )
        .Tooltip( _( "Open a Gerber job file and its associated gerber plot files" ) )
        .Icon( BITMAPS::file_gerber_job ) );

TOOL_ACTION GERBVIEW_ACTIONS::openZipFile( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.openZipFile" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Open Zip Archive File..." ) )
        .Tooltip( _( "Open a zipped archive (Gerber and Drill) file" ) )
        .Icon( BITMAPS::zip ) );

TOOL_ACTION GERBVIEW_ACTIONS::toggleLayerManager( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.toggleLayerManager" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Show Layers Manager" ) )
        .Tooltip( _( "Show or hide the layer manager" ) )
        .Icon( BITMAPS::layers_manager ) );

TOOL_ACTION GERBVIEW_ACTIONS::showDCodes( TOOL_ACTION_ARGS()
        .Name( "gerbview.Inspection.showDCodes" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "List DCodes..." ) )
        .Tooltip( _( "List D-codes defined in Gerber files" ) )
        .Icon( BITMAPS::show_dcodenumber ) );

TOOL_ACTION GERBVIEW_ACTIONS::showSource( TOOL_ACTION_ARGS()
        .Name( "gerbview.Inspection.showSource" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Show Source..." ) )
        .Tooltip( _( "Show source file for the current layer" ) )
        .Icon( BITMAPS::tools ) );

TOOL_ACTION GERBVIEW_ACTIONS::exportToPcbnew( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.exportToPcbnew" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Export to PCB Editor..." ) )
        .Tooltip( _( "Export data as a KiCad PCB file" ) )
        .Icon( BITMAPS::export_to_pcbnew ) );

TOOL_ACTION GERBVIEW_ACTIONS::clearLayer( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.clearLayer" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Clear Current Layer..." ) )
        .Tooltip( _( "Clear the selected graphic layer" ) )
        .Icon( BITMAPS::delete_sheet ) );

TOOL_ACTION GERBVIEW_ACTIONS::clearAllLayers( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.clearAllLayers" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Clear All Layers" ) )
        .Tooltip( _( "Clear all layers. All data will be deleted" ) )
        .Icon( BITMAPS::delete_gerber ) );

TOOL_ACTION GERBVIEW_ACTIONS::reloadAllLayers( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.reloadAllLayers" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Reload All Layers" ) )
        .Tooltip( _( "Reload all layers. All data will be reloaded" ) )
        .Icon( BITMAPS::reload ) );

TOOL_ACTION GERBVIEW_ACTIONS::layerChanged( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.layerChanged" )
        .Scope( AS_GLOBAL )
        .Flags( AF_NOTIFY ) );

TOOL_ACTION GERBVIEW_ACTIONS::highlightClear( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.highlightClear" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Clear Highlight" ) )
        .Tooltip( _( "Clear Highlight" ) )
        .Icon( BITMAPS::cancel ) );

TOOL_ACTION GERBVIEW_ACTIONS::highlightNet( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.highlightNet" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Highlight Net" ) )
        .Tooltip( _( "Highlight Net" ) )
        .Icon( BITMAPS::general_ratsnest ) );

TOOL_ACTION GERBVIEW_ACTIONS::highlightComponent( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.highlightComponent" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Highlight Component" ) )
        .Tooltip( _( "Highlight Component" ) )
        .Icon( BITMAPS::module ) );

TOOL_ACTION GERBVIEW_ACTIONS::highlightAttribute( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.highlightAttribute" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Highlight Attribute" ) )
        .Tooltip( _( "Highlight Attribute" ) )
        .Icon( BITMAPS::flag ) );

TOOL_ACTION GERBVIEW_ACTIONS::highlightDCode( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.highlightDCode" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Highlight DCode" ) )
        .Tooltip( _( "Highlight DCode" ) )
        .Icon( BITMAPS::show_dcodenumber ) );

TOOL_ACTION GERBVIEW_ACTIONS::layerNext( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.layerNext" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_PAGEDOWN )
        .LegacyHotkeyName( "Switch to Next Layer" )
        .MenuText( _( "Next Layer" ) )
        .Tooltip( _( "Next Layer" ) ) );

TOOL_ACTION GERBVIEW_ACTIONS::layerPrev( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.layerPrev" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_PAGEUP )
        .LegacyHotkeyName( "Switch to Previous Layer" )
        .MenuText( _( "Previous Layer" ) )
        .Tooltip( _( "Previous Layer" ) ) );

TOOL_ACTION GERBVIEW_ACTIONS::moveLayerUp( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.moveLayerUp" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '+' )
        .MenuText( _( "Move Layer Up" ) )
        .Tooltip( _( "Move Current Layer up" ) )
        .Icon( BITMAPS::up ) );

TOOL_ACTION GERBVIEW_ACTIONS::moveLayerDown( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.moveLayerDown" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '-' )
        .MenuText( _( "Move Layer Down" ) )
        .Tooltip( _( "Move Current Layer Down" ) )
        .Icon( BITMAPS::down ) );

TOOL_ACTION GERBVIEW_ACTIONS::linesDisplayOutlines( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.linesDisplayOutlines" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'L' )
        .LegacyHotkeyName( "Gbr Lines Display Mode" )
        .MenuText( _( "Sketch Lines" ) )
        .Tooltip( _( "Show lines in outline mode" ) )
        .Icon( BITMAPS::showtrack ) );

TOOL_ACTION GERBVIEW_ACTIONS::flashedDisplayOutlines( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.flashedDisplayOutlines" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'F' )
        .LegacyHotkeyName( "Gbr Flashed Display Mode" )
        .MenuText( _( "Sketch Flashed Items" ) )
        .Tooltip( _( "Show flashed items in outline mode" ) )
        .Icon( BITMAPS::pad_sketch ) );

TOOL_ACTION GERBVIEW_ACTIONS::polygonsDisplayOutlines( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.polygonsDisplayOutlines" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'P' )
        .LegacyHotkeyName( "Gbr Polygons Display Mode" )
        .MenuText( _( "Sketch Polygons" ) )
        .Tooltip( _( "Show polygons in outline mode" ) )
        .Icon( BITMAPS::opt_show_polygon ) );

TOOL_ACTION GERBVIEW_ACTIONS::negativeObjectDisplay( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.negativeObjectDisplay" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'N' )
        .LegacyHotkeyName( "Gbr Negative Obj Display Mode" )
        .MenuText( _( "Ghost Negative Objects" ) )
        .Tooltip( _( "Show negative objects in ghost color" ) )
        .Icon( BITMAPS::gerbview_show_negative_objects ) );

TOOL_ACTION GERBVIEW_ACTIONS::dcodeDisplay( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.dcodeDisplay" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'D' )
        .LegacyHotkeyName( "DCodes Display Mode" )
        .MenuText( _( "Show DCodes" ) )
        .Tooltip( _( "Show dcode number" ) )
        .Icon( BITMAPS::show_dcodenumber ) );

TOOL_ACTION GERBVIEW_ACTIONS::toggleDiffMode( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.toggleDiffMode" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Show in Differential Mode" ) )
        .Tooltip( _( "Show layers in differential compare mode" ) )
        .Icon( BITMAPS::gbr_select_mode1 ) );

TOOL_ACTION GERBVIEW_ACTIONS::toggleXORMode( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.toggleXORMode" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Show in XOR Mode" ) )
        .Tooltip( _( "Show layers in exclusive-or compare mode" ) )
        .Icon( BITMAPS::gbr_select_mode2 ) );

TOOL_ACTION GERBVIEW_ACTIONS::flipGerberView( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.flipGerberView" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Flip Gerber View" ) )
        .Tooltip( _( "Show as mirror image" ) )
        .Icon( BITMAPS::flip_board ) );


// GERBVIEW_SELECTION_TOOL
//
TOOL_ACTION GERBVIEW_ACTIONS::selectionActivate( TOOL_ACTION_ARGS()
        .Name( "gerbview.InteractiveSelection" )
        .Scope( AS_GLOBAL )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION GERBVIEW_ACTIONS::selectItem( TOOL_ACTION_ARGS()
        .Name( "gerbview.InteractiveSelection.SelectItem" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION GERBVIEW_ACTIONS::unselectItem( TOOL_ACTION_ARGS()
        .Name( "gerbview.InteractiveSelection.UnselectItem" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION GERBVIEW_ACTIONS::selectionClear( TOOL_ACTION_ARGS()
        .Name( "gerbview.InteractiveSelection.Clear" )
        .Scope( AS_GLOBAL ) );


// Drag and drop
//
TOOL_ACTION GERBVIEW_ACTIONS::loadZipFile( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.loadZipFile" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION GERBVIEW_ACTIONS::loadGerbFiles( TOOL_ACTION_ARGS()
        .Name( "gerbview.Control.loadGerbFiles" )
        .Scope( AS_GLOBAL ) );
