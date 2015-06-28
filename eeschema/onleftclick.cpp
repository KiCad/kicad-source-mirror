/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file eeschema/onleftclick.cpp
 */

#include <fctsys.h>
#include <eeschema_id.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <schframe.h>
#include <menus_helpers.h>

#include <sch_bus_entry.h>
#include <sch_text.h>
#include <sch_marker.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_bitmap.h>
#include <class_library.h>      // fo class SCHLIB_FILTER to filter power parts


// TODO(hzeller): These pairs of elmenets should be represented by an object, but don't want
// to refactor too much right now to not get in the way with other code changes.
static wxArrayString s_CmpNameList;
static int s_CmpLastUnit;

static wxArrayString s_PowerNameList;
static int s_LastPowerUnit;


void SCH_EDIT_FRAME::OnLeftClick( wxDC* aDC, const wxPoint& aPosition )
{
    SCH_ITEM*   item = GetScreen()->GetCurItem();
    wxPoint     gridPosition = GetGridPosition( aPosition );

    if( ( GetToolId() == ID_NO_TOOL_SELECTED ) || ( item && item->GetFlags() ) )
    {
        m_canvas->SetAutoPanRequest( false );
        SetRepeatItem( NULL );

        if( item && item->GetFlags() )
        {
            switch( item->Type() )
            {
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIERARCHICAL_LABEL_T:
            case SCH_TEXT_T:
            case SCH_SHEET_PIN_T:
            case SCH_SHEET_T:
            case SCH_BUS_WIRE_ENTRY_T:
            case SCH_BUS_BUS_ENTRY_T:
            case SCH_JUNCTION_T:
            case SCH_COMPONENT_T:
            case SCH_FIELD_T:
            case SCH_BITMAP_T:
            case SCH_NO_CONNECT_T:
                addCurrentItemToList();
                return;

            case SCH_LINE_T:    // May already be drawing segment.
                break;

            default:
                wxFAIL_MSG( wxT( "SCH_EDIT_FRAME::OnLeftClick error.  Item type <" ) +
                            item->GetClass() + wxT( "> is already being edited." ) );
                item->ClearFlags();
                break;
            }
        }
        else
        {
            item = LocateAndShowItem( aPosition );
        }
    }

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        break;

    case ID_HIERARCHY_PUSH_POP_BUTT:
        if( ( item && item->GetFlags() ) || ( g_RootSheet->CountSheets() == 0 ) )
            break;

        item = LocateAndShowItem( aPosition, SCH_COLLECTOR::SheetsOnly );

        if( item )  // The user has clicked on a sheet: this is an enter sheet command
        {
            m_CurrentSheet->Push( (SCH_SHEET*) item );
            DisplayCurrentSheet();
        }
        else if( m_CurrentSheet->Last() != g_RootSheet )
        {   // The user has clicked ouside a sheet:this is an leave sheet command
            m_CurrentSheet->Pop();
            DisplayCurrentSheet();
        }
        break;

    case ID_NOCONN_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            if( GetScreen()->GetItem( gridPosition, 0, SCH_NO_CONNECT_T ) == NULL )
            {
                SCH_NO_CONNECT*  no_connect = AddNoConnect( aDC, gridPosition );
                SetRepeatItem( no_connect );
                GetScreen()->SetCurItem( no_connect );
                m_canvas->SetAutoPanRequest( true );
            }
        }
        else
        {
            addCurrentItemToList();
        }
        break;

    case ID_JUNCTION_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            if( GetScreen()->GetItem( gridPosition, 0, SCH_JUNCTION_T ) == NULL )
            {
                SCH_JUNCTION* junction = AddJunction( aDC, gridPosition, true );
                SetRepeatItem( junction );
                GetScreen()->SetCurItem( junction );
                m_canvas->SetAutoPanRequest( true );
            }
        }
        else
        {
            addCurrentItemToList();
        }
        break;

    case ID_WIRETOBUS_ENTRY_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            CreateBusWireEntry();
            m_canvas->SetAutoPanRequest( true );
        }
        else
        {
            addCurrentItemToList();
        }
        break;

    case ID_BUSTOBUS_ENTRY_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            CreateBusBusEntry();
            m_canvas->SetAutoPanRequest( true );
        }
        else
        {
            addCurrentItemToList();
        }
        break;

    case ID_SCHEMATIC_DELETE_ITEM_BUTT:
        DeleteItemAtCrossHair( aDC );
        break;

    case ID_WIRE_BUTT:
        BeginSegment( aDC, LAYER_WIRE );
        m_canvas->SetAutoPanRequest( true );
        break;

    case ID_BUS_BUTT:
        BeginSegment( aDC, LAYER_BUS );
        m_canvas->SetAutoPanRequest( true );
        break;

    case ID_LINE_COMMENT_BUTT:
        BeginSegment( aDC, LAYER_NOTES );
        m_canvas->SetAutoPanRequest( true );
        break;

    case ID_TEXT_COMMENT_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            GetScreen()->SetCurItem( CreateNewText( aDC, LAYER_NOTES ) );
            m_canvas->SetAutoPanRequest( true );
        }
        else
        {
            addCurrentItemToList();
        }
        break;

    case ID_ADD_IMAGE_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            GetScreen()->SetCurItem( CreateNewImage( aDC ) );
            m_canvas->SetAutoPanRequest( true );
        }
        else
        {
            addCurrentItemToList();
        }
        break;

    case ID_LABEL_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            GetScreen()->SetCurItem( CreateNewText( aDC, LAYER_LOCLABEL ) );
            m_canvas->SetAutoPanRequest( true );
        }
        else
        {
            addCurrentItemToList();
        }
        break;

    case ID_GLABEL_BUTT:
    case ID_HIERLABEL_BUTT:
        if( (item == NULL) || (item->GetFlags() == 0) )
        {
            if( GetToolId() == ID_GLABEL_BUTT )
                GetScreen()->SetCurItem( CreateNewText( aDC, LAYER_GLOBLABEL ) );

            if( GetToolId() == ID_HIERLABEL_BUTT )
                GetScreen()->SetCurItem( CreateNewText( aDC, LAYER_HIERLABEL ) );

            m_canvas->SetAutoPanRequest( true );
        }
        else
        {
            addCurrentItemToList();
        }
        break;

    case ID_SHEET_SYMBOL_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            item = CreateSheet( aDC );

            if( item != NULL )
            {
                GetScreen()->SetCurItem( item );
                m_canvas->SetAutoPanRequest( true );
            }
        }
        else
        {
            addCurrentItemToList();
        }
        break;

    case ID_IMPORT_HLABEL_BUTT:
    case ID_SHEET_PIN_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
            item = LocateAndShowItem( aPosition, SCH_COLLECTOR::SheetsAndSheetLabels );

        if( item == NULL )
            break;

        if( (item->Type() == SCH_SHEET_T) && (item->GetFlags() == 0) )
        {
            if( GetToolId() == ID_IMPORT_HLABEL_BUTT )
                GetScreen()->SetCurItem( ImportSheetPin( (SCH_SHEET*) item, aDC ) );
            else
                GetScreen()->SetCurItem( CreateSheetPin( (SCH_SHEET*) item, aDC ) );
        }
        else if( (item->Type() == SCH_SHEET_PIN_T) && (item->GetFlags() != 0) )
        {
            addCurrentItemToList();
        }
        break;

    case ID_SCH_PLACE_COMPONENT:
        if( (item == NULL) || (item->GetFlags() == 0) )
        {
            GetScreen()->SetCurItem( Load_Component( aDC, NULL,
                                                     s_CmpNameList, s_CmpLastUnit, true ) );
            m_canvas->SetAutoPanRequest( true );
        }
        else
        {
            addCurrentItemToList();
        }
        break;

    case ID_PLACE_POWER_BUTT:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            SCHLIB_FILTER filter;
            filter.FilterPowerParts( true );
            GetScreen()->SetCurItem( Load_Component( aDC, &filter,
                                                     s_PowerNameList, s_LastPowerUnit, false ) );
            m_canvas->SetAutoPanRequest( true );
        }
        else
        {
            addCurrentItemToList();
        }
        break;

    default:
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        wxFAIL_MSG( wxT( "SCH_EDIT_FRAME::OnLeftClick invalid tool ID <" ) +
                    wxString::Format( wxT( "%d> selected." ), GetToolId() ) );
    }
}


