/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "common_actions.h"
#include <tool/action_manager.h>
#include <pcbnew_id.h>
#include <wx/defs.h>

// Selection tool actions
TOOL_ACTION COMMON_ACTIONS::selectionActivate( "pcbnew.InteractiveSelection",
        AS_GLOBAL, 0, "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION COMMON_ACTIONS::selectionSingle( "pcbnew.InteractiveSelection.Single",
        AS_GLOBAL, 0, "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION COMMON_ACTIONS::selectionClear( "pcbnew.InteractiveSelection.Clear",
        AS_GLOBAL, 0, "", "" );    // No description, it is not supposed to be shown anywhere

// Edit tool actions
TOOL_ACTION COMMON_ACTIONS::editActivate( "pcbnew.InteractiveEdit",
        AS_GLOBAL, 'M',
        "Move", "Moves the selected item(s)" );

TOOL_ACTION COMMON_ACTIONS::rotate( "pcbnew.rotate",
        AS_CONTEXT, 'R',
        "Rotate", "Rotates selected item(s)" );

TOOL_ACTION COMMON_ACTIONS::flip( "pcbnew.flip",
        AS_CONTEXT, 'F',
        "Flip", "Flips selected item(s)" );

TOOL_ACTION COMMON_ACTIONS::remove( "pcbnew.InteractiveEdit.remove",
        AS_GLOBAL, WXK_DELETE,
        "Remove", "Deletes selected item(s)" );

TOOL_ACTION COMMON_ACTIONS::properties( "pcbnew.InteractiveEdit.properties",
        AS_GLOBAL, 'E',
        "Properties...", "Displays properties window" );

// Drawing tool actions
TOOL_ACTION COMMON_ACTIONS::drawLine( "pcbnew.InteractiveDrawing.line",
        AS_GLOBAL, 0,
        "Draw a line", "Draw a line" );

TOOL_ACTION COMMON_ACTIONS::drawCircle( "pcbnew.InteractiveDrawing.circle",
        AS_GLOBAL, 0,
        "Draw a circle", "Draw a circle" );

TOOL_ACTION COMMON_ACTIONS::drawArc( "pcbnew.InteractiveDrawing.arc",
        AS_GLOBAL, 0,
        "Draw an arc", "Draw an arc" );

TOOL_ACTION COMMON_ACTIONS::drawText( "pcbnew.InteractiveDrawing.text",
        AS_GLOBAL, 0,
        "Add a text", "Add a text" );

TOOL_ACTION COMMON_ACTIONS::drawDimension( "pcbnew.InteractiveDrawing.dimension",
        AS_GLOBAL, 0,
        "Add a dimension", "Add a dimension" );

TOOL_ACTION COMMON_ACTIONS::drawZone( "pcbnew.InteractiveDrawing.zone",
        AS_GLOBAL, 0,
        "Add a filled zone", "Add a filled zone" );

TOOL_ACTION COMMON_ACTIONS::drawKeepout( "pcbnew.InteractiveDrawing.keepout",
        AS_GLOBAL, 0,
        "Add a keepout area", "Add a keepout area" );

TOOL_ACTION COMMON_ACTIONS::placeTarget( "pcbnew.InteractiveDrawing.placeTarget",
        AS_GLOBAL, 0,
        "Add layer alignment target", "Add layer alignment target" );

TOOL_ACTION COMMON_ACTIONS::placeModule( "pcbnew.InteractiveDrawing.placeModule",
        AS_GLOBAL, 0,
        "Add modules", "Add modules" );

TOOL_ACTION COMMON_ACTIONS::routerActivate( "pcbnew.InteractiveRouter",
        AS_GLOBAL, 0,
        "Run push & shove router", "Run push & shove router" );

TOOL_ACTION COMMON_ACTIONS::pointEditorUpdate( "pcbnew.PointEditor.update",
        AS_GLOBAL, 0, "", "" );    // No description, it is not supposed to be shown anywhere


std::string COMMON_ACTIONS::TranslateLegacyId( int aId )
{
    switch( aId )
    {
    case ID_PCB_MODULE_BUTT:
        return COMMON_ACTIONS::placeModule.GetName();

    case ID_TRACK_BUTT:
        return COMMON_ACTIONS::routerActivate.GetName();

    case ID_PCB_ZONES_BUTT:
        return COMMON_ACTIONS::drawZone.GetName();

    case ID_PCB_KEEPOUT_AREA_BUTT:
        return COMMON_ACTIONS::drawKeepout.GetName();

    case ID_PCB_ADD_LINE_BUTT:
        return COMMON_ACTIONS::drawLine.GetName();

    case ID_PCB_CIRCLE_BUTT:
        return COMMON_ACTIONS::drawCircle.GetName();

    case ID_PCB_ARC_BUTT:
        return COMMON_ACTIONS::drawArc.GetName();

    case ID_PCB_ADD_TEXT_BUTT:
        return COMMON_ACTIONS::drawText.GetName();

    case ID_PCB_DIMENSION_BUTT:
        return COMMON_ACTIONS::drawDimension.GetName();

    case ID_PCB_MIRE_BUTT:
        return COMMON_ACTIONS::placeTarget.GetName();
    }

    return "";
}
