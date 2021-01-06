/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/tool_manager.h>
#include <gerbview_id.h>
#include <bitmaps.h>
#include "gerbview_actions.h"


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s


// GERBVIEW_CONTROL
//
TOOL_ACTION GERBVIEW_ACTIONS::openGerber( "gerbview.Control.openGerber",
        AS_GLOBAL, 0, "",
        _( "Open Gerber Plot File(s)..." ),
        _( "Open Gerber plot file(s) on the current layer. Previous data will be deleted" ),
        load_gerber_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::openDrillFile( "gerbview.Control.openDrillFile",
        AS_GLOBAL, 0, "",
        _( "Open Excellon Drill File(s)..." ),
        _( "Open Excellon drill file(s) on the current layer. Previous data will be deleted" ),
        load_drill_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::openJobFile( "gerbview.Control.openJobFile",
        AS_GLOBAL, 0, "",
        _( "Open Gerber Job File..." ),
        _( "Open a Gerber job file and its associated gerber plot files" ),
        gerber_job_file_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::openZipFile( "gerbview.Control.openZipFile",
        AS_GLOBAL, 0, "",
        _( "Open Zip Archive File..." ),
        _( "Open a zipped archive (Gerber and Drill) file" ),
        zip_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::toggleLayerManager( "gerbview.Control.toggleLayerManager",
        AS_GLOBAL, 0, "",
        _( "Show Layers Manager" ),
        _( "Show or hide the layer manager" ),
        layers_manager_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::showDCodes( "gerbview.Inspection.showDCodes",
        AS_GLOBAL, 0, "",
        _( "List DCodes..." ),
        _( "List D-codes defined in Gerber files" ),
        show_dcodenumber_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::showSource( "gerbview.Inspection.showSource",
        AS_GLOBAL, 0, "",
        _( "Show Source..." ),
        _( "Show source file for the current layer" ),
        tools_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::exportToPcbnew( "gerbview.Control.exportToPcbnew",
        AS_GLOBAL, 0, "",
        _( "Export to Pcbnew..." ),
        _( "Export data in Pcbnew format" ),
        export_to_pcbnew_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::clearLayer( "gerbview.Control.clearLayer",
        AS_GLOBAL, 0, "",
        _( "Clear Current Layer..." ), _( "Clear the selected graphic layer" ),
        delete_sheet_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::clearAllLayers( "gerbview.Control.clearAllLayers",
        AS_GLOBAL, 0, "",
        _( "Clear All Layers" ),
        _( "Clear all layers. All data will be deleted" ),
        delete_gerber_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::reloadAllLayers( "gerbview.Control.reloadAllLayers",
        AS_GLOBAL, 0, "",
        _( "Reload All Layers" ),
        _( "Reload all layers. All data will be reloaded" ),
        reload_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::layerChanged( "gerbview.Control.layerChanged",
        AS_GLOBAL, 0, "", "", "",
        nullptr, AF_NOTIFY );

TOOL_ACTION GERBVIEW_ACTIONS::highlightClear( "gerbview.Control.highlightClear",
        AS_GLOBAL, 0, "",
        _( "Clear Highlight" ), "" );

TOOL_ACTION GERBVIEW_ACTIONS::highlightNet( "gerbview.Control.highlightNet",
        AS_GLOBAL, 0, "",
        _( "Highlight Net" ), "",
        general_ratsnest_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::highlightComponent( "gerbview.Control.highlightComponent",
        AS_GLOBAL, 0, "",
        _( "Highlight Component" ), "",
        module_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::highlightAttribute( "gerbview.Control.highlightAttribute",
        AS_GLOBAL, 0, "",
        _( "Highlight Attribute" ), "",
        flag_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::layerNext( "gerbview.Control.layerNext",
        AS_GLOBAL,
        '+', LEGACY_HK_NAME( "Switch to Next Layer" ),
        _( "Next Layer" ), "" );

TOOL_ACTION GERBVIEW_ACTIONS::layerPrev( "gerbview.Control.layerPrev",
        AS_GLOBAL,
        '-', LEGACY_HK_NAME( "Switch to Previous Layer" ),
        _( "Previous Layer" ), "" );

TOOL_ACTION GERBVIEW_ACTIONS::linesDisplayOutlines( "gerbview.Control.linesDisplayOutlines",
        AS_GLOBAL,
        'L', LEGACY_HK_NAME( "Gbr Lines Display Mode" ),
        _( "Sketch Lines" ), _( "Show lines in outline mode" ),
        showtrack_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::flashedDisplayOutlines( "gerbview.Control.flashedDisplayOutlines",
        AS_GLOBAL,
        'F', LEGACY_HK_NAME( "Gbr Flashed Display Mode" ),
        _( "Sketch Flashed Items" ), _( "Show flashed items in outline mode" ),
        pad_sketch_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::polygonsDisplayOutlines( "gerbview.Control.polygonsDisplayOutlines",
        AS_GLOBAL,
        'P', LEGACY_HK_NAME( "Gbr Polygons Display Mode" ),
        _( "Sketch Polygons" ), _( "Show polygons in outline mode" ),
        opt_show_polygon_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::negativeObjectDisplay( "gerbview.Control.negativeObjectDisplay",
        AS_GLOBAL,
        'N', LEGACY_HK_NAME( "Gbr Negative Obj Display Mode" ),
        _( "Ghost Negative Objects" ), _( "Show negative objects in ghost color" ),
        gerbview_show_negative_objects_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::dcodeDisplay( "gerbview.Control.dcodeDisplay",
        AS_GLOBAL,
        'D', LEGACY_HK_NAME( "DCodes Display Mode" ),
        _( "Show DCodes" ), _( "Show dcode number" ),
        show_dcodenumber_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::toggleDiffMode( "gerbview.Control.toggleDiffMode",
        AS_GLOBAL, 0, "",
        _( "Show in Differential Mode" ), _( "Show layers in diff (compare) mode" ),
        gbr_select_mode2_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::flipGerberView( "gerbview.Control.flipGerberView",
        AS_GLOBAL, 0, "",
        _( "Flip Gerber View" ), _( "Show as mirror image" ),
        flip_board_xpm );


// GERBVIEW_SELECTION_TOOL
//
TOOL_ACTION GERBVIEW_ACTIONS::selectionActivate( "gerbview.InteractiveSelection",
        AS_GLOBAL, 0, "",
        "", "", NULL, AF_ACTIVATE ); // No description, it is not supposed to be shown anywhere

TOOL_ACTION GERBVIEW_ACTIONS::selectItem( "gerbview.InteractiveSelection.SelectItem",
        AS_GLOBAL );

TOOL_ACTION GERBVIEW_ACTIONS::unselectItem( "gerbview.InteractiveSelection.UnselectItem",
        AS_GLOBAL );

TOOL_ACTION GERBVIEW_ACTIONS::selectionClear( "gerbview.InteractiveSelection.Clear",
        AS_GLOBAL );