/**
 * Function OnLeftDClick
 * called on a double click event from the drawpanel mouse handler
 *  if an editable item is found (text, component)
 *      Call the suitable dialog editor.
 *  Id a create command is in progress:
 *      validate and finish the command
 */
void SCH_EDIT_FRAME::OnLeftDClick( wxDC* aDC, const wxPoint& aPosition )

{
    EDA_ITEM* item = GetScreen()->GetCurItem();

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            item = LocateAndShowItem( aPosition );
        }

        if( ( item == NULL ) || ( item->GetFlags() != 0 ) )
            break;

        switch( item->Type() )
        {
        case SCH_SHEET_T:
            m_CurrentSheet->Push( (SCH_SHEET*) item );
            DisplayCurrentSheet();
            break;

        case SCH_COMPONENT_T:
            EditComponent( (SCH_COMPONENT*) item );
            GetCanvas()->MoveCursorToCrossHair();

            if( item->GetFlags() == 0 )
                GetScreen()->SetCurItem( NULL );

            GetCanvas()->Refresh();
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
            EditSchematicText( (SCH_TEXT*) item );
            break;

        case SCH_BITMAP_T:
            EditImage( (SCH_BITMAP*) item );
            break;

        case SCH_FIELD_T:
            EditComponentFieldText( (SCH_FIELD*) item );
            GetCanvas()->MoveCursorToCrossHair();
            break;

        case SCH_MARKER_T:
            ( (SCH_MARKER*) item )->DisplayMarkerInfo( this );
            break;

        default:
            break;
        }

        break;

    case ID_BUS_BUTT:
    case ID_WIRE_BUTT:
    case ID_LINE_COMMENT_BUTT:
        if( item && item->IsNew() )
            EndSegment( aDC );

        break;
    }
}
